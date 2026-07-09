#include "sd_app_loader.h"

#include <Arduino.h>
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "launcher_log.h"
#include "ota_app_flash.h"

static constexpr char SD_VFS_ROOT[] = "/sd";
static constexpr char MIAOS_ROOT[] = "/MiaOS";
static constexpr char APP_SUFFIX[] = ".app";
static constexpr char APP_FIRMWARE_EXT[] = ".bin";
static constexpr uint16_t MAX_SCAN_ENTRIES = 128;

static void scanYield() { delay(1); }

static bool endsWith(const char *value, const char *suffix) {
  const size_t valueLen = strlen(value);
  const size_t suffixLen = strlen(suffix);
  return valueLen >= suffixLen && strcmp(value + valueLen - suffixLen, suffix) == 0;
}

static void copyText(char *dest, size_t destSize, const char *src) {
  if (destSize == 0) {
    return;
  }
  strncpy(dest, src, destSize - 1);
  dest[destSize - 1] = '\0';
}

static const char *baseName(const char *path) {
  const char *name = strrchr(path, '/');
  return name == nullptr ? path : name + 1;
}

static void copyAppName(char *dest, size_t destSize, const char *appPath) {
  if (destSize == 0) {
    return;
  }

  const char *name = baseName(appPath);
  size_t nameLen = strlen(name);
  const size_t suffixLen = strlen(APP_SUFFIX);
  if (nameLen >= suffixLen && strcmp(name + nameLen - suffixLen, APP_SUFFIX) == 0) {
    nameLen -= suffixLen;
  }

  const int written = snprintf(dest, destSize, "%.*s", static_cast<int>(nameLen), name);
  if (written < 0 || static_cast<size_t>(written) >= destSize) {
    dest[destSize - 1] = '\0';
  }
}

static void copyAppCategory(char *dest, size_t destSize, const char *rootPath) {
  if (destSize == 0) {
    return;
  }

  const char *category = baseName(rootPath);
  const int written = snprintf(dest, destSize, "%s", category);
  if (written < 0 || static_cast<size_t>(written) >= destSize) {
    dest[destSize - 1] = '\0';
  }
}

static bool formatElfPath(char *path, size_t pathSize, const char *rootPath,
                           const char *appName) {
  const char *name = strrchr(appName, '/');
  name = name == nullptr ? appName : name + 1;

  // Strip .app suffix to get the firmware filename base
  size_t nameLen = strlen(name);
  const size_t suffixLen = strlen(APP_SUFFIX);
  if (nameLen >= suffixLen && strcmp(name + nameLen - suffixLen, APP_SUFFIX) == 0) {
    nameLen -= suffixLen;
  }

  const int written = snprintf(path, pathSize, "%s/%s/%.*s%s", rootPath, name,
                                static_cast<int>(nameLen), name, APP_FIRMWARE_EXT);
  return written >= 0 && static_cast<size_t>(written) < pathSize;
}

static bool formatVfsPath(char *path, size_t pathSize, const char *sdPath) {
  const char *separator = sdPath[0] == '/' ? "" : "/";
  const int written = snprintf(path, pathSize, "%s%s%s", SD_VFS_ROOT, separator, sdPath);
  return written >= 0 && static_cast<size_t>(written) < pathSize;
}

static bool statPath(const char *sdPath, struct stat *info) {
  char vfsPath[160];
  return formatVfsPath(vfsPath, sizeof(vfsPath), sdPath) && stat(vfsPath, info) == 0;
}

static bool isDirectoryPath(const char *sdPath) {
  struct stat info;
  return statPath(sdPath, &info) && S_ISDIR(info.st_mode);
}

static bool fileSize(const char *sdPath, size_t *size) {
  struct stat info;
  if (!statPath(sdPath, &info) || !S_ISREG(info.st_mode)) {
    return false;
  }
  *size = static_cast<size_t>(info.st_size);
  return true;
}

