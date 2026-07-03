#include "serial_file_service.h"

#include <SD.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "launcher_log.h"
#include "wifi_file_paths.h"

namespace {

constexpr uint16_t COMMAND_BUFFER_SIZE = 160;
constexpr size_t UPLOAD_CHUNK_SIZE = 1024;
constexpr uint32_t UPLOAD_TIMEOUT_MS = 15000;

char g_commandBuffer[COMMAND_BUFFER_SIZE];
uint16_t g_commandLength = 0;
bool g_commandOverflow = false;
char g_status[48] = "Idle";
char g_targetPath[96] = "/";
bool g_sdReady = false;
bool g_uploadActive = false;
uint32_t g_expectedBytes = 0;
uint32_t g_receivedBytes = 0;
uint32_t g_lastUploadActivityMs = 0;
File g_uploadFile;

void copyText(char *dest, size_t destSize, const char *src) {
  if (destSize == 0) {
    return;
  }
  strncpy(dest, src, destSize - 1);
  dest[destSize - 1] = '\0';
}

void setStatus(const char *status) {
  copyText(g_status, sizeof(g_status), status);
}

void sendLine(const char *line) {
  Serial.print(line);
  Serial.print('\n');
}

void sendOk(const char *message) {
  char line[96];
  snprintf(line, sizeof(line), "OK %s", message);
  sendLine(line);
}

void sendError(const char *message) {
  char line[96];
  snprintf(line, sizeof(line), "ERR %s", message);
  sendLine(line);
}

bool parsePathArgument(const char *raw, String &normalizedPath) {
  if (raw == nullptr || raw[0] == '\0') {
    return false;
  }
  String path = raw;
  return normalizeSdPath(path, normalizedPath);
}

bool ensureSdReady() {
  if (g_sdReady) {
    return true;
  }
  sendError("sd unavailable");
  setStatus("SD unavailable");
  return false;
}

void discardPartialUploadFile() {
  if (g_targetPath[0] != '\0' && strcmp(g_targetPath, "/") != 0) {
    SD.remove(g_targetPath);
  }
}

void finishUpload(const char *status, const char *response, const char *logPrefix,
                  bool removePartial) {
  if (g_uploadFile) {
    g_uploadFile.close();
  }
  if (removePartial) {
    discardPartialUploadFile();
  }
  g_uploadActive = false;
  setStatus(status);
  if (response != nullptr) {
    sendError(response);
  }
  launcherLogAppendf("serial put %s path=%s wrote=%lu expected=%lu", logPrefix, g_targetPath,
                     static_cast<unsigned long>(g_receivedBytes),
                     static_cast<unsigned long>(g_expectedBytes));
}

bool startUpload(const char *rawPath, const char *rawSize) {
  if (!ensureSdReady()) {
    return false;
  }

  String normalizedPath;
  if (!parsePathArgument(rawPath, normalizedPath)) {
    sendError("invalid path");
    return false;
  }
  if (rawSize == nullptr || rawSize[0] == '\0') {
    sendError("missing size");
    return false;
  }

  char *end = nullptr;
  const unsigned long parsedSize = strtoul(rawSize, &end, 10);
  if (end == rawSize || *end != '\0') {
    sendError("invalid size");
    return false;
  }

  SD.remove(normalizedPath);

  g_uploadFile = SD.open(normalizedPath, FILE_WRITE);
  if (!g_uploadFile) {
    sendError("open failed");
    return false;
  }

  g_uploadFile.seek(0);
  g_expectedBytes = static_cast<uint32_t>(parsedSize);
  g_receivedBytes = 0;
  g_uploadActive = true;
  g_lastUploadActivityMs = millis();
  copyText(g_targetPath, sizeof(g_targetPath), normalizedPath.c_str());
  setStatus("Receiving file");
  launcherLogAppendf("serial put start path=%s size=%lu", normalizedPath.c_str(), parsedSize);
  sendLine("READY");
  return true;
}

void handleList(const char *rawPath) {
  if (!ensureSdReady()) {
    return;
  }

  String normalizedPath;
  if (!parsePathArgument(rawPath == nullptr ? "/" : rawPath, normalizedPath)) {
    sendError("invalid path");
    return;
  }

  File dir = SD.open(normalizedPath);
  if (!dir || !dir.isDirectory()) {
    if (dir) {
      dir.close();
    }
    sendError("not directory");
    return;
  }

  sendLine("OK LIST");
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }
    char line[128];
    const char *name = strrchr(entry.name(), '/');
    name = name == nullptr ? entry.name() : name + 1;
    snprintf(line, sizeof(line), "ITEM %c %lu %s", entry.isDirectory() ? 'D' : 'F',
             static_cast<unsigned long>(entry.isDirectory() ? 0 : entry.size()), name);
    sendLine(line);
    entry.close();
  }
  dir.close();
  sendLine("END");
  setStatus("Listed directory");
  copyText(g_targetPath, sizeof(g_targetPath), normalizedPath.c_str());
  launcherLogAppendf("serial list path=%s", normalizedPath.c_str());
}

