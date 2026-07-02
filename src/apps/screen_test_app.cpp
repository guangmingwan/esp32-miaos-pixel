#include "apps/screen_test_app.h"

#include <Arduino.h>
#include "lcd_ili9342.h"

namespace {

struct ScreenPattern {
  const char *label;
  uint16_t color;
};

constexpr uint32_t SCREEN_TEST_STEP_MS = 1500;
constexpr ScreenPattern SCREEN_PATTERNS[] = {
    {"RED", 0xF800},
    {"GREEN", 0x07E0},
    {"BLUE", 0x001F},
    {"WHITE", 0xFFFF},
    {"BLACK", 0x0000},
};

uint8_t g_patternIndex = 0;
uint32_t g_lastStepMs = 0;

void drawPattern(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  const ScreenPattern &pattern = SCREEN_PATTERNS[g_patternIndex];
  Serial.printf("[screen-test] %s\n", pattern.label);
  Lcd.fillScreen(pattern.color);
}

void stepPattern(AppContext &context, int8_t delta, uint32_t nowMs) {
  const int patternCount = static_cast<int>(sizeof(SCREEN_PATTERNS) / sizeof(SCREEN_PATTERNS[0]));
  int nextIndex = static_cast<int>(g_patternIndex) + delta;
  if (nextIndex < 0) {
    nextIndex += patternCount;
  }
  if (nextIndex >= patternCount) {
    nextIndex -= patternCount;
  }

  g_patternIndex = static_cast<uint8_t>(nextIndex);
  g_lastStepMs = nowMs;
  drawPattern(context);
}

void screenTestBegin(AppContext &context) {
  g_patternIndex = 0;
  g_lastStepMs = millis();
  drawPattern(context);
}

void screenTestTick(AppContext &context, uint32_t nowMs) {
  if (context.buttons[0].pressed) {
    stepPattern(context, 1, nowMs);
    return;
  }
  if (context.buttons[1].pressed) {
    stepPattern(context, -1, nowMs);
    return;
  }
  if (nowMs - g_lastStepMs >= SCREEN_TEST_STEP_MS) {
    stepPattern(context, 1, nowMs);
  }
}

void screenTestEnd(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  Lcd.fillScreen(0x0000);
}

}

const LauncherApp &screenTestApp() {
  static const LauncherApp app = {"Screen Test", screenTestBegin, screenTestTick,
                                  screenTestEnd};
  return app;
}
