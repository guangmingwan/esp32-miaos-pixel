#include "sd_app_loader.h"

#include <SD.h>
#include <stdio.h>
#include <string.h>

#include "mia_elf_runner.h"

static constexpr char APP_ROOT[] = "/MiaOS/Application";
static constexpr char APP_SUFFIX[] = ".app";
static constexpr char APP_ELF_NAME[] = "app.elf";

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

static bool formatElfPath(char *path, size_t pathSize, const char *appName) {
  const char *name = strrchr(appName, '/');
  name = name == nullptr ? appName : name + 1;
  const int written = snprintf(path, pathSize, "%s/%s/%s", APP_ROOT, name,
                               APP_ELF_NAME);
  return written >= 0 && static_cast<size_t>(written) < pathSize;
}

static const char *baseName(const char *path) {
  const char *name = strrchr(path, '/');
  return name == nullptr ? path : name + 1;
}

SdAppLoaderResult scanSdApps(SdAppManifestSummary *apps, uint8_t capacity,
                              bool sdReady) {
  if (!sdReady) {
    return {SdAppLoaderStatus::SdUnavailable, 0};
  }

  File root = SD.open(APP_ROOT);
  if (!root) {
    return {SdAppLoaderStatus::NoAppsFound, 0};
  }
  if (!root.isDirectory()) {
    root.close();
    return {SdAppLoaderStatus::ReadError, 0};
  }

  uint8_t found = 0;
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }

    const char *entryName = entry.name();
    const bool candidate = entry.isDirectory() && endsWith(entryName, APP_SUFFIX);
    char elfPath[sizeof(SdAppManifestSummary::path)] = {};
    const bool pathOk = candidate && formatElfPath(elfPath, sizeof(elfPath), entryName);
    entry.close();

    if (!pathOk) {
      continue;
    }

    File elf = SD.open(elfPath, FILE_READ);
    if (!elf) {
      continue;
    }
    elf.close();

    if (apps != nullptr && found < capacity) {
      copyText(apps[found].name, sizeof(apps[found].name), baseName(entryName));
      copyText(apps[found].path, sizeof(apps[found].path), elfPath);
    }
    ++found;
  }

  root.close();
  return {found > 0 ? SdAppLoaderStatus::Ok : SdAppLoaderStatus::NoAppsFound,
          found};
}

SdAppLoaderResult runSdAppByPath(const char *path, bool sdReady) {
  MiaElfRunResult result = miaRunElfApp(path, sdReady);
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
