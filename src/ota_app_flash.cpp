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

static void normalizeV1Manifest(const OtaAppManifestV1 &legacy, OtaAppManifest *manifest) {
  memset(manifest, 0, sizeof(*manifest));
  manifest->magic = MIA_APP_MANIFEST_V1_MAGIC;
  memcpy(manifest->category, legacy.category, sizeof(manifest->category));
  memcpy(manifest->name, legacy.name, sizeof(manifest->name));
  manifest->crc = legacy.crc;
}

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
  esp_err_t err;
  {
    // Pause TG1 IWDT during image header verification; on this ESP32-S3 board
    // esp_image_get_metadata can trigger a watchdog reset when the partition
    // has no valid image or corrupted data.
    ScopedIntWdtPause wdtGuard;
    err = esp_image_get_metadata(&part, &metadata);
  }
  if (err != ESP_OK || metadata.image_len == 0 || metadata.image_len > slot->size) {
    return false;
  }

  // Scan forward from image_len to find the manifest trailer.
  // esptool.py pads each segment to 16 bytes and appends an optional SHA-256
  // hash, so metadata.image_len points before the trailer location.  Scan a
  // reasonable region for the MIA1 magic.
  {
    const size_t scanBytes = 512;
    const size_t scanEnd = min(metadata.image_len + scanBytes, slot->size);
    const size_t actualBytes = scanEnd - metadata.image_len;

    uint8_t *scanBuf = (uint8_t *)heap_caps_malloc(actualBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!scanBuf) return false;

    err = esp_partition_read(slot, metadata.image_len, scanBuf, actualBytes);
    if (err != ESP_OK) {
      free(scanBuf);
      return false;
    }

    bool found = false;
    for (size_t i = 0; i + sizeof(OtaAppManifestV1) <= actualBytes; i++) {
      uint32_t magic;
      memcpy(&magic, scanBuf + i, sizeof(magic));
      if (magic == MIA_APP_MANIFEST_MAGIC && i + sizeof(*manifest) <= actualBytes) {
        memcpy(manifest, scanBuf + i, sizeof(*manifest));
        if (miaManifestIsValid(manifest)) {
          found = true;
          break;
        }
      } else if (magic == MIA_APP_MANIFEST_V1_MAGIC) {
        OtaAppManifestV1 legacy;
        memcpy(&legacy, scanBuf + i, sizeof(legacy));
        if (miaManifestV1IsValid(&legacy)) {
          normalizeV1Manifest(legacy, manifest);
          found = true;
          break;
        }
      }
    }
    free(scanBuf);
    if (found) return true;
  }

  return false;
}

// Create a directory and all parents.  Arduino SD.mkdir doesn't create
// intermediate directories, so we split at '/' and create each component.
static bool recursiveSdMkdir(const char *path) {
  if (!path || path[0] == '\0') return false;

  char buf[128];
  const size_t len = strnlen(path, sizeof(buf) - 1);
  if (len >= sizeof(buf) - 1) return false;
  memcpy(buf, path, len);
  buf[len] = '\0';

  char *p = buf;
  if (*p == '/') ++p;
  while (true) {
    char *slash = strchr(p, '/');
    if (slash) *slash = '\0';
    if (buf[0] != '\0' && !SD.mkdir(buf)) {
      if (!SD.exists(buf)) return false;  // mkdir fails if dir exists
    }
    if (!slash) break;
    *slash = '/';
    p = slash + 1;
  }
  return true;
}

bool miaReadManifestFromFile(const char *sdPath, OtaAppManifest *manifest) {
  if (!sdPath || !manifest) return false;

  File file = SD.open(sdPath);
  if (!file) return false;

  const size_t fileSize = file.size();
  if (fileSize >= sizeof(OtaAppManifest) && file.seek(fileSize - sizeof(OtaAppManifest))) {
    OtaAppManifest current;
    if (file.read(reinterpret_cast<uint8_t *>(&current), sizeof(current)) == sizeof(current) &&
        miaManifestIsValid(&current)) {
      file.close();
      *manifest = current;
      return true;
    }
  }
  if (fileSize >= sizeof(OtaAppManifestV1) && file.seek(fileSize - sizeof(OtaAppManifestV1))) {
    OtaAppManifestV1 legacy;
    if (file.read(reinterpret_cast<uint8_t *>(&legacy), sizeof(legacy)) == sizeof(legacy) &&
        miaManifestV1IsValid(&legacy)) {
      file.close();
      normalizeV1Manifest(legacy, manifest);
      return true;
    }
  }
  file.close();
  return false;
}

