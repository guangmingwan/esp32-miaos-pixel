#include "serial_file_service.h"

#include <SD.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "HWCDC.h"
#include "launcher_log.h"
#include "wifi_file_paths.h"

namespace {

constexpr uint16_t COMMAND_BUFFER_SIZE = 160;
constexpr uint16_t CONTROL_BUFFER_SIZE = 32;
constexpr size_t VCP_WINDOW_SIZE = 6144;
constexpr size_t DOWNLOAD_CHUNK_SIZE = VCP_WINDOW_SIZE;
constexpr uint32_t UPLOAD_TIMEOUT_MS = 15000;
constexpr uint32_t DOWNLOAD_TIMEOUT_MS = 15000;

char g_commandBuffer[COMMAND_BUFFER_SIZE];
uint16_t g_commandLength = 0;
bool g_commandOverflow = false;
char g_controlBuffer[CONTROL_BUFFER_SIZE];
uint16_t g_controlLength = 0;
bool g_controlOverflow = false;
bool g_enterRequested = false;
bool g_exitRequested = false;
char g_status[48] = "Idle";
char g_targetPath[96] = "/";
bool g_sdReady = false;
bool g_uploadActive = false;
uint32_t g_expectedBytes = 0;
uint32_t g_receivedBytes = 0;
uint32_t g_nextUploadAckBytes = 0;
uint32_t g_lastUploadActivityMs = 0;
uint8_t g_uploadChunk[VCP_WINDOW_SIZE];
size_t g_uploadChunkLength = 0;
File g_uploadFile;
bool g_downloadActive = false;
uint32_t g_downloadBytes = 0;
uint32_t g_sentBytes = 0;
uint32_t g_lastDownloadActivityMs = 0;
File g_downloadFile;
TaskHandle_t g_serviceTask = nullptr;
volatile bool g_serviceStopRequested = false;
portMUX_TYPE g_serviceStateMux = portMUX_INITIALIZER_UNLOCKED;
SerialFileServiceSnapshot g_publishedSnapshot = {};

HWCDC &vcp() { return Serial; }
void copyText(char *dest, size_t destSize, const char *src);

void publishSnapshot() {
  SerialFileServiceSnapshot snapshot = {};
  snapshot.sdReady = g_sdReady;
  snapshot.uploadActive = g_uploadActive;
  snapshot.downloadActive = g_downloadActive;
  snapshot.transferredBytes = g_downloadActive ? g_sentBytes : g_receivedBytes;
  snapshot.totalBytes = g_downloadActive ? g_downloadBytes : g_expectedBytes;
  copyText(snapshot.status, sizeof(snapshot.status), g_status);
  copyText(snapshot.targetPath, sizeof(snapshot.targetPath), g_targetPath);

  portENTER_CRITICAL(&g_serviceStateMux);
  g_publishedSnapshot = snapshot;
  portEXIT_CRITICAL(&g_serviceStateMux);
}

void setServiceTask(TaskHandle_t task) {
  portENTER_CRITICAL(&g_serviceStateMux);
  g_serviceTask = task;
  portEXIT_CRITICAL(&g_serviceStateMux);
}

TaskHandle_t serviceTask() {
  portENTER_CRITICAL(&g_serviceStateMux);
  TaskHandle_t task = g_serviceTask;
  portEXIT_CRITICAL(&g_serviceStateMux);
  return task;
}

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
  vcp().print(line);
  vcp().print('\n');
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
  g_uploadChunkLength = 0;
  g_nextUploadAckBytes = min<uint32_t>(VCP_WINDOW_SIZE, g_expectedBytes);
  g_uploadActive = true;
  g_lastUploadActivityMs = millis();
  copyText(g_targetPath, sizeof(g_targetPath), normalizedPath.c_str());
  setStatus("Receiving file");
  launcherLogAppendf("serial put start path=%s size=%lu", normalizedPath.c_str(), parsedSize);
  sendLine("READY");
  return true;
}

void finishDownload(const char *status, const char *response, const char *logPrefix) {
  if (g_downloadFile) {
    g_downloadFile.close();
  }
  g_downloadActive = false;
  setStatus(status);
  if (response != nullptr) {
    sendError(response);
  }
  launcherLogAppendf("serial get %s path=%s sent=%lu expected=%lu", logPrefix, g_targetPath,
                     static_cast<unsigned long>(g_sentBytes),
                     static_cast<unsigned long>(g_downloadBytes));
}

