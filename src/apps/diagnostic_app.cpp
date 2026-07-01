#include "apps/diagnostic_app.h"

#include <Arduino.h>
#include <inttypes.h>

#include "lava_native_display.h"
#include "pins.h"

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

static uint32_t g_lastRenderMs = 0;
static uint32_t g_lastLogMs = 0;
static uint32_t g_frame = 0;

static bool buttonPressed(const AppContext &context, size_t index) {
  return index < BUTTON_COUNT && context.buttons[index].down;
}

static void drawStatusBox(int16_t x, int16_t y, const char *label, bool ok) {
  const uint8_t color = ok ? LAVA_GREEN : LAVA_RED;
  lavaFillRect(x, y, 45, 16, LAVA_BLACK);
  lavaDrawRect(x, y, 45, 16, color);
  lavaDrawText(x + 4, y + 4, label, color, LAVA_BLACK);
}

static void drawButtonGrid(const AppContext &context, int16_t y) {
  lavaFillRect(0, y, LAVA_SCREEN_W, 43, LAVA_BLACK);
  lavaDrawText(2, y, "BTN", LAVA_WHITE, LAVA_BLACK);

  for (size_t i = 0; i < BUTTON_COUNT; ++i) {
    const int16_t x = 2 + (i % 3) * 52;
    const int16_t rowY = y + 12 + (i / 3) * 18;
    const bool pressed = buttonPressed(context, i);
    const uint8_t color = pressed ? LAVA_YELLOW : LAVA_BLUE;

    lavaFillRect(x, rowY, 46, 14, color);
    lavaDrawText(x + 4, rowY + 3, BUTTONS[i].label, LAVA_BLACK, color);
    lavaDrawText(x + 16, rowY + 3, pressed ? ":P" : ":.", LAVA_BLACK, color);
  }
}

static void drawStaticScreen(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 16, LAVA_BLUE);
  lavaDrawText(4, 4, "ESP32-S3 ILI9342", LAVA_WHITE, LAVA_BLUE);

  char line[48];
  snprintf(line, sizeof(line), "%s r%d %uMHz Heap %uK", ESP.getChipModel(),
           ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getFreeHeap() / 1024);
  lavaDrawText(2, 39, line, LAVA_WHITE, LAVA_BLACK);

  snprintf(line, sizeof(line), "Flash %uMB SD-CS%d", ESP.getFlashChipSize() / 1024 / 1024,
           SD_CS_PIN);
  lavaDrawText(2, 51, line, LAVA_WHITE, LAVA_BLACK);

  lavaDrawText(2, 63, "VBAT1", LAVA_WHITE, LAVA_BLACK);
}

static void renderDisplay(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  const int vbat = analogRead(VBAT_ADC_PIN);

  drawStatusBox(2, 19, "TFT OK", context.tftReady);
  drawStatusBox(52, 19, "SD", context.sdReady);
  drawStatusBox(102, 19, "SPI", true);

  char line[48];
  lavaFillRect(2, 63, 156, 8, LAVA_BLACK);
  snprintf(line, sizeof(line), "VBAT1 %4d", vbat);
  lavaDrawText(2, 63, line, LAVA_WHITE, LAVA_BLACK);

  drawButtonGrid(context, 76);

  lavaFillRect(2, 120, 156, 8, LAVA_BLACK);
  snprintf(line, sizeof(line), "Uptime %" PRIu32 "s F%" PRIu32,
           static_cast<uint32_t>(millis() / 1000), g_frame++);
  lavaDrawText(2, 120, line, LAVA_CYAN, LAVA_BLACK);
  lavaPresent();
}

static void logStatus(AppContext &context) {
  const int vbat = analogRead(VBAT_ADC_PIN);

  Serial.printf("TFT=%s SD=%s VBAT=%d Buttons:",
                context.tftReady ? "OK" : "FAIL", context.sdReady ? "OK" : "FAIL", vbat);
  for (size_t i = 0; i < BUTTON_COUNT; ++i) {
    Serial.printf(" %s=%d", BUTTONS[i].label, buttonPressed(context, i) ? 1 : 0);
  }
  Serial.println();
}

static void diagnosticBegin(AppContext &context) {
  g_lastRenderMs = 0;
  g_lastLogMs = 0;
  g_frame = 0;
  drawStaticScreen(context);
  renderDisplay(context);
  logStatus(context);
}

static void diagnosticTick(AppContext &context, uint32_t nowMs) {
  if (nowMs - g_lastRenderMs >= 500) {
    g_lastRenderMs = nowMs;
    renderDisplay(context);
  }

  if (nowMs - g_lastLogMs >= 2000) {
    g_lastLogMs = nowMs;
    logStatus(context);
  }
}

static void diagnosticEnd(AppContext &context) {
  (void)context;
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

const LauncherApp &diagnosticApp() {
  static const LauncherApp app = {"Diagnostic", diagnosticBegin, diagnosticTick,
                                  diagnosticEnd};
  return app;
}