static void scanAppRoot(const char *rootPath, SdAppManifestSummary *apps,
                        uint8_t capacity, uint8_t &found, uint16_t &scanned) {
  scanYield();
  launcherTracef("[sd-scan] root: %s", rootPath);
  if (!isDirectoryPath(rootPath)) {
    launcherTracef("[sd-scan] app root is not a directory: %s", rootPath);
    return;
  }

  char vfsRootPath[160];
  if (!formatVfsPath(vfsRootPath, sizeof(vfsRootPath), rootPath)) {
    launcherTracef("[sd-scan] app root path too long: %s", rootPath);
    return;
  }

  scanYield();
  DIR *root = opendir(vfsRootPath);
  if (root == nullptr) {
    launcherTracef("[sd-scan] app root missing/unreadable: %s", rootPath);
    return;
  }

  while (scanned < MAX_SCAN_ENTRIES) {
    scanYield();
    struct dirent *entry = readdir(root);
    if (entry == nullptr) {
      break;
    }
    ++scanned;

    const char *entryName = entry->d_name;
    if (strcmp(entryName, ".") == 0 || strcmp(entryName, "..") == 0) {
      continue;
    }

    char appPath[128];
    const int appPathWritten = snprintf(appPath, sizeof(appPath), "%s/%s", rootPath, entryName);
    if (appPathWritten < 0 || static_cast<size_t>(appPathWritten) >= sizeof(appPath)) {
      continue;
    }

    if (isDirectoryPath(appPath) && endsWith(entryName, APP_SUFFIX)) {
      char elfPath[128];
      if (formatElfPath(elfPath, sizeof(elfPath), rootPath, entryName)) {
        scanYield();
        size_t elfSize = 0;
        if (fileSize(elfPath, &elfSize)) {
          launcherTracef("[sd-scan] app found: %s size=%u", elfPath,
                         static_cast<unsigned>(elfSize));
          if (apps != nullptr && found < capacity) {
            copyAppName(apps[found].name, sizeof(apps[found].name), entryName);
            copyAppCategory(apps[found].category, sizeof(apps[found].category), rootPath);
            copyText(apps[found].path, sizeof(apps[found].path), elfPath);
            launcherTracef("[sd-scan] stored app[%u] category='%s' name='%s' path='%s'",
                           static_cast<unsigned>(found), apps[found].category,
                           apps[found].name, apps[found].path);
          }
          if (found < UINT8_MAX) {
            ++found;
          }
        } else {
          launcherTracef("[sd-scan] skip missing elf: %s", elfPath);
        }
      }
    }
    scanYield();
  }
  closedir(root);
  scanYield();
}

SdAppLoaderResult scanSdApps(SdAppManifestSummary *apps, uint8_t capacity,
                                 bool sdReady) {
  launcherTracef("[sd-scan] start sdReady=%d capacity=%u", sdReady ? 1 : 0,
                 static_cast<unsigned>(capacity));
  if (!sdReady) {
    launcherTrace("[sd-scan] skip: SD unavailable");
    return {SdAppLoaderStatus::SdUnavailable, 0, 0};
  }

  uint16_t scanned = 0;
  uint8_t found = 0;

  // Dynamically list subdirectories under /MiaOS/ as category roots.
  char vfsMiaos[64];
  formatVfsPath(vfsMiaos, sizeof(vfsMiaos), MIAOS_ROOT);
  DIR *miaosDir = opendir(vfsMiaos);
  if (miaosDir != nullptr) {
    while (true) {
      scanYield();
      struct dirent *entry = readdir(miaosDir);
      if (entry == nullptr) break;

      const char *name = entry->d_name;
      if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

      char rootPath[64];
      const int rpLen = snprintf(rootPath, sizeof(rootPath), "%s/%s", MIAOS_ROOT, name);
      if (rpLen < 0 || static_cast<size_t>(rpLen) >= sizeof(rootPath)) continue;

      if (isDirectoryPath(rootPath)) {
        scanAppRoot(rootPath, apps, capacity, found, scanned);
      }
    }
    closedir(miaosDir);
  } else {
    launcherTracef("[sd-scan] missing miaos root: %s", vfsMiaos);
  }

  if (found > capacity) {
    launcherTracef("[sd-scan] found=%u exceeds menu capacity=%u",
                   static_cast<unsigned>(found), static_cast<unsigned>(capacity));
    found = capacity;
  }
  launcherTracef("[sd-scan] done scanned=%u found=%u status=%s",
                 static_cast<unsigned>(scanned), static_cast<unsigned>(found),
                 found > 0 ? "ok" : "none");
  return {found > 0 ? SdAppLoaderStatus::Ok : SdAppLoaderStatus::NoAppsFound,
          found, 0};
}

