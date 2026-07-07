#pragma once

#include <Arduino.h>

enum class SdAppLoaderStatus : uint8_t {
  Ok,
  SdUnavailable,
  NoAppsFound,
  ReadError,
  RunError,
};

struct SdAppManifestSummary {
  char name[32];
  char category[16];
  char path[128];
};

struct SdAppLoaderResult {
  SdAppLoaderStatus status;
  uint8_t appCount;
  int errorCode;
};

SdAppLoaderResult scanSdApps(SdAppManifestSummary *apps, uint8_t capacity,
                              bool sdReady);
SdAppLoaderResult runSdAppByPath(const char *path, bool sdReady);
SdAppLoaderResult exportSdAppByPath(const char *path, bool sdReady);
const char *sdAppLoaderStatusText(SdAppLoaderStatus status);

/* Returns true if the SD firmware file's manifest matches ota_1's manifest. */
bool sdManifestMatchesOta(const char *sdPath);
