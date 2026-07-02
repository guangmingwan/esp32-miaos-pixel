#include "sd_app_loader.h"

#include <SD.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mia_elf_runner.h"

static constexpr char APP_ROOT[] = "/MiaOS/Application";
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

static void copyAppDisplayName(char *dest, size_t destSize, const char *appPath) {
  if (destSize == 0) {
    return;
  }

  const char *name = baseName(appPath);
  size_t nameLen = strlen(name);
  const size_t suffixLen = strlen(APP_SUFFIX);
  if (nameLen >= suffixLen && strcmp(name + nameLen - suffixLen, APP_SUFFIX) == 0) {
    nameLen -= suffixLen;
  }

  const size_t copyLen = nameLen < destSize - 1 ? nameLen : destSize - 1;
  memcpy(dest, name, copyLen);
  dest[copyLen] = '\0';
}

static bool formatElfPath(char *path, size_t pathSize, const char *appName) {
  const char *name = strrchr(appName, '/');
  name = name == nullptr ? appName : name + 1;
  const int written = snprintf(path, pathSize, "%s/%s/%s", APP_ROOT, name,
                               APP_ELF_NAME);
  return written >= 0 && static_cast<size_t>(written) < pathSize;
}

SdAppLoaderResult scanSdApps(SdAppManifestSummary *apps, uint8_t capacity,
                              bool sdReady) {
  Serial.printf("[sd-scan] start sdReady=%d capacity=%u root=%s\n", sdReady ? 1 : 0,
                static_cast<unsigned>(capacity), APP_ROOT);
  if (!sdReady) {
    Serial.println("[sd-scan] skip: SD unavailable");
    return {SdAppLoaderStatus::SdUnavailable, 0};
  }

  static constexpr char DIRECT_APP_PATH[] = "/MiaOS/Application/mia_test.app/app.elf";
  File elf = SD.open(DIRECT_APP_PATH, FILE_READ);
  if (!elf) {
    Serial.printf("[sd-scan] direct app missing/unreadable: %s\n", DIRECT_APP_PATH);
    return {SdAppLoaderStatus::NoAppsFound, 0};
  }
  Serial.printf("[sd-scan] direct app found: %s size=%u\n", DIRECT_APP_PATH,
                static_cast<unsigned>(elf.size()));
  elf.close();

  if (apps != nullptr && capacity > 0) {
    copyText(apps[0].name, sizeof(apps[0].name), "mia_test");
    copyText(apps[0].path, sizeof(apps[0].path), DIRECT_APP_PATH);
    Serial.printf("[sd-scan] stored app[0] name='%s' path='%s'\n",
                  apps[0].name, apps[0].path);
  }
  const uint8_t found = 1;
  Serial.println("[sd-scan] done scanned=direct found=1 status=ok");
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
