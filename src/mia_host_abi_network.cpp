#include "mia_host_abi.h"

#include <Arduino.h>
#include <SD.h>
#include <WiFi.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_FTP_SERVER_NETWORK_TYPE_ESP32 6
#define DEFAULT_STORAGE_TYPE_ESP32 5
#include <SimpleFTPServer.h>

#include "app.h"
#include "wifi_file_config.h"
#include "wifi_file_http.h"

extern AppContext g_context;

namespace {

constexpr uint32_t WIFI_STA_CONNECT_TIMEOUT_MS = 12000;
constexpr char WIFI_FILES_DEFAULT_AP_SSID[] = "MiaOS-SD";
constexpr char FTP_AP_SSID[] = "MiaOS-FTP";
constexpr char FTP_USER[] = "guest";
constexpr char FTP_PASS[] = "guest";

WifiFileConfig g_wifiFilesConfig = {"", "", WIFI_FILES_DEFAULT_AP_SSID, ""};
bool g_wifiFilesRunning = false;
bool g_wifiFilesApMode = false;
IPAddress g_wifiFilesIp;
String g_wifiFilesStatus = "Stopped";

FtpServer g_ftpServer;
bool g_ftpRunning = false;
IPAddress g_ftpIp;
String g_ftpStatus = "Stopped";

void copyString(char *dest, size_t destSize, const String &value) {
  if (destSize == 0) {
    return;
  }
  strncpy(dest, value.c_str(), destSize - 1);
  dest[destSize - 1] = '\0';
}

void copyText(char *dest, size_t destSize, const char *value) {
  if (destSize == 0) {
    return;
  }
  strncpy(dest, value, destSize - 1);
  dest[destSize - 1] = '\0';
}

void stopWifiFilesService() {
  stopWifiFileHttpServer();
  g_wifiFilesRunning = false;
  g_wifiFilesApMode = false;
  g_wifiFilesIp = IPAddress();
  g_wifiFilesStatus = "Stopped";
}

void stopFtpService() {
  if (g_ftpRunning) {
    g_ftpServer.end();
  }
  g_ftpRunning = false;
  g_ftpIp = IPAddress();
  g_ftpStatus = "Stopped";
}

bool connectWifiFilesStation() {
  if (g_wifiFilesConfig.stationSsid.length() == 0) {
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(g_wifiFilesConfig.stationSsid.c_str(), g_wifiFilesConfig.stationPassword.c_str());
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < WIFI_STA_CONNECT_TIMEOUT_MS) {
    delay(200);
  }

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(true);
    return false;
  }

  g_wifiFilesApMode = false;
  g_wifiFilesIp = WiFi.localIP();
  g_wifiFilesStatus = "HTTP ready";
  return true;
}

void startWifiFilesAccessPoint() {
  WiFi.mode(WIFI_AP);
  if (g_wifiFilesConfig.accessPointPassword.length() >= 8) {
    WiFi.softAP(g_wifiFilesConfig.accessPointSsid.c_str(),
                g_wifiFilesConfig.accessPointPassword.c_str());
  } else {
    WiFi.softAP(g_wifiFilesConfig.accessPointSsid.c_str());
  }
  g_wifiFilesApMode = true;
  g_wifiFilesIp = WiFi.softAPIP();
  g_wifiFilesStatus = "HTTP ready";
}

void ftpStatusCallback(FtpOperation operation, uint32_t freeSpace, uint32_t totalSpace) {
  (void)freeSpace;
  (void)totalSpace;
  switch (operation) {
    case FTP_CONNECT:
      Serial.println(F("FTP: client connected"));
      break;
    case FTP_DISCONNECT:
      Serial.println(F("FTP: client disconnected"));
      break;
    case FTP_FREE_SPACE_CHANGE:
      Serial.println(F("FTP: free space changed"));
      break;
  }
}

void ftpTransferCallback(FtpTransferOperation operation, const char *name,
                         uint32_t transferredSize) {
  switch (operation) {
    case FTP_UPLOAD_START:
      Serial.printf("FTP: upload start %s\n", name);
      break;
    case FTP_DOWNLOAD_START:
      Serial.printf("FTP: download start %s\n", name);
      break;
    case FTP_TRANSFER_STOP:
      Serial.printf("FTP: transfer complete %u bytes\n", transferredSize);
      break;
    case FTP_TRANSFER_ERROR:
      Serial.println(F("FTP: transfer error"));
      break;
    case FTP_UPLOAD:
    case FTP_DOWNLOAD:
      break;
  }
}

}

