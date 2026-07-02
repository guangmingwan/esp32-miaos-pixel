#include "apps/ftp_server_app.h"

#include <Arduino.h>
#include <WiFi.h>

#define DEFAULT_FTP_SERVER_NETWORK_TYPE_ESP32 6
#define DEFAULT_STORAGE_TYPE_ESP32 5
#include <SimpleFTPServer.h>

#include "lava_native_display.h"

enum LavaPalette : uint8_t {
  LAVA_BLACK = 0,
  LAVA_WHITE = 1,
  LAVA_BLUE = 2,
  LAVA_GREEN = 3,
  LAVA_RED = 4,
  LAVA_YELLOW = 5,
  LAVA_CYAN = 6,
  LAVA_GRAY = 7,
  LAVA_DARK_BLUE = 8,
};

static constexpr const char *FTP_AP_SSID = "MiaOS-FTP";
static constexpr const char *FTP_USER = "guest";
static constexpr const char *FTP_PASS = "guest";

static FtpServer g_ftp;
static bool g_ftpStarted = false;
static IPAddress g_ip;
static String g_status = "Stopped";

static void drawFtpServer(AppContext &context) {
  if (!context.tftReady) {
    return;
  }
  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(4, 6, "FTP Server", LAVA_BLACK, LAVA_YELLOW);
  if (!context.sdReady) {
    lavaDrawText(108, 92, "SD unavailable", LAVA_RED, LAVA_BLACK);
    lavaDrawText(92, 222, "SEL+ST Exit", LAVA_GRAY, LAVA_BLACK);
    lavaPresent();
    return;
  }
  lavaDrawText(4, 34, g_status.c_str(), g_ftpStarted ? LAVA_GREEN : LAVA_YELLOW,
               LAVA_BLACK);
  lavaDrawText(4, 56, FTP_AP_SSID, LAVA_CYAN, LAVA_BLACK);
  String host = "ftp://" + g_ip.toString();
  lavaDrawText(4, 82, host.c_str(), LAVA_YELLOW, LAVA_BLACK);
  lavaDrawText(4, 108, "User: guest", LAVA_WHITE, LAVA_BLACK);
  lavaDrawText(4, 126, "Pass: guest", LAVA_WHITE, LAVA_BLACK);
  lavaDrawText(4, 222, "Use PASV  SEL+ST:Exit", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

static void ftpStatusCallback(FtpOperation operation, uint32_t freeSpace,
                              uint32_t totalSpace) {
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

static void ftpTransferCallback(FtpTransferOperation operation, const char *name,
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

static void ftpServerBegin(AppContext &context) {
  g_ftpStarted = false;
  g_status = "Starting...";
  drawFtpServer(context);
  if (!context.sdReady) {
    g_status = "SD unavailable";
    drawFtpServer(context);
    return;
  }
  WiFi.mode(WIFI_AP);
  WiFi.softAP(FTP_AP_SSID);
  g_ip = WiFi.softAPIP();
  g_ftp.setLocalIp(g_ip);
  g_ftp.setCallback(ftpStatusCallback);
  g_ftp.setTransferCallback(ftpTransferCallback);
  g_ftp.begin(FTP_USER, FTP_PASS, "MiaOS FTP ready");
  g_ftpStarted = true;
  g_status = "FTP ready";
  Serial.printf("FTP Server: %s ftp://%s user=%s pass=%s\n", FTP_AP_SSID,
                g_ip.toString().c_str(), FTP_USER, FTP_PASS);
  drawFtpServer(context);
}

static void ftpServerTick(AppContext &context, uint32_t nowMs) {
  (void)nowMs;
  if (g_ftpStarted) {
    g_ftp.handleFTP();
  }
  if (context.buttons[0].pressed) {
    drawFtpServer(context);
  }
}

static void ftpServerEnd(AppContext &context) {
  (void)context;
  if (g_ftpStarted) {
    g_ftp.end();
  }
  g_ftpStarted = false;
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

const LauncherApp &ftpServerApp() {
  static const LauncherApp app = {"FTP Server", ftpServerBegin, ftpServerTick,
                                  ftpServerEnd};
  return app;
}