void handleMkdir(const char *rawPath) {
  if (!ensureSdReady()) {
    return;
  }

  String normalizedPath;
  if (!parsePathArgument(rawPath, normalizedPath)) {
    sendError("invalid path");
    return;
  }
  if (!SD.mkdir(normalizedPath)) {
    sendError("mkdir failed");
    return;
  }
  sendOk("mkdir");
  setStatus("Created directory");
  copyText(g_targetPath, sizeof(g_targetPath), normalizedPath.c_str());
  launcherLogAppendf("serial mkdir path=%s", normalizedPath.c_str());
}

void handleDelete(const char *rawPath) {
  if (!ensureSdReady()) {
    return;
  }

  String normalizedPath;
  if (!parsePathArgument(rawPath, normalizedPath) || normalizedPath == "/") {
    sendError("invalid path");
    return;
  }

  File target = SD.open(normalizedPath);
  if (!target) {
    sendError("not found");
    return;
  }
  const bool isDirectory = target.isDirectory();
  target.close();

  const bool removed = isDirectory ? SD.rmdir(normalizedPath) : SD.remove(normalizedPath);
  if (!removed) {
    sendError("delete failed");
    return;
  }
  sendOk("delete");
  setStatus("Deleted path");
  copyText(g_targetPath, sizeof(g_targetPath), normalizedPath.c_str());
  launcherLogAppendf("serial delete path=%s", normalizedPath.c_str());
}

void processCommand() {
  g_commandBuffer[g_commandLength] = '\0';
  if (g_commandLength == 0) {
    return;
  }

  char *command = strtok(g_commandBuffer, " ");
  char *arg1 = strtok(nullptr, "");
  if (command == nullptr) {
    return;
  }

  if (strcmp(command, "PING") == 0) {
    sendOk("PONG");
    setStatus("Pinged");
    return;
  }
  if (strcmp(command, "HELP") == 0) {
    sendLine("OK COMMANDS PING LIST MKDIR DELETE PUT HELP");
    setStatus("Help shown");
    return;
  }
  if (strcmp(command, "LIST") == 0) {
    handleList(arg1 == nullptr ? "/" : arg1);
    return;
  }
  if (strcmp(command, "MKDIR") == 0) {
    handleMkdir(arg1);
    return;
  }
  if (strcmp(command, "DELETE") == 0) {
    handleDelete(arg1);
    return;
  }
  if (strcmp(command, "PUT") == 0) {
    if (arg1 == nullptr) {
      sendError("missing args");
      return;
    }
    char *lastSpace = strrchr(arg1, ' ');
    if (lastSpace == nullptr) {
      sendError("missing size");
      return;
    }
    *lastSpace = '\0';
    const char *rawSize = lastSpace + 1;
    startUpload(arg1, rawSize);
    return;
  }

  sendError("unknown command");
}

