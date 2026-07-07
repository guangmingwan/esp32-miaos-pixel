#include "ota_app_flash.h"

#include <Arduino.h>
#include <SD.h>
#include <esp_heap_caps.h>
#include <esp_ota_ops.h>
#include <esp_image_format.h>
#include <esp_rom_crc.h>
#include <esp_system.h>
#include <soc/soc.h>
#include <string.h>

#include "int_wdt_guard.h"
#include "launcher_log.h"

static constexpr size_t FLASH_CHUNK_SIZE = 4096;

const esp_partition_t *miaFindOtaPartition() {
  return esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                  ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
}

const esp_partition_t *miaFindAppSlot() {
  return esp_partition_find_first(ESP_PARTITION_TYPE_APP,
                                  ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
}

static bool toVfsPath(const char *sdPath, char *vfsPath, size_t vfsPathSize) {
  if (sdPath == nullptr || vfsPath == nullptr || vfsPathSize == 0) {
    return false;
  }
  const char *separator = sdPath[0] == '/' ? "" : "/";
  const int written = snprintf(vfsPath, vfsPathSize, "/sd%s%s", separator, sdPath);
  return written >= 0 && static_cast<size_t>(written) < vfsPathSize;
}

esp_err_t miaForceOtaBoot(const esp_partition_t *target) {
  const esp_partition_t *otap = miaFindOtaPartition();
  if (!otap || !target) {
    return ESP_ERR_NOT_FOUND;
  }

  typedef struct __attribute__((packed)) {
    uint32_t ota_seq;
    uint8_t seq_label[20];
    uint32_t ota_state;
    uint32_t crc;
  } OtaEntry;

  const uint8_t slot = target->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_MIN;
  const uint8_t ota_app_count = 2;

  auto setEntry = [](OtaEntry *e, uint32_t seq, uint32_t state) {
    memset(e, 0, sizeof(OtaEntry));
    e->ota_seq = seq;
    e->ota_state = state;
    e->crc = esp_rom_crc32_le(UINT32_MAX, (uint8_t *)&e->ota_seq, 4);
  };

  OtaEntry entries[2];
  setEntry(&entries[0], slot + 1, ESP_OTA_IMG_VALID);
  setEntry(&entries[1], slot + 1 + ota_app_count, ESP_OTA_IMG_VALID);

  esp_err_t err = esp_partition_erase_range(otap, 0, otap->size);
  if (err != ESP_OK) {
    return err;
  }
  err = esp_partition_write(otap, 0, &entries[0], sizeof(OtaEntry));
  if (err != ESP_OK) {
    return err;
  }
  return esp_partition_write(otap, 4096, &entries[1], sizeof(OtaEntry));
}

OtaAppFlashResult miaFlashAppToSlot(const char *sdPath, bool sdReady) {
  launcherTracef("[ota-flash] request path='%s' sdReady=%d",
                 sdPath == nullptr ? "<null>" : sdPath, sdReady ? 1 : 0);

  if (!sdReady) {
    return {OtaAppFlashStatus::SdUnavailable, 0, 0};
  }
  if (sdPath == nullptr || sdPath[0] == '\0') {
    return {OtaAppFlashStatus::InvalidPath, 0, 0};
  }

  File file = SD.open(sdPath);
  if (!file) {
    launcherTrace("[ota-flash] open failed");
    return {OtaAppFlashStatus::OpenFailed, 0, 0};
  }

  const size_t firmwareSize = file.size();
  if (firmwareSize == 0) {
    file.close();
    launcherTrace("[ota-flash] empty file");
    return {OtaAppFlashStatus::EmptyFile, 0, 0};
  }

  const esp_partition_t *slot = miaFindAppSlot();
  if (!slot) {
    file.close();
    launcherTrace("[ota-flash] ota_1 partition not found");
    return {OtaAppFlashStatus::NoPartition, 0, 0};
  }

  if (firmwareSize > slot->size) {
    file.close();
    launcherTracef("[ota-flash] too large: %u > %u",
                   static_cast<unsigned>(firmwareSize),
                   static_cast<unsigned>(slot->size));
    return {OtaAppFlashStatus::TooLarge, 0, 0};
  }

  ScopedIntWdtPause wdtGuard;

  const size_t eraseSize =
      (firmwareSize + SPI_FLASH_SEC_SIZE - 1) & ~(size_t)(SPI_FLASH_SEC_SIZE - 1);
  launcherTracef("[ota-flash] erase ota_1 size=0x%08x", static_cast<unsigned>(eraseSize));
  esp_err_t err = esp_partition_erase_range(slot, 0, eraseSize);
  if (err != ESP_OK) {
    file.close();
    launcherTracef("[ota-flash] erase failed err=0x%x", err);
    return {OtaAppFlashStatus::EraseFailed, err, 0};
  }

  uint8_t *buf = static_cast<uint8_t *>(
      heap_caps_malloc(FLASH_CHUNK_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (!buf) {
    file.close();
    launcherTrace("[ota-flash] alloc failed");
    return {OtaAppFlashStatus::AllocFailed, 0, 0};
  }

  size_t written = 0;
  while (written < firmwareSize) {
    const size_t toRead =
        (firmwareSize - written < FLASH_CHUNK_SIZE) ? (firmwareSize - written) : FLASH_CHUNK_SIZE;
    const size_t bytesRead = file.read(buf, toRead);
    if (bytesRead != toRead) {
      free(buf);
      file.close();
      launcherTracef("[ota-flash] read failed at offset=0x%08x",
                     static_cast<unsigned>(written));
      return {OtaAppFlashStatus::ReadFailed, 0, written};
    }

    size_t writeSize = bytesRead;
    if (writeSize % 4 != 0) {
      const size_t padding = 4 - (writeSize % 4);
      memset(buf + writeSize, 0xFF, padding);
      writeSize += padding;
    }

    err = esp_partition_write(slot, written, buf, writeSize);
    if (err != ESP_OK) {
      free(buf);
      file.close();
      launcherTracef("[ota-flash] write failed at offset=0x%08x err=0x%x",
                     static_cast<unsigned>(written), err);
      return {OtaAppFlashStatus::WriteFailed, err, written};
    }

    written += bytesRead;
    if ((written % (64 * 1024)) == 0) {
      launcherTracef("[ota-flash] progress 0x%08x / 0x%08x",
                     static_cast<unsigned>(written),
                     static_cast<unsigned>(firmwareSize));
    }
  }

  free(buf);
  file.close();

  launcherTracef("[ota-flash] write complete bytes=%u, setting otadata -> ota_1",
                 static_cast<unsigned>(written));
  err = miaForceOtaBoot(slot);
  if (err != ESP_OK) {
    launcherTracef("[ota-flash] otadata write failed err=0x%x", err);
    return {OtaAppFlashStatus::OtaDataFailed, err, written};
  }

  launcherTrace("[ota-flash] rebooting to app slot");
  delay(100);
  ESP.restart();
  return {OtaAppFlashStatus::Ok, 0, written};
}

const char *miaOtaAppFlashStatusText(OtaAppFlashStatus status) {
  switch (status) {
    case OtaAppFlashStatus::Ok:
      return "OK";
    case OtaAppFlashStatus::SdUnavailable:
      return "SD unavailable";
    case OtaAppFlashStatus::InvalidPath:
      return "Invalid path";
    case OtaAppFlashStatus::OpenFailed:
      return "Open failed";
    case OtaAppFlashStatus::EmptyFile:
      return "Empty file";
    case OtaAppFlashStatus::NoPartition:
      return "No OTA partition";
    case OtaAppFlashStatus::TooLarge:
      return "Firmware too large";
    case OtaAppFlashStatus::EraseFailed:
      return "Erase failed";
    case OtaAppFlashStatus::ReadFailed:
      return "SD read failed";
    case OtaAppFlashStatus::WriteFailed:
      return "Flash write failed";
    case OtaAppFlashStatus::AllocFailed:
      return "Memory alloc failed";
    case OtaAppFlashStatus::OtaDataFailed:
      return "OTA data write failed";
  }
  return "Unknown";
}

void miaBootAppSlot() {
  const esp_partition_t *slot = miaFindAppSlot();
  if (!slot) {
    launcherTrace("[ota-boot] ota_1 partition not found");
    return;
  }
  launcherTrace("[ota-boot] setting otadata -> ota_1, rebooting");
  esp_err_t err = miaForceOtaBoot(slot);
  if (err != ESP_OK) {
    launcherTracef("[ota-boot] otadata write failed err=0x%x", err);
  }
  delay(100);
  ESP.restart();
}

bool miaReadOtaManifest(OtaAppManifest *manifest) {
  if (!manifest) return false;

  const esp_partition_t *slot = miaFindAppSlot();
  if (!slot) return false;

  esp_partition_pos_t part = {};
  part.offset = slot->address;
  part.size = slot->size;
  esp_image_metadata_t metadata = {};
  esp_err_t err = esp_image_get_metadata(&part, &metadata);
  if (err != ESP_OK || metadata.image_len == 0 || metadata.image_len > slot->size) {
    return false;
  }

  // Manifest trailer is at offset image_len (after the ESP-IDF image)
  const size_t trailerOffset = metadata.image_len;
  if (trailerOffset + MIA_MANIFEST_TRAILER_SIZE > slot->size) {
    return false;
  }

  uint8_t buf[MIA_MANIFEST_TRAILER_SIZE];
  err = esp_partition_read(slot, trailerOffset, buf, sizeof(buf));
  if (err != ESP_OK) return false;

  memcpy(manifest, buf, sizeof(buf));
  return miaManifestIsValid(manifest) != 0;
}

bool miaReadManifestFromFile(const char *sdPath, OtaAppManifest *manifest) {
  if (!sdPath || !manifest) return false;

  File file = SD.open(sdPath);
  if (!file) return false;

  const size_t fileSize = file.size();
  if (fileSize < static_cast<size_t>(MIA_MANIFEST_TRAILER_SIZE)) {
    file.close();
    return false;
  }

  if (!file.seek(fileSize - MIA_MANIFEST_TRAILER_SIZE)) {
    file.close();
    return false;
  }

  uint8_t buf[MIA_MANIFEST_TRAILER_SIZE];
  if (file.read(buf, sizeof(buf)) != static_cast<int>(sizeof(buf))) {
    file.close();
    return false;
  }
  file.close();

  memcpy(manifest, buf, sizeof(buf));
  return miaManifestIsValid(manifest) != 0;
}

OtaAppExportResult miaExportAppSlotToSd(const char *sdPath, bool sdReady) {
  char vfsPath[192];
  esp_partition_pos_t part = {};
  esp_image_metadata_t metadata = {};
  const esp_partition_t *slot = miaFindAppSlot();

  launcherTracef("[ota-export] request path='%s' sdReady=%d",
                 sdPath == nullptr ? "<null>" : sdPath, sdReady ? 1 : 0);

  if (!sdReady) {
    return {OtaAppExportStatus::SdUnavailable, 0, 0};
  }
  if (sdPath == nullptr || sdPath[0] == '\0' || !toVfsPath(sdPath, vfsPath, sizeof(vfsPath))) {
    return {OtaAppExportStatus::InvalidPath, 0, 0};
  }
  if (!slot) {
    launcherTrace("[ota-export] ota_1 partition not found");
    return {OtaAppExportStatus::NoPartition, 0, 0};
  }

  part.offset = slot->address;
  part.size = slot->size;
  esp_err_t err = esp_image_get_metadata(&part, &metadata);
  if (err != ESP_OK || metadata.image_len == 0 || metadata.image_len > slot->size) {
    launcherTracef("[ota-export] invalid image err=0x%x len=%u", err,
                   static_cast<unsigned>(metadata.image_len));
    return {OtaAppExportStatus::InvalidImage, err, 0};
  }

  if (SD.exists(sdPath)) {
    SD.remove(sdPath);
  }
  File out = SD.open(sdPath, FILE_WRITE);
  if (!out) {
    launcherTrace("[ota-export] open for write failed");
    return {OtaAppExportStatus::OpenFailed, 0, 0};
  }

  uint8_t *buf = static_cast<uint8_t *>(heap_caps_malloc(FLASH_CHUNK_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (!buf) {
    out.close();
    return {OtaAppExportStatus::WriteFailed, 0, 0};
  }

  size_t offset = 0;
  while (offset < metadata.image_len) {
    const size_t toRead = (metadata.image_len - offset < FLASH_CHUNK_SIZE)
                              ? (metadata.image_len - offset)
                              : FLASH_CHUNK_SIZE;
    err = esp_partition_read(slot, offset, buf, toRead);
    if (err != ESP_OK) {
      free(buf);
      out.close();
      launcherTracef("[ota-export] partition read failed offset=0x%08x err=0x%x",
                     static_cast<unsigned>(offset), err);
      return {OtaAppExportStatus::ReadFailed, err, offset};
    }
    const size_t written = out.write(buf, toRead);
    if (written != toRead) {
      free(buf);
      out.close();
      launcherTracef("[ota-export] sd write failed offset=0x%08x", static_cast<unsigned>(offset));
      return {OtaAppExportStatus::WriteFailed, 0, offset};
    }
    offset += toRead;
  }

  free(buf);

  OtaAppManifest trailer;
  if (offset + MIA_MANIFEST_TRAILER_SIZE <= slot->size) {
    uint8_t trailerBuf[MIA_MANIFEST_TRAILER_SIZE];
    err = esp_partition_read(slot, offset, trailerBuf, sizeof(trailerBuf));
    if (err == ESP_OK) {
      memcpy(&trailer, trailerBuf, sizeof(trailer));
      if (miaManifestIsValid(&trailer)) {
        const size_t written = out.write(trailerBuf, sizeof(trailerBuf));
        if (written == sizeof(trailerBuf)) {
          offset += written;
          launcherTracef("[ota-export] manifest trailer included (%u bytes)",
                         static_cast<unsigned>(written));
        }
      }
    }
  }

  out.close();
  launcherTracef("[ota-export] done bytes=%u -> %s", static_cast<unsigned>(offset), sdPath);
  return {OtaAppExportStatus::Ok, 0, offset};
}

OtaAppExportResult miaExportOtaToSd(bool sdReady) {
  if (!sdReady) {
    return {OtaAppExportStatus::SdUnavailable, 0, 0};
  }

  OtaAppManifest manifest;
  if (!miaReadOtaManifest(&manifest)) {
    launcherTrace("[ota-export-auto] no manifest in ota_1");
    return {OtaAppExportStatus::ManifestMissing, 0, 0};
  }

  char category[16];
  char appName[32];
  strncpy(category, manifest.category, sizeof(category) - 1);
  category[sizeof(category) - 1] = '\0';
  strncpy(appName, manifest.name, sizeof(appName) - 1);
  appName[sizeof(appName) - 1] = '\0';

  char sdPath[128];
  const int pathLen = snprintf(sdPath, sizeof(sdPath), "/MiaOS/%s/%s.app/%s.bin",
                                category, appName, appName);
  if (pathLen < 0 || static_cast<size_t>(pathLen) >= sizeof(sdPath)) {
    return {OtaAppExportStatus::InvalidPath, 0, 0};
  }

  launcherTracef("[ota-export-auto] manifest category=%s name=%s path=%s",
                 category, appName, sdPath);

  char dirPath[96];
  const int dirLen = snprintf(dirPath, sizeof(dirPath), "/MiaOS/%s/%s.app",
                               category, appName);
  if (dirLen < 0 || static_cast<size_t>(dirLen) >= sizeof(dirPath)) {
    return {OtaAppExportStatus::InvalidPath, 0, 0};
  }

  if (!SD.mkdir(dirPath)) {
    launcherTracef("[ota-export-auto] mkdir failed: %s", dirPath);
    return {OtaAppExportStatus::MkdirFailed, 0, 0};
  }

  return miaExportAppSlotToSd(sdPath, sdReady);
}

const char *miaOtaAppExportStatusText(OtaAppExportStatus status) {
  switch (status) {
    case OtaAppExportStatus::Ok:
      return "OK";
    case OtaAppExportStatus::SdUnavailable:
      return "SD unavailable";
    case OtaAppExportStatus::InvalidPath:
      return "Invalid path";
    case OtaAppExportStatus::NoPartition:
      return "No OTA partition";
    case OtaAppExportStatus::InvalidImage:
      return "Invalid image";
    case OtaAppExportStatus::OpenFailed:
      return "Open failed";
    case OtaAppExportStatus::ReadFailed:
      return "Flash read failed";
    case OtaAppExportStatus::WriteFailed:
      return "SD write failed";
    case OtaAppExportStatus::ManifestMissing:
      return "No manifest in ota_1";
    case OtaAppExportStatus::MkdirFailed:
      return "SD mkdir failed";
  }
  return "Unknown";
}
