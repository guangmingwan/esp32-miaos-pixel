#include "sd_app_loader.h"

#include <SD.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mia_elf_runner.h"

static constexpr const char *APP_ROOTS[] = {
    "/Games",
    "/Utils",
    "/Settings",
    "/Emulators",
    "/Media",
    "/Application",
    "/MiaOS/Application",
};
static constexpr char APP_SUFFIX[] = ".app";
static constexpr char APP_ELF_NAME[] = "app.elf";
static constexpr uint16_t MAX_SCAN_ENTRIES = 128;

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

static void copyAppDisplayName(char *dest, size_t destSize, const char *rootPath,
                               const char *appPath) {
  if (destSize == 0) {
    return;
  }

  const char *name = baseName(appPath);
  size_t nameLen = strlen(name);
  const size_t suffixLen = strlen(APP_SUFFIX);
  if (nameLen >= suffixLen && strcmp(name + nameLen - suffixLen, APP_SUFFIX) == 0) {
    nameLen -= suffixLen;
  }

  const char *category = baseName(rootPath);
  const int written = snprintf(dest, destSize, "%s/%.*s", category,
                               static_cast<int>(nameLen), name);
  if (written < 0 || static_cast<size_t>(written) >= destSize) {
    dest[destSize - 1] = '\0';
  }
}

static bool formatElfPath(char *path, size_t pathSize, const char *rootPath,
                          const char *appName) {
  const char *name = strrchr(appName, '/');
  name = name == nullptr ? appName : name + 1;
  const int written = snprintf(path, pathSize, "%s/%s/%s", rootPath, name,
                               APP_ELF_NAME);
  return written >= 0 && static_cast<size_t>(written) < pathSize;
}

static void scanAppRoot(const char *rootPath, SdAppManifestSummary *apps,
                        uint8_t capacity, uint8_t &found, uint16_t &scanned) {
  File root = SD.open(rootPath);
  if (!root) {
    Serial.printf("[sd-scan] app root missing/unreadable: %s\n", rootPath);
    return;
  }
  if (!root.isDirectory()) {
    Serial.printf("[sd-scan] app root is not a directory: %s\n", rootPath);
    root.close();
    return;
  }

  while (scanned < MAX_SCAN_ENTRIES) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }
    ++scanned;

    const char *entryName = entry.name();
    const char *name = baseName(entryName);
    if (entry.isDirectory() && endsWith(name, APP_SUFFIX)) {
      char elfPath[128];
      if (formatElfPath(elfPath, sizeof(elfPath), rootPath, entryName)) {
        File elf = SD.open(elfPath, FILE_READ);
        if (elf) {
          Serial.printf("[sd-scan] app found: %s size=%u\n", elfPath,
                        static_cast<unsigned>(elf.size()));
          elf.close();
          if (apps != nullptr && found < capacity) {
            copyAppDisplayName(apps[found].name, sizeof(apps[found].name), rootPath,
                               entryName);
            copyText(apps[found].path, sizeof(apps[found].path), elfPath);
            Serial.printf("[sd-scan] stored app[%u] name='%s' path='%s'\n",
                          static_cast<unsigned>(found), apps[found].name,
                          apps[found].path);
          }
          if (found < UINT8_MAX) {
            ++found;
          }
        } else {
          Serial.printf("[sd-scan] skip missing elf: %s\n", elfPath);
        }
      }
    }
    entry.close();
  }
  root.close();
}

SdAppLoaderResult scanSdApps(SdAppManifestSummary *apps, uint8_t capacity,
                                bool sdReady) {
  Serial.printf("[sd-scan] start sdReady=%d capacity=%u\n", sdReady ? 1 : 0,
                static_cast<unsigned>(capacity));
  if (!sdReady) {
    Serial.println("[sd-scan] skip: SD unavailable");
    return {SdAppLoaderStatus::SdUnavailable, 0};
  }

  uint16_t scanned = 0;
  uint8_t found = 0;
  for (size_t i = 0; i < sizeof(APP_ROOTS) / sizeof(APP_ROOTS[0]) &&
                     scanned < MAX_SCAN_ENTRIES;
       ++i) {
    scanAppRoot(APP_ROOTS[i], apps, capacity, found, scanned);
  }

  if (found > capacity) {
    Serial.printf("[sd-scan] found=%u exceeds menu capacity=%u\n",
                  static_cast<unsigned>(found), static_cast<unsigned>(capacity));
    found = capacity;
  }
  Serial.printf("[sd-scan] done scanned=%u found=%u status=%s\n",
                static_cast<unsigned>(scanned), static_cast<unsigned>(found),
                found > 0 ? "ok" : "none");
  return {found > 0 ? SdAppLoaderStatus::Ok : SdAppLoaderStatus::NoAppsFound,
          found};
}

SdAppLoaderResult runSdAppByPath(const char *path, bool sdReady) {
  Serial.printf("[sd-run] path='%s' sdReady=%d\n", path == nullptr ? "<null>" : path,
                sdReady ? 1 : 0);
  MiaElfRunResult result = miaRunElfApp(path, sdReady);
  Serial.printf("[sd-run] result status=%s code=%d\n", miaElfRunStatusText(result.status),
                result.errorCode);
  switch (result.status) {
    case MiaElfRunStatus::Ok:
      return {SdAppLoaderStatus::Ok, 1};
    case MiaElfRunStatus::SdUnavailable:
      return {SdAppLoaderStatus::SdUnavailable, 0};
    case MiaElfRunStatus::ReadError:
      return {SdAppLoaderStatus::ReadError, 0};
    case MiaElfRunStatus::RunError:
      return {SdAppLoaderStatus::RunError, 0};
  }
  return {SdAppLoaderStatus::RunError, 0};
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