int32_t mia_host_wifi_scan(MiaHostWifiNetwork *networks, uint32_t capacity) {
  if (networks == nullptr || capacity == 0) {
    return -1;
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  WiFi.scanDelete();
  const int count = WiFi.scanNetworks(false, true);
  if (count <= 0) {
    return count;
  }

  const uint32_t stored = count < static_cast<int>(capacity) ? static_cast<uint32_t>(count) : capacity;
  for (uint32_t index = 0; index < stored; ++index) {
    memset(&networks[index], 0, sizeof(MiaHostWifiNetwork));
    copyString(networks[index].ssid, sizeof(networks[index].ssid), WiFi.SSID(index));
    networks[index].rssi = WiFi.RSSI(index);
  }
  WiFi.scanDelete();
  return static_cast<int32_t>(stored);
}

void mia_host_wifi_off(void) {
  WiFi.scanDelete();
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
}

uint8_t mia_host_wifi_files_start(void) {
  if (!g_context.sdReady) {
    g_wifiFilesStatus = "SD unavailable";
    return 0;
  }

  stopFtpService();
  stopWifiFilesService();
  mia_host_wifi_off();

  g_wifiFilesConfig = loadWifiFileConfig();
  g_wifiFilesStatus = "Starting...";
  if (!connectWifiFilesStation()) {
    startWifiFilesAccessPoint();
  }
  startWifiFileHttpServer();
  g_wifiFilesRunning = true;
  return 1;
}

void mia_host_wifi_files_poll(void) {
  handleWifiFileHttpClient();
}

void mia_host_wifi_files_stop(void) {
  stopWifiFilesService();
  mia_host_wifi_off();
}

uint8_t mia_host_wifi_files_get_status(MiaHostWifiFilesStatus *status) {
  if (status == nullptr) {
    return 0;
  }

  memset(status, 0, sizeof(MiaHostWifiFilesStatus));
  status->running = g_wifiFilesRunning ? 1 : 0;
  status->ap_mode = g_wifiFilesApMode ? 1 : 0;
  copyString(status->status, sizeof(status->status), g_wifiFilesStatus);
  copyString(status->ssid, sizeof(status->ssid),
             g_wifiFilesApMode ? g_wifiFilesConfig.accessPointSsid : g_wifiFilesConfig.stationSsid);
  copyString(status->ip, sizeof(status->ip), g_wifiFilesIp.toString());
  return 1;
}

uint8_t mia_host_ftp_start(void) {
  if (!g_context.sdReady) {
    g_ftpStatus = "SD unavailable";
    return 0;
  }

  stopWifiFilesService();
  stopFtpService();
  mia_host_wifi_off();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(FTP_AP_SSID);
  g_ftpIp = WiFi.softAPIP();
  g_ftpServer.setLocalIp(g_ftpIp);
  g_ftpServer.setCallback(ftpStatusCallback);
  g_ftpServer.setTransferCallback(ftpTransferCallback);
  g_ftpServer.begin(FTP_USER, FTP_PASS, "MiaOS FTP ready");
  g_ftpRunning = true;
  g_ftpStatus = "FTP ready";
  return 1;
}

void mia_host_ftp_poll(void) {
  if (g_ftpRunning) {
    g_ftpServer.handleFTP();
  }
}

void mia_host_ftp_stop(void) {
  stopFtpService();
  mia_host_wifi_off();
}

uint8_t mia_host_ftp_get_status(MiaHostFtpStatus *status) {
  if (status == nullptr) {
    return 0;
  }

  memset(status, 0, sizeof(MiaHostFtpStatus));
  status->running = g_ftpRunning ? 1 : 0;
  copyString(status->status, sizeof(status->status), g_ftpStatus);
  copyText(status->ssid, sizeof(status->ssid), FTP_AP_SSID);
  copyString(status->ip, sizeof(status->ip), g_ftpIp.toString());
  copyText(status->user, sizeof(status->user), FTP_USER);
  copyText(status->pass, sizeof(status->pass), FTP_PASS);
  return 1;
}