void startDownload(const char *rawPath) {
  if (!ensureSdReady()) {
    return;
  }

  String normalizedPath;
  if (!parsePathArgument(rawPath, normalizedPath)) {
    sendError("invalid path");
    return;
  }

  g_downloadFile = SD.open(normalizedPath, FILE_READ);
  if (!g_downloadFile || g_downloadFile.isDirectory()) {
    if (g_downloadFile) {
      g_downloadFile.close();
    }
    sendError("not file");
    return;
  }

  const size_t fileSize = g_downloadFile.size();
  if (fileSize > UINT32_MAX) {
    g_downloadFile.close();
    sendError("file too large");
    return;
  }

  g_downloadBytes = static_cast<uint32_t>(fileSize);
  g_sentBytes = 0;
  g_downloadActive = true;
  g_lastDownloadActivityMs = millis();
  copyText(g_targetPath, sizeof(g_targetPath), normalizedPath.c_str());
  setStatus("Sending file");
  launcherLogAppendf("serial get start path=%s size=%lu", normalizedPath.c_str(),
                     static_cast<unsigned long>(g_downloadBytes));

  char line[32];
  snprintf(line, sizeof(line), "DATA %lu", static_cast<unsigned long>(g_downloadBytes));
  sendLine(line);
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

void handleRename(char *rawPaths) {
  if (!ensureSdReady()) {
    return;
  }

  char *separator = rawPaths == nullptr ? nullptr : strchr(rawPaths, '\t');
  if (separator == nullptr) {
    sendError("invalid paths");
    return;
  }
  *separator = '\0';

  String oldPath;
  String newPath;
  if (!parsePathArgument(rawPaths, oldPath) || !parsePathArgument(separator + 1, newPath) ||
      oldPath == "/" || newPath == "/") {
    sendError("invalid paths");
    return;
  }
  if (!SD.rename(oldPath, newPath)) {
    sendError("rename failed");
    return;
  }
  sendOk("rename");
  setStatus("Renamed path");
  copyText(g_targetPath, sizeof(g_targetPath), newPath.c_str());
  launcherLogAppendf("serial rename old=%s new=%s", oldPath.c_str(), newPath.c_str());
}

void processCommand() {
  g_commandBuffer[g_commandLength] = '\0';
  if (g_commandLength == 0) {
    return;
  }

  if (strcmp(g_commandBuffer, "SFS1 ENTER") == 0) {
    sendLine("SFS1 READY");
    return;
  }
  if (strcmp(g_commandBuffer, "SFS1 EXIT") == 0) {
    g_exitRequested = true;
    sendLine("SFS1 EXITING");
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
  if (strcmp(command, "INFO") == 0) {
    char line[64];
    snprintf(line, sizeof(line), "OK INFO CPU=%uMHz CORE=%d", ESP.getCpuFreqMHz(),
             xPortGetCoreID());
    sendLine(line);
    setStatus("Info shown");
    return;
  }
  if (strcmp(command, "HELP") == 0) {
    sendLine("OK COMMANDS PING INFO LIST MKDIR DELETE RENAME PUT GET HELP; CONTROL SFS1 EXIT");
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
  if (strcmp(command, "RENAME") == 0) {
    handleRename(arg1);
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
  if (strcmp(command, "GET") == 0) {
    startDownload(arg1);
    return;
  }

  sendError("unknown command");
}

void pollCommandLine() {
  while (vcp().available() > 0 && !g_uploadActive && !g_downloadActive) {
    const int value = vcp().read();
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

  const size_t remaining = g_expectedBytes - g_receivedBytes - g_uploadChunkLength;
  const size_t available = static_cast<size_t>(max(0, vcp().available()));
  if (available > 0 && remaining > 0) {
    const size_t capacity = sizeof(g_uploadChunk) - g_uploadChunkLength;
    const size_t readCount = vcp().read(g_uploadChunk + g_uploadChunkLength,
                                        min(min(capacity, remaining), available));
    if (readCount > 0) {
      g_uploadChunkLength += readCount;
      g_lastUploadActivityMs = millis();
    }
  }

  const bool finalChunk = g_receivedBytes + g_uploadChunkLength >= g_expectedBytes;
  if (g_uploadChunkLength == sizeof(g_uploadChunk) || finalChunk) {
    const size_t written = g_uploadFile.write(g_uploadChunk, g_uploadChunkLength);
    if (written != g_uploadChunkLength) {
      finishUpload("Write failed", "write failed", "failed", true);
      return;
    }
    g_receivedBytes += static_cast<uint32_t>(written);
    g_uploadChunkLength = 0;
    g_lastUploadActivityMs = millis();

    if (g_receivedBytes < g_expectedBytes && g_receivedBytes >= g_nextUploadAckBytes) {
      char line[32];
      snprintf(line, sizeof(line), "ACK %lu", static_cast<unsigned long>(g_receivedBytes));
      sendLine(line);
      g_nextUploadAckBytes =
          min<uint32_t>(g_expectedBytes, g_nextUploadAckBytes + VCP_WINDOW_SIZE);
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

void pollDownload() {
  if (!g_downloadActive) {
    return;
  }

  if (millis() - g_lastDownloadActivityMs > DOWNLOAD_TIMEOUT_MS) {
    finishDownload("Download timeout", "download timeout", "timeout");
    return;
  }

  if (g_sentBytes >= g_downloadBytes) {
    g_downloadFile.close();
    g_downloadActive = false;
    setStatus("Download complete");
    sendOk("sent");
    launcherLogAppendf("serial get complete path=%s size=%lu", g_targetPath,
                       static_cast<unsigned long>(g_downloadBytes));
    return;
  }

  const int writable = vcp().availableForWrite();
  if (writable <= 0) {
    return;
  }

  uint8_t chunk[DOWNLOAD_CHUNK_SIZE];
  const size_t want = min<size_t>(sizeof(chunk), g_downloadBytes - g_sentBytes);
  const size_t toRead = min<size_t>(want, static_cast<size_t>(writable));
  const size_t readCount = g_downloadFile.read(chunk, toRead);
  if (readCount == 0) {
    finishDownload("Read failed", "read failed", "failed");
    return;
  }

  const size_t written = vcp().write(chunk, readCount);
  g_sentBytes += static_cast<uint32_t>(written);
  if (written != readCount) {
    g_downloadFile.seek(g_sentBytes);
  }
  if (written > 0) {
    g_lastDownloadActivityMs = millis();
  }
}

void vcpFileServiceTask(void *) {
  setServiceTask(xTaskGetCurrentTaskHandle());
  launcherLogAppendf("vcp service task core=%d", xPortGetCoreID());
  publishSnapshot();

  while (!g_serviceStopRequested) {
    pollDownload();
    pollUpload();
    pollCommandLine();
    publishSnapshot();
    vTaskDelay(1);
  }

  if (g_uploadActive) {
    finishUpload("Stopped", nullptr, "aborted", true);
  }
  if (g_downloadActive) {
    finishDownload("Stopped", nullptr, "aborted");
  }
  g_uploadActive = false;
  g_downloadActive = false;
  setStatus("Stopped");
  publishSnapshot();
  launcherLogAppend("vcp service end");
  setServiceTask(nullptr);
  vTaskDelete(nullptr);
}

}

void serialFileServiceBegin(bool sdReady) {
  g_sdReady = sdReady;
  g_commandLength = 0;
  g_commandOverflow = false;
  g_uploadActive = false;
  g_expectedBytes = 0;
  g_receivedBytes = 0;
  g_uploadChunkLength = 0;
  g_nextUploadAckBytes = 0;
  g_lastUploadActivityMs = 0;
  g_downloadActive = false;
  g_downloadBytes = 0;
  g_sentBytes = 0;
  g_lastDownloadActivityMs = 0;
  g_exitRequested = false;
  g_serviceStopRequested = false;
  copyText(g_targetPath, sizeof(g_targetPath), "/");
  setStatus(sdReady ? "Waiting for host" : "SD unavailable");
  publishSnapshot();
  sendLine("SFS1 READY");
  launcherLogAppendf("vcp service begin sd_ready=%d", sdReady ? 1 : 0);

  TaskHandle_t task = nullptr;
  if (xTaskCreatePinnedToCore(vcpFileServiceTask, "vcp_file", 16384, nullptr, 2, &task, 1) !=
      pdPASS) {
    setStatus("Task start failed");
    publishSnapshot();
    sendError("task start failed");
  } else {
    setServiceTask(task);
  }
}

void serialFileServiceTick() {
}

void serialFileServiceEnd() {
  g_serviceStopRequested = true;
  while (serviceTask() != nullptr) {
    delay(1);
  }
}

SerialFileServiceSnapshot serialFileServiceSnapshot() {
  portENTER_CRITICAL(&g_serviceStateMux);
  const SerialFileServiceSnapshot snapshot = g_publishedSnapshot;
  portEXIT_CRITICAL(&g_serviceStateMux);
  return snapshot;
}

void serialFileServicePollLauncherControl() {
  while (vcp().available() > 0) {
    const int value = vcp().read();
    if (value < 0) {
      break;
    }
    const char ch = static_cast<char>(value);
    if (ch == '\r') {
      continue;
    }
    if (ch == '\n') {
      if (!g_controlOverflow) {
        g_controlBuffer[g_controlLength] = '\0';
        if (strcmp(g_controlBuffer, "SFS1 ENTER") == 0) {
          g_enterRequested = true;
          sendLine("SFS1 ENTERING");
        }
      }
      g_controlLength = 0;
      g_controlOverflow = false;
      continue;
    }
    if (g_controlLength + 1 < CONTROL_BUFFER_SIZE) {
      g_controlBuffer[g_controlLength++] = ch;
    } else {
      g_controlOverflow = true;
    }
  }
}

bool serialFileServiceTakeEnterRequest() {
  const bool requested = g_enterRequested;
  g_enterRequested = false;
  return requested;
}

bool serialFileServiceTakeExitRequest() {
  const bool requested = g_exitRequested;
  g_exitRequested = false;
  return requested;
}
