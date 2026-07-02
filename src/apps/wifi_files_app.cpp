#include "apps/wifi_files_app.h"

#include <Arduino.h>
#include <WiFi.h>

#include "lava_native_display.h"
#include "wifi_file_config.h"
#include "wifi_file_http.h"

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

static constexpr uint32_t STA_CONNECT_TIMEOUT_MS = 12000;

static WifiFileConfig g_config;
static bool g_serverStarted = false;
static bool g_apMode = false;
static IPAddress g_ip;
static String g_status = "Stopped";

static void drawWifiFiles(AppContext &context) {
  if (!context.tftReady) {
    return;
  }
  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(4, 6, "WiFi Files", LAVA_BLACK, LAVA_YELLOW);
  if (!context.sdReady) {
    lavaDrawText(108, 92, "SD unavailable", LAVA_RED, LAVA_BLACK);
    lavaDrawText(92, 222, "SEL+ST Exit", LAVA_GRAY, LAVA_BLACK);
    lavaPresent();
    return;
  }
  lavaDrawText(4, 34, g_status.c_str(), g_serverStarted ? LAVA_GREEN : LAVA_YELLOW,
               LAVA_BLACK);
  lavaDrawText(4, 56, g_apMode ? "Mode: AP" : "Mode: Router", LAVA_CYAN, LAVA_BLACK);
  lavaDrawText(4, 78,
                g_apMode ? g_config.accessPointSsid.c_str() : g_config.stationSsid.c_str(),
                LAVA_WHITE, LAVA_BLACK);
  String url = "http://" + g_ip.toString();
  lavaDrawText(4, 104, url.c_str(), LAVA_YELLOW, LAVA_BLACK);
  lavaDrawText(4, 222, "Guest access  SEL+ST:Exit", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

static bool connectStation() {
  if (g_config.stationSsid.length() == 0) {
    return false;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(g_config.stationSsid.c_str(), g_config.stationPassword.c_str());
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < STA_CONNECT_TIMEOUT_MS) {
    delay(200);
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(true);
    return false;
  }
  g_apMode = false;
  g_ip = WiFi.localIP();
  g_status = "HTTP ready";
  return true;
}

static void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  if (g_config.accessPointPassword.length() >= 8) {
    WiFi.softAP(g_config.accessPointSsid.c_str(), g_config.accessPointPassword.c_str());
  } else {
    WiFi.softAP(g_config.accessPointSsid.c_str());
  }
  g_apMode = true;
  g_ip = WiFi.softAPIP();
  g_status = "HTTP ready";
}

static void wifiFilesBegin(AppContext &context) {
  g_serverStarted = false;
  g_status = "Starting...";
  drawWifiFiles(context);
  if (!context.sdReady) {
    g_status = "SD unavailable";
    drawWifiFiles(context);
    return;
  }
  g_config = loadWifiFileConfig();
  if (!connectStation()) {
    startAccessPoint();
  }
  startWifiFileHttpServer();
  g_serverStarted = true;
  Serial.printf("WiFi Files: %s http://%s/\n",
                g_apMode ? g_config.accessPointSsid.c_str() : g_config.stationSsid.c_str(),
                g_ip.toString().c_str());
  drawWifiFiles(context);
}

static void wifiFilesTick(AppContext &context, uint32_t nowMs) {
  (void)nowMs;
  handleWifiFileHttpClient();
  if (context.buttons[0].pressed) {
    drawWifiFiles(context);
  }
}

static void wifiFilesEnd(AppContext &context) {
  (void)context;
  stopWifiFileHttpServer();
  g_serverStarted = false;
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

const LauncherApp &wifiFilesApp() {
  static const LauncherApp app = {"WiFi Files", wifiFilesBegin, wifiFilesTick,
                                  wifiFilesEnd};
  return app;
}