SdAppLoaderResult runSdAppByPath(const char *path, bool sdReady) {
  launcherTracef("[sd-run] path='%s' sdReady=%d", path == nullptr ? "<null>" : path,
                 sdReady ? 1 : 0);
  OtaAppFlashResult result = miaFlashAppToSlot(path, sdReady);
  launcherTracef("[sd-run] result status=%s code=%d",
                 miaOtaAppFlashStatusText(result.status), result.errorCode);
  switch (result.status) {
    case OtaAppFlashStatus::Ok:
      return {SdAppLoaderStatus::Ok, 1, result.errorCode};
    case OtaAppFlashStatus::SdUnavailable:
      return {SdAppLoaderStatus::SdUnavailable, 0, result.errorCode};
    case OtaAppFlashStatus::InvalidPath:
    case OtaAppFlashStatus::OpenFailed:
    case OtaAppFlashStatus::EmptyFile:
    case OtaAppFlashStatus::TooLarge:
    case OtaAppFlashStatus::ReadFailed:
      return {SdAppLoaderStatus::ReadError, 0, result.errorCode};
    case OtaAppFlashStatus::NoPartition:
    case OtaAppFlashStatus::EraseFailed:
    case OtaAppFlashStatus::WriteFailed:
    case OtaAppFlashStatus::AllocFailed:
    case OtaAppFlashStatus::OtaDataFailed:
      return {SdAppLoaderStatus::RunError, 0, result.errorCode};
  }
  return {SdAppLoaderStatus::RunError, 0, result.errorCode};
}

SdAppLoaderResult exportSdAppByPath(const char *path, bool sdReady) {
  OtaAppExportResult result = miaExportAppSlotToSd(path, sdReady);
  launcherTracef("[sd-export] path='%s' status=%s code=%d",
                 path == nullptr ? "<null>" : path,
                 miaOtaAppExportStatusText(result.status), result.errorCode);
  switch (result.status) {
    case OtaAppExportStatus::Ok:
      return {SdAppLoaderStatus::Ok, 1, result.errorCode};
    case OtaAppExportStatus::SdUnavailable:
      return {SdAppLoaderStatus::SdUnavailable, 0, result.errorCode};
    case OtaAppExportStatus::InvalidPath:
    case OtaAppExportStatus::OpenFailed:
    case OtaAppExportStatus::WriteFailed:
      return {SdAppLoaderStatus::ReadError, 0, result.errorCode};
    case OtaAppExportStatus::NoPartition:
    case OtaAppExportStatus::InvalidImage:
    case OtaAppExportStatus::ReadFailed:
    case OtaAppExportStatus::ManifestMissing:
    case OtaAppExportStatus::MkdirFailed:
      return {SdAppLoaderStatus::RunError, 0, result.errorCode};
  }
  return {SdAppLoaderStatus::RunError, 0, result.errorCode};
}

bool sdManifestMatchesOta(const char *sdPath) {
  OtaAppManifest sdManifest;
  if (!miaReadManifestFromFile(sdPath, &sdManifest)) {
    return false;
  }
  OtaAppManifest slotManifest;
  if (!miaReadOtaManifest(&slotManifest)) {
    return false;
  }
  if (sdManifest.magic != slotManifest.magic) return false;
  if (memcmp(sdManifest.category, slotManifest.category, MIA_MANIFEST_CATEGORY_SIZE) != 0) return false;
  if (memcmp(sdManifest.name, slotManifest.name, MIA_MANIFEST_NAME_SIZE) != 0) return false;
  return sdManifest.crc == slotManifest.crc;
}

const char *sdAppLoaderStatusText(SdAppLoaderStatus status) {
  switch (status) {
    case SdAppLoaderStatus::Ok:
      return "OK";
    case SdAppLoaderStatus::SdUnavailable:
      return "card unavailable";
    case SdAppLoaderStatus::NoAppsFound:
      return "none";
    case SdAppLoaderStatus::ReadError:
      return "read error";
    case SdAppLoaderStatus::RunError:
      return "run error";
  }
  return "unknown";
}