void pollCommandLine() {
  while (Serial.available() > 0 && !g_uploadActive) {
    const int value = Serial.read();
    if (value < 0) {
      break;
    }
    const char ch = static_cast<char>(value);
    if (ch == '\r') {
      continue;
    }
    if (ch == '\n') {
      if (g_commandOverflow) {
        sendError("line too long");
        setStatus("Rejected long line");
      } else {
        processCommand();
      }
      g_commandLength = 0;
      g_commandOverflow = false;
      continue;
    }
    if (g_commandLength + 1 < COMMAND_BUFFER_SIZE) {
      g_commandBuffer[g_commandLength++] = ch;
    } else {
      g_commandOverflow = true;
    }
  }
}

void pollUpload() {
  if (!g_uploadActive) {
    return;
  }

  if (millis() - g_lastUploadActivityMs > UPLOAD_TIMEOUT_MS) {
    finishUpload("Upload timeout", "upload timeout", "timeout", true);
    return;
  }

  while (Serial.available() > 0 && g_receivedBytes < g_expectedBytes) {
    uint8_t chunk[UPLOAD_CHUNK_SIZE];
    size_t readCount = 0;
    const size_t want = min<size_t>(sizeof(chunk), g_expectedBytes - g_receivedBytes);
    while (readCount < want && Serial.available() > 0) {
      const int value = Serial.read();
      if (value < 0) {
        break;
      }
      chunk[readCount++] = static_cast<uint8_t>(value);
    }
    if (readCount == 0) {
      break;
    }
    const size_t written = g_uploadFile.write(chunk, readCount);
    g_receivedBytes += static_cast<uint32_t>(written);
    g_lastUploadActivityMs = millis();
    if (written != readCount) {
      finishUpload("Write failed", "write failed", "failed", true);
      return;
    }
  }

  if (g_receivedBytes >= g_expectedBytes) {
    g_uploadFile.close();

    File verifyFile = SD.open(g_targetPath, FILE_READ);
    const bool verified = verifyFile && !verifyFile.isDirectory() &&
                          static_cast<uint32_t>(verifyFile.size()) == g_expectedBytes;
    if (verifyFile) {
      verifyFile.close();
    }
    if (!verified) {
      finishUpload("Verify failed", "verify failed", "verify-failed", true);
      return;
    }

    g_uploadActive = false;
    setStatus("Upload complete");
    sendOk("stored");
    launcherLogAppendf("serial put complete path=%s size=%lu", g_targetPath,
                       static_cast<unsigned long>(g_expectedBytes));
  }
}

}

void serialFileServiceBegin(bool sdReady) {
  g_sdReady = sdReady;
  g_commandLength = 0;
  g_commandOverflow = false;
  g_uploadActive = false;
  g_expectedBytes = 0;
  g_receivedBytes = 0;
  g_lastUploadActivityMs = 0;
  copyText(g_targetPath, sizeof(g_targetPath), "/");
  setStatus(sdReady ? "Waiting for host" : "SD unavailable");
  sendLine("SFS1 READY");
  launcherLogAppendf("serial service begin sd_ready=%d", sdReady ? 1 : 0);
}

void serialFileServiceTick() {
  pollUpload();
  pollCommandLine();
}

void serialFileServiceEnd() {
  if (g_uploadActive) {
    finishUpload("Stopped", nullptr, "aborted", true);
  }
  g_uploadActive = false;
  setStatus("Stopped");
  launcherLogAppend("serial service end");
}

SerialFileServiceSnapshot serialFileServiceSnapshot() {
  SerialFileServiceSnapshot snapshot = {};
  snapshot.sdReady = g_sdReady;
  snapshot.uploadActive = g_uploadActive;
  snapshot.receivedBytes = g_receivedBytes;
  snapshot.totalBytes = g_expectedBytes;
  copyText(snapshot.status, sizeof(snapshot.status), g_status);
  copyText(snapshot.targetPath, sizeof(snapshot.targetPath), g_targetPath);
  return snapshot;
}
