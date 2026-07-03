#include "launcher_log.h"

#include <SD.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <Arduino.h>

namespace {

constexpr char LOG_ROOT[] = "/MiaOS";
constexpr char LOG_DIR[] = "/MiaOS/logs";
constexpr char LOG_FILE[] = "/MiaOS/logs/latest.log";
constexpr char LOG_PROBE_FILE[] = "/MiaOS/logs/.probe";
constexpr uint8_t PENDING_LOG_LINE_COUNT = 16;
constexpr size_t PENDING_LOG_LINE_SIZE = 192;
bool g_logReady = false;
char g_pendingLogLines[PENDING_LOG_LINE_COUNT][PENDING_LOG_LINE_SIZE];
uint8_t g_pendingLogCount = 0;

bool ensureDir(const char *path) {
  File dir = SD.open(path);
  if (dir) {
    const bool isDirectory = dir.isDirectory();
    dir.close();
    if (isDirectory) {
      return true;
    }
  }
  return SD.mkdir(path);
}

bool ensureLogDir() {
  return ensureDir(LOG_ROOT) && ensureDir(LOG_DIR);
}

bool canCreateLogFile() {
  File file = SD.open(LOG_PROBE_FILE, FILE_WRITE);
  if (!file) {
    return false;
  }
  file.close();
  SD.remove(LOG_PROBE_FILE);
  return true;
}

void appendLine(const char *line) {
  if (!g_logReady || line == nullptr) {
    return;
  }

  File file = SD.open(LOG_FILE, FILE_WRITE);
  if (!file) {
    return;
  }
  file.seek(file.size());
  file.print(line);
  file.print('\n');
  file.close();
}

void appendPendingLine(const char *line) {
  if (line == nullptr || g_pendingLogCount >= PENDING_LOG_LINE_COUNT) {
    return;
  }
  strncpy(g_pendingLogLines[g_pendingLogCount], line, PENDING_LOG_LINE_SIZE - 1);
  g_pendingLogLines[g_pendingLogCount][PENDING_LOG_LINE_SIZE - 1] = '\0';
  ++g_pendingLogCount;
}

void flushPendingLines() {
  for (uint8_t index = 0; index < g_pendingLogCount; ++index) {
    appendLine(g_pendingLogLines[index]);
  }
  g_pendingLogCount = 0;
}

}

void launcherLogBeginSession(bool sdReady) {
  g_logReady = false;
  if (!sdReady) {
    return;
  }
  if (!ensureLogDir()) {
    return;
  }
  if (!canCreateLogFile()) {
    return;
  }

  SD.remove(LOG_FILE);
  File file = SD.open(LOG_FILE, FILE_WRITE);
  if (!file) {
    return;
  }
  file.close();
  g_logReady = true;
  appendLine("launcher log session start");
  flushPendingLines();
}

void launcherLogAppend(const char *message) {
  appendLine(message);
}

void launcherLogAppendf(const char *format, ...) {
  if (!g_logReady || format == nullptr) {
    return;
  }

  char buffer[192];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  appendLine(buffer);
}

void launcherTrace(const char *message) {
  if (message == nullptr) {
    return;
  }
  Serial.println(message);
  if (g_logReady) {
    appendLine(message);
  } else {
    appendPendingLine(message);
  }
}

void launcherTracef(const char *format, ...) {
  if (format == nullptr) {
    return;
  }

  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  size_t length = strlen(buffer);
  while (length > 0 && (buffer[length - 1] == '\n' || buffer[length - 1] == '\r')) {
    buffer[--length] = '\0';
  }
  Serial.println(buffer);
  if (g_logReady) {
    appendLine(buffer);
  } else {
    appendPendingLine(buffer);
  }
}

void launcherLogRecordSdRun(const SdAppManifestSummary &app, const SdAppLoaderResult &result) {
  launcherLogAppendf("sd app run category=%s name=%s path=%s", app.category, app.name,
                     app.path);
  launcherLogAppendf("sd app result status=%s error=%d", sdAppLoaderStatusText(result.status),
                     result.errorCode);
}

bool launcherLogRead(char *buffer, size_t bufferSize) {
  if (buffer == nullptr || bufferSize == 0) {
    return false;
  }

  File file = SD.open(LOG_FILE, FILE_READ);
  if (!file) {
    return false;
  }

  const size_t maxBytes = bufferSize - 1;
  const size_t fileSize = file.size();
  size_t bytesRead = 0;
  if (fileSize > maxBytes) {
    file.seek(fileSize - maxBytes);
    while (file.available()) {
      const int c = file.read();
      if (c < 0 || c == '\n') {
        break;
      }
    }
  }
  bytesRead = file.readBytes(buffer, maxBytes);
  buffer[bytesRead] = '\0';
  file.close();
  return true;
}

const char *launcherLogPath() {
  return LOG_FILE;
}
