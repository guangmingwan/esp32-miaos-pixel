#pragma once

#include <Arduino.h>

#include "sd_app_loader.h"

void launcherLogBeginSession(bool sdReady);
void launcherLogAppend(const char *message);
void launcherLogAppendf(const char *format, ...);
void launcherTrace(const char *message);
void launcherTracef(const char *format, ...);
void launcherLogRecordSdRun(const SdAppManifestSummary &app, const SdAppLoaderResult &result);
bool launcherLogRead(char *buffer, size_t bufferSize);
const char *launcherLogPath();
