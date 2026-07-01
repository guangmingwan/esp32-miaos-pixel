#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

#include "app.h"
#include "apps/about_app.h"
#include "apps/calculator_app.h"
#include "apps/diagnostic_app.h"
#include "apps/flashlight_app.h"
#include "apps/ftp_server_app.h"
#include "apps/minesweeper_app.h"
#include "apps/sd_browser_app.h"
#include "apps/timer_app.h"
#include "apps/wifi_files_app.h"
#include "apps/wifi_scan_app.h"
#include "lcd_ili9342.h"
#include "lava_native_display.h"
#include "pins.h"
#include "sd_app_loader.h"

SPIClass tftSpi(FSPI);
SPIClass sdSpi(HSPI);

static AppContext g_context = {};
static bool g_buttonLast[BUTTON_COUNT] = {};
static uint32_t g_lastLauncherRenderMs = 0;
static uint8_t g_selectedApp = 0;
static const LauncherApp *g_activeApp = nullptr;
static SdAppLoaderResult g_sdScan = {SdAppLoaderStatus::SdUnavailable, 0};
static SdAppManifestSummary g_sdApps[4] = {};
static SdAppLoaderResult g_lastSdRun = {SdAppLoaderStatus::Ok, 0};

static const LauncherApp *const BUILTIN_APPS[] = {
    &diagnosticApp(),
    &calculatorApp(),
    &flashlightApp(),
    &timerApp(),
    &sdBrowserApp(),
    &wifiFilesApp(),
    &ftpServerApp(),
    &wifiScanApp(),
    &minesweeperApp(),
    &aboutApp(),
};
static constexpr uint8_t BUILTIN_APP_COUNT = sizeof(BUILTIN_APPS) / sizeof(BUILTIN_APPS[0]);

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

static String macAddress() {
  const uint64_t mac = ESP.getEfuseMac();
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
           static_cast<uint8_t>(mac >> 40), static_cast<uint8_t>(mac >> 32),
           static_cast<uint8_t>(mac >> 24), static_cast<uint8_t>(mac >> 16),
           static_cast<uint8_t>(mac >> 8), static_cast<uint8_t>(mac));
  return String(buffer);
}

static bool buttonDown(size_t index) {
  return index < BUTTON_COUNT && digitalRead(BUTTONS[index].pin) == LOW;
}

static void configurePins() {
  pinMode(TFT_CS_PIN, OUTPUT);
  pinMode(TFT_DC_PIN, OUTPUT);
  digitalWrite(TFT_CS_PIN, HIGH);

  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);

  if (TFT_BL_PIN >= 0) {
    pinMode(TFT_BL_PIN, OUTPUT);
    digitalWrite(TFT_BL_PIN, HIGH);
  }

  pinMode(BEEP_PIN, OUTPUT);
  digitalWrite(BEEP_PIN, LOW);

  for (size_t i = 0; i < BUTTON_COUNT; ++i) {
    pinMode(BUTTONS[i].pin, BUTTONS[i].externalPullup ? INPUT : INPUT_PULLUP);
  }
}

static void initDisplay() {
  tftSpi.begin(TFT_SCK_PIN, TFT_MISO_PIN, TFT_MOSI_PIN, TFT_CS_PIN);
  Lcd.begin(tftSpi, TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN, TFT_BL_PIN, TFT_SPI_HZ);
  lavaDisplayInit();
  g_context.tftReady = true;
}

static void initSdCard() {
  digitalWrite(TFT_CS_PIN, HIGH);
#ifdef MIA_ENABLE_SD
  sdSpi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  g_context.sdReady = SD.begin(SD_CS_PIN, sdSpi);
#else
  g_context.sdReady = false;
#endif
  g_sdScan = scanSdApps(g_sdApps, sizeof(g_sdApps) / sizeof(g_sdApps[0]),
                        g_context.sdReady);
}

