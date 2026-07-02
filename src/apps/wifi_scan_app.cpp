#include "apps/wifi_scan_app.h"

#include <Arduino.h>
#include <WiFi.h>

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

static constexpr int MAX_NETWORKS = 10;
static int g_networkCount = 0;
static int g_firstNetwork = 0;
static bool g_scanning = false;

static void drawWifiScan(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(4, 6, "WiFi Scan", LAVA_BLACK, LAVA_YELLOW);

  if (g_scanning) {
    lavaDrawText(128, 96, "Scanning...", LAVA_CYAN, LAVA_BLACK);
    lavaDrawText(118, 116, "Please wait", LAVA_GRAY, LAVA_BLACK);
    lavaDrawText(92, 222, "SEL+ST Exit", LAVA_GRAY, LAVA_BLACK);
    lavaPresent();
    return;
  }

  if (g_networkCount <= 0) {
    lavaDrawText(116, 92, "No networks", LAVA_YELLOW, LAVA_BLACK);
    lavaDrawText(80, 222, "A:Scan  SEL+ST:Exit", LAVA_GRAY, LAVA_BLACK);
    lavaPresent();
    return;
  }

  char line[34];
  const int visibleCount = min(MAX_NETWORKS, g_networkCount - g_firstNetwork);
  for (int i = 0; i < visibleCount; ++i) {
    const int index = g_firstNetwork + i;
    String ssid = WiFi.SSID(index);
    if (ssid.length() > 16) {
      ssid = ssid.substring(0, 16);
    }
    snprintf(line, sizeof(line), "%2ddB %s", WiFi.RSSI(index), ssid.c_str());
    lavaDrawText(8, 36 + i * 18, line, i == 0 ? LAVA_YELLOW : LAVA_WHITE, LAVA_BLACK);
  }

  snprintf(line, sizeof(line), "%d found A:Rescan", g_networkCount);
  lavaDrawText(8, 206, line, LAVA_GRAY, LAVA_BLACK);
  lavaDrawText(8, 222, "UP/DN Scroll  SEL+ST:Exit", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

static void startWifiScan(AppContext &context) {
  g_scanning = true;
  drawWifiScan(context);
  WiFi.scanDelete();
  g_networkCount = WiFi.scanNetworks(false, true);
  g_firstNetwork = 0;
  g_scanning = false;
  drawWifiScan(context);
}

static void wifiScanBegin(AppContext &context) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  g_networkCount = 0;
  g_firstNetwork = 0;
  startWifiScan(context);
}

static void wifiScanTick(AppContext &context, uint32_t nowMs) {
  (void)nowMs;
  if (context.buttons[0].pressed) {
    startWifiScan(context);
    return;
  }
  if (context.buttons[2].pressed && g_firstNetwork > 0) {
    --g_firstNetwork;
    drawWifiScan(context);
  }
  if (context.buttons[3].pressed && g_firstNetwork + MAX_NETWORKS < g_networkCount) {
    ++g_firstNetwork;
    drawWifiScan(context);
  }
}

static void wifiScanEnd(AppContext &context) {
  (void)context;
  WiFi.scanDelete();
  WiFi.mode(WIFI_OFF);
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

const LauncherApp &wifiScanApp() {
  static const LauncherApp app = {"WiFi Scan", wifiScanBegin, wifiScanTick,
                                  wifiScanEnd};
  return app;
}
