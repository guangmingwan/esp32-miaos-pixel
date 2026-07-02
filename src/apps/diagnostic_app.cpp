#include "apps/diagnostic_app.h"

#include <Arduino.h>
#include <inttypes.h>

#include "lava_native_display.h"
#include "pins.h"

extern ButtonState g_allButtons[];

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

static bool buttonDown(size_t index) {
  return index < ALL_BUTTON_COUNT && g_allButtons[index].down;
}

static void drawStatusBox(int16_t x, int16_t y, const char *label, bool ok) {
  const uint8_t color = ok ? LAVA_GREEN : LAVA_RED;
  lavaFillRect(x, y, 68, 18, LAVA_BLACK);
  lavaDrawRect(x, y, 68, 18, color);
  lavaDrawText(x + 8, y + 5, label, color, LAVA_BLACK);
}

static void drawButtonGrid(int16_t y) {
  lavaFillRect(0, y, LAVA_SCREEN_W, 64, LAVA_BLACK);
  lavaDrawText(2, y, "BTN", LAVA_WHITE, LAVA_BLACK);

  for (size_t i = 0; i < ALL_BUTTON_COUNT; ++i) {
    const int16_t col = i % 7;
    const int16_t row = i / 7;
    const int16_t x = 4 + col * 45;
    const int16_t rowY = y + 14 + row * 24;
    const bool pressed = buttonDown(i);
    const uint8_t color = pressed ? LAVA_YELLOW : LAVA_BLUE;

    lavaFillRect(x, rowY, 40, 18, color);
    lavaDrawText(x + 5, rowY + 5, ALL_BUTTONS[i].label, LAVA_BLACK, color);
  }
}

static void drawStaticScreen(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(4, 6, "ESP32-S3 Diagnostic", LAVA_BLACK, LAVA_YELLOW);

  char line[48];
  snprintf(line, sizeof(line), "%s r%d %uMHz Heap %uK", ESP.getChipModel(),
           ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getFreeHeap() / 1024);
  lavaDrawText(4, 34, line, LAVA_WHITE, LAVA_BLACK);

  snprintf(line, sizeof(line), "Flash %uMB SD-CS%d", ESP.getFlashChipSize() / 1024 / 1024,
           SD_CS_PIN);
  lavaDrawText(4, 48, line, LAVA_WHITE, LAVA_BLACK);
  lavaDrawText(4, 62, "BOOT/ST share GPIO0", LAVA_GRAY, LAVA_BLACK);
}

static void renderDisplay(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  const int vbat_raw = analogRead(VBAT_ADC_PIN);
  const float vbat_v = vbat_raw * 3.3f / 4095.0f * VBAT_DIVIDER;

  drawStatusBox(8, 82, "TFT OK", context.tftReady);
  drawStatusBox(84, 82, "SD OK", context.sdReady);
  drawStatusBox(160, 82, "SPI OK", true);

  char line[48];
  lavaFillRect(4, 62, 220, 8, LAVA_BLACK);
  snprintf(line, sizeof(line), "VBAT %.2fV (%d)", vbat_v, vbat_raw);
  lavaDrawText(4, 62, line, LAVA_WHITE, LAVA_BLACK);

  drawButtonGrid(112);

  lavaFillRect(4, 196, 220, 8, LAVA_BLACK);
  snprintf(line, sizeof(line), "Uptime %" PRIu32 "s F%" PRIu32,
           static_cast<uint32_t>(millis() / 1000), g_frame++);
  lavaDrawText(4, 196, line, LAVA_CYAN, LAVA_BLACK);
  lavaDrawText(4, 210, "Hold SEL+ST to exit", LAVA_GRAY, LAVA_BLACK);
  lavaDrawText(4, 224, "BOOT and ST mirror GPIO0", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

static void logStatus(AppContext &context) {
  const int vbat_raw = analogRead(VBAT_ADC_PIN);
  const float vbat_v = vbat_raw * 3.3f / 4095.0f * VBAT_DIVIDER;

  Serial.printf("TFT=%s SD=%s VBAT=%.2fV Buttons:",
                context.tftReady ? "OK" : "FAIL", context.sdReady ? "OK" : "FAIL",
                vbat_v);
  for (size_t i = 0; i < ALL_BUTTON_COUNT; ++i) {
    Serial.printf(" %s=%d", ALL_BUTTONS[i].label, buttonDown(i) ? 1 : 0);
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