static void updateButtons() {
  for (size_t i = 0; i < BUTTON_COUNT; ++i) {
    const bool down = buttonDown(i);
    g_context.buttons[i].down = down;
    g_context.buttons[i].pressed = down && !g_buttonLast[i];
    g_context.buttons[i].released = !down && g_buttonLast[i];
    g_buttonLast[i] = down;
  }
}

static void updateBeep() {
  bool anyPressed = false;
  for (size_t i = 0; i < BUTTON_COUNT; ++i) {
    anyPressed = anyPressed || g_context.buttons[i].down;
  }
  digitalWrite(BEEP_PIN, anyPressed ? HIGH : LOW);
}

static const char *sdScanStatusText() {
  switch (g_sdScan.status) {
    case SdAppLoaderStatus::Ok:
      return "SD apps: ready";
    case SdAppLoaderStatus::SdUnavailable:
      return "SD apps: card unavailable";
    case SdAppLoaderStatus::NoAppsFound:
      return "SD apps: none";
    case SdAppLoaderStatus::ReadError:
      return "SD apps: read error";
    case SdAppLoaderStatus::RunError:
      return "SD apps: run error";
  }
  return "SD apps: unknown";
}

static uint8_t totalLauncherItems() {
  return BUILTIN_APP_COUNT + min<uint8_t>(g_sdScan.appCount,
                                          sizeof(g_sdApps) / sizeof(g_sdApps[0]));
}