static OtaAppExportResult exportAppSlotToSd(const char *sdPath, bool sdReady,
                                             OtaAppSyncProgress progress, void *context) {
  char vfsPath[192];
  esp_partition_pos_t part = {};
  esp_image_metadata_t metadata = {};
  const esp_partition_t *slot = miaFindAppSlot();
  OtaAppManifest manifest;

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
  if (!miaReadOtaManifest(&manifest)) {
    return {OtaAppExportStatus::ManifestMissing, 0, 0};
  }

  part.offset = slot->address;
  part.size = slot->size;
  esp_err_t err;
  {
    ScopedIntWdtPause wdtGuard;
    err = esp_image_get_metadata(&part, &metadata);
  }
  if (err != ESP_OK || metadata.image_len == 0 || metadata.image_len > slot->size) {
    launcherTracef("[ota-export] invalid image err=0x%x len=%u", err,
                   static_cast<unsigned>(metadata.image_len));
    return {OtaAppExportStatus::InvalidImage, err, 0};
  }

  const size_t imageBytes = manifest.magic == MIA_APP_MANIFEST_MAGIC
                                ? static_cast<size_t>(manifest.image_size)
                                : static_cast<size_t>(metadata.image_len);
  if (imageBytes < metadata.image_len || imageBytes > slot->size) {
    return {OtaAppExportStatus::InvalidImage, 0, 0};
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
  uint32_t imageCrc = 0;
  while (offset < imageBytes) {
    const size_t toRead = (imageBytes - offset < FLASH_CHUNK_SIZE)
                              ? (imageBytes - offset)
                              : FLASH_CHUNK_SIZE;
    err = esp_partition_read(slot, offset, buf, toRead);
    if (err != ESP_OK) {
      free(buf);
      out.close();
      launcherTracef("[ota-export] partition read failed offset=0x%08x err=0x%x",
                     static_cast<unsigned>(offset), err);
      return {OtaAppExportStatus::ReadFailed, err, offset};
    }
    imageCrc = esp_rom_crc32_le(imageCrc, buf, toRead);
    const size_t written = out.write(buf, toRead);
    if (written != toRead) {
      free(buf);
      out.close();
      launcherTracef("[ota-export] sd write failed offset=0x%08x", static_cast<unsigned>(offset));
      return {OtaAppExportStatus::WriteFailed, 0, offset};
    }
    offset += toRead;
    if (progress) progress("Writing OTA app", manifest.name, offset, imageBytes, context);
  }

  free(buf);
  if (manifest.magic == MIA_APP_MANIFEST_MAGIC && imageCrc != manifest.image_crc) {
    out.close();
    SD.remove(sdPath);
    launcherTracef("[ota-export] image CRC mismatch expected=%08x actual=%08x",
                   static_cast<unsigned>(manifest.image_crc), static_cast<unsigned>(imageCrc));
    return {OtaAppExportStatus::VerifyFailed, 0, offset};
  }

  size_t trailerWritten = 0;
  if (manifest.magic == MIA_APP_MANIFEST_MAGIC) {
    trailerWritten = out.write(reinterpret_cast<const uint8_t *>(&manifest), sizeof(manifest));
  } else {
    OtaAppManifestV1 legacy = {};
    legacy.magic = MIA_APP_MANIFEST_V1_MAGIC;
    memcpy(legacy.category, manifest.category, sizeof(legacy.category));
    memcpy(legacy.name, manifest.name, sizeof(legacy.name));
    legacy.crc = manifest.crc;
    trailerWritten = out.write(reinterpret_cast<const uint8_t *>(&legacy), sizeof(legacy));
  }
  const size_t expectedTrailer = manifest.magic == MIA_APP_MANIFEST_MAGIC
                                     ? sizeof(OtaAppManifest)
                                     : sizeof(OtaAppManifestV1);
  if (trailerWritten != expectedTrailer) {
    out.close();
    SD.remove(sdPath);
    return {OtaAppExportStatus::WriteFailed, 0, offset};
  }
  offset += trailerWritten;

  out.close();
  launcherTracef("[ota-export] done bytes=%u -> %s", static_cast<unsigned>(offset), sdPath);
  return {OtaAppExportStatus::Ok, 0, offset};
}

OtaAppExportResult miaExportAppSlotToSd(const char *sdPath, bool sdReady) {
  return exportAppSlotToSd(sdPath, sdReady, nullptr, nullptr);
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

  if (!recursiveSdMkdir(dirPath)) {
    launcherTracef("[ota-export-auto] mkdir failed: %s", dirPath);
    return {OtaAppExportStatus::MkdirFailed, 0, 0};
  }

  return miaExportAppSlotToSd(sdPath, sdReady);
}

OtaAppSyncResult miaSyncNewerOtaToSd(bool sdReady, OtaAppSyncProgress progress,
                                     void *context) {
  if (!sdReady) {
    return {OtaAppSyncStatus::SdUnavailable, OtaAppExportStatus::SdUnavailable, 0};
  }
  OtaAppManifest ota;
  if (!miaReadOtaManifest(&ota)) {
    return {OtaAppSyncStatus::OtaManifestMissing, OtaAppExportStatus::ManifestMissing, 0};
  }
  if (ota.magic != MIA_APP_MANIFEST_MAGIC || ota.build_epoch == 0 || ota.image_size == 0) {
    return {OtaAppSyncStatus::OtaManifestLegacy, OtaAppExportStatus::ManifestMissing, 0};
  }

  char category[MIA_MANIFEST_CATEGORY_SIZE];
  char appName[MIA_MANIFEST_NAME_SIZE];
  memcpy(category, ota.category, sizeof(category));
  memcpy(appName, ota.name, sizeof(appName));
  category[sizeof(category) - 1] = '\0';
  appName[sizeof(appName) - 1] = '\0';
  if (category[0] == '\0' || appName[0] == '\0' || strstr(category, "..") ||
      strstr(appName, "..") || strpbrk(category, "/\\") || strpbrk(appName, "/\\")) {
    return {OtaAppSyncStatus::Failed, OtaAppExportStatus::InvalidPath, 0};
  }

  char sdPath[128];
  char directPath[128];
  snprintf(sdPath, sizeof(sdPath), "/MiaOS/%s/%s.app/%s.bin", category, appName, appName);
  snprintf(directPath, sizeof(directPath), "/%s/%s.app/%s.bin", category, appName, appName);
  if (SD.exists(directPath)) strncpy(sdPath, directPath, sizeof(sdPath));
  sdPath[sizeof(sdPath) - 1] = '\0';

  OtaAppManifest sd = {};
  if (miaReadManifestFromFile(sdPath, &sd) && sd.magic == MIA_APP_MANIFEST_MAGIC &&
      sd.build_epoch >= ota.build_epoch) {
    launcherTracef("[ota-sync] %s up to date OTA=%llu SD=%llu", appName,
                   static_cast<unsigned long long>(ota.build_epoch),
                   static_cast<unsigned long long>(sd.build_epoch));
    return {OtaAppSyncStatus::UpToDate, OtaAppExportStatus::Ok, 0};
  }

  char dirPath[96];
  snprintf(dirPath, sizeof(dirPath), "/MiaOS/%s/%s.app", category, appName);
  if (strncmp(sdPath, "/MiaOS/", 7) == 0 && !recursiveSdMkdir(dirPath)) {
    return {OtaAppSyncStatus::Failed, OtaAppExportStatus::MkdirFailed, 0};
  }

  char tempPath[144];
  char backupPath[144];
  snprintf(tempPath, sizeof(tempPath), "%s.tmp", sdPath);
  snprintf(backupPath, sizeof(backupPath), "%s.bak", sdPath);
  SD.remove(tempPath);
  SD.remove(backupPath);
  launcherTracef("[ota-sync] updating %s OTA=%llu SD=%llu", appName,
                 static_cast<unsigned long long>(ota.build_epoch),
                 static_cast<unsigned long long>(sd.magic == MIA_APP_MANIFEST_MAGIC ? sd.build_epoch : 0));
  if (progress) progress("OTA update found", appName, 0, ota.image_size, context);

  OtaAppExportResult exported = exportAppSlotToSd(tempPath, true, progress, context);
  if (exported.status != OtaAppExportStatus::Ok) {
    SD.remove(tempPath);
    return {OtaAppSyncStatus::Failed, exported.status, exported.bytesWritten};
  }
  OtaAppManifest verified;
  if (!miaReadManifestFromFile(tempPath, &verified) || verified.magic != ota.magic ||
      verified.build_epoch != ota.build_epoch || verified.image_crc != ota.image_crc) {
    SD.remove(tempPath);
    return {OtaAppSyncStatus::Failed, OtaAppExportStatus::VerifyFailed, exported.bytesWritten};
  }
  if (progress) progress("Verifying update", appName, ota.image_size, ota.image_size, context);

  const bool hadOldFile = SD.exists(sdPath);
  if (hadOldFile && !SD.rename(sdPath, backupPath)) {
    SD.remove(tempPath);
    return {OtaAppSyncStatus::Failed, OtaAppExportStatus::WriteFailed, exported.bytesWritten};
  }
  if (!SD.rename(tempPath, sdPath)) {
    if (hadOldFile) SD.rename(backupPath, sdPath);
    SD.remove(tempPath);
    return {OtaAppSyncStatus::Failed, OtaAppExportStatus::WriteFailed, exported.bytesWritten};
  }
  SD.remove(backupPath);
  if (progress) progress("Update complete", appName, ota.image_size, ota.image_size, context);
  launcherTracef("[ota-sync] updated %s bytes=%u", sdPath,
                 static_cast<unsigned>(exported.bytesWritten));
  return {OtaAppSyncStatus::Updated, OtaAppExportStatus::Ok, exported.bytesWritten};
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
    case OtaAppExportStatus::VerifyFailed:
      return "Verification failed";
  }
  return "Unknown";
}