static void drawLauncher() {
  if (!g_context.tftReady) {
    return;
  }

  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 16, LAVA_DARK_BLUE);
  lavaDrawText(4, 4, "MiaOS 0.1", LAVA_WHITE, LAVA_DARK_BLUE);

  lavaDrawText(4, 20, "Apps", LAVA_CYAN, LAVA_BLACK);
  const uint8_t maxVisibleApps = 6;
  uint8_t firstApp = 0;
  const uint8_t itemCount = totalLauncherItems();
  if (itemCount > maxVisibleApps && g_selectedApp >= maxVisibleApps) {
    firstApp = g_selectedApp - maxVisibleApps + 1;
  }
  const uint8_t visibleEnd = min<uint8_t>(itemCount, firstApp + maxVisibleApps);
  for (uint8_t i = firstApp; i < visibleEnd; ++i) {
    const int16_t y = 32 + (i - firstApp) * 12;
    const bool selected = i == g_selectedApp;
    const bool sdApp = i >= BUILTIN_APP_COUNT;
    const char *name = sdApp ? g_sdApps[i - BUILTIN_APP_COUNT].name : BUILTIN_APPS[i]->name;
    lavaFillRect(4, y - 2, 152, 11, selected ? LAVA_BLUE : LAVA_BLACK);
    lavaDrawText(10, y, selected ? ">" : " ", LAVA_YELLOW,
                 selected ? LAVA_BLUE : LAVA_BLACK);
    lavaDrawText(22, y, sdApp ? "SD:" : "", LAVA_GREEN,
                 selected ? LAVA_BLUE : LAVA_BLACK);
    lavaDrawText(sdApp ? 40 : 22, y, name, LAVA_WHITE,
                 selected ? LAVA_BLUE : LAVA_BLACK);
  }

  lavaDrawText(4, 110, sdScanStatusText(), g_context.sdReady ? LAVA_GREEN : LAVA_RED,
               LAVA_BLACK);
  if (g_lastSdRun.status != SdAppLoaderStatus::Ok) {
    lavaDrawText(4, 100, sdAppLoaderStatusText(g_lastSdRun.status), LAVA_RED,
                 LAVA_BLACK);
  }
  lavaDrawText(4, 120, "A:Start  B:Back", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

static void enterSelectedApp() {
  if (g_selectedApp >= totalLauncherItems()) {
    return;
  }

  if (g_selectedApp >= BUILTIN_APP_COUNT) {
    const SdAppManifestSummary &app = g_sdApps[g_selectedApp - BUILTIN_APP_COUNT];
    g_lastSdRun = runSdAppByPath(app.path, g_context.sdReady);
    drawLauncher();
    return;
  }

  g_activeApp = BUILTIN_APPS[g_selectedApp];
  if (g_activeApp->begin != nullptr) {
    g_activeApp->begin(g_context);
  }
}

static void exitActiveApp() {
  if (g_activeApp != nullptr && g_activeApp->end != nullptr) {
    g_activeApp->end(g_context);
  }
  g_activeApp = nullptr;
  g_lastLauncherRenderMs = 0;
  drawLauncher();
}

static void tickLauncher(uint32_t nowMs) {
  if (g_context.buttons[2].pressed && g_selectedApp > 0) {
    --g_selectedApp;
    drawLauncher();
  }
  if (g_context.buttons[3].pressed && g_selectedApp + 1 < totalLauncherItems()) {
    ++g_selectedApp;
    drawLauncher();
  }
  if (g_context.buttons[0].pressed) {
    enterSelectedApp();
    return;
  }

  if (nowMs - g_lastLauncherRenderMs >= 1000) {
    g_lastLauncherRenderMs = nowMs;
    drawLauncher();
  }
}

static void printStartupInfo() {
  Serial.println();
  Serial.println("ESP32-S3 Retro-Pixel launcher (ILI9342 320x240)");
  Serial.printf("Chip: %s rev%d, CPU %uMHz\n", ESP.getChipModel(),
                ESP.getChipRevision(), ESP.getCpuFreqMHz());
  Serial.printf("Flash: %u bytes, heap: %u bytes\n", ESP.getFlashChipSize(),
                ESP.getFreeHeap());
  Serial.printf("MAC: %s\n", macAddress().c_str());
  Serial.printf("TFT: ILI9342 320x240 rot=%u SCK=%d MOSI=%d CS=%d DC=%d RST=%d BL=%d\n",
                TFT_ROTATION, TFT_SCK_PIN, TFT_MOSI_PIN, TFT_CS_PIN, TFT_DC_PIN,
                TFT_RST_PIN, TFT_BL_PIN);
  Serial.printf("SD: SCK=%d MOSI=%d MISO=%d CS=%d\n", SD_SCK_PIN, SD_MOSI_PIN,
                SD_MISO_PIN, SD_CS_PIN);
  Serial.printf("ADC: VBAT=%d, Beep=%d\n", VBAT_ADC_PIN, BEEP_PIN);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("[setup] configurePins");
  configurePins();
  Serial.println("[setup] Wire.begin");
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("[setup] updateButtons");
  updateButtons();

  Serial.println("[setup] printStartupInfo");
  printStartupInfo();
  Serial.println("[setup] initDisplay - BEFORE");
  initDisplay();
  Serial.println("[setup] initDisplay - AFTER");

  Serial.println("[diag] fillScreen RED");
  Lcd.fillScreen(0xF800);
  delay(1500);
  Serial.println("[diag] fillScreen GREEN");
  Lcd.fillScreen(0x07E0);
  delay(1500);
  Serial.println("[diag] fillScreen BLUE");
  Lcd.fillScreen(0x001F);
  delay(1500);
  Serial.println("[diag] fillScreen WHITE");
  Lcd.fillScreen(0xFFFF);
  delay(1500);
  Serial.println("[diag] fillScreen BLACK");
  Lcd.fillScreen(0x0000);

  Serial.println("[setup] initSdCard");
  initSdCard();
  Serial.println("[setup] drawLauncher - BEFORE");
  drawLauncher();
  Serial.println("[setup] drawLauncher - AFTER");
}

void loop() {
  const uint32_t now = millis();

  updateButtons();
  updateBeep();

  if (g_activeApp != nullptr) {
    if (g_context.buttons[1].pressed) {
      exitActiveApp();
      return;
    }

    if (g_activeApp->tick != nullptr) {
      g_activeApp->tick(g_context, now);
    }
    return;
  }

  tickLauncher(now);
}

#if defined(MIA_ESPIDF_APP_MAIN)
extern "C" void app_main(void) {
  initArduino();
  setup();
  while (true) {
    loop();
    vTaskDelay(1);
  }
}
#endif
