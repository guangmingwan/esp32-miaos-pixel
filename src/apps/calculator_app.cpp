#include "apps/calculator_app.h"

#include <Arduino.h>

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

static constexpr uint8_t KEY_COUNT = 16;
static constexpr char KEYS[KEY_COUNT] = {'7', '8', '9', '/', '4', '5', '6', '*',
                                         '1', '2', '3', '-', 'C', '0', '=', '+'};

static uint8_t g_selectedKey = 0;
static int32_t g_accumulator = 0;
static int32_t g_entry = 0;
static char g_pendingOp = 0;
static bool g_hasAccumulator = false;
static bool g_error = false;

static void resetCalculator() {
  g_accumulator = 0;
  g_entry = 0;
  g_pendingOp = 0;
  g_hasAccumulator = false;
  g_error = false;
}

static bool applyPendingOp() {
  if (!g_hasAccumulator) {
    g_accumulator = g_entry;
    g_hasAccumulator = true;
    return true;
  }

  switch (g_pendingOp) {
    case '+':
      g_accumulator += g_entry;
      return true;
    case '-':
      g_accumulator -= g_entry;
      return true;
    case '*':
      g_accumulator *= g_entry;
      return true;
    case '/':
      if (g_entry == 0) {
        g_error = true;
        return false;
      }
      g_accumulator /= g_entry;
      return true;
  }

  g_accumulator = g_entry;
  return true;
}

static void pressCalculatorKey(char key) {
  if (key == 'C') {
    resetCalculator();
    return;
  }

  if (g_error) {
    return;
  }

  if (key >= '0' && key <= '9') {
    if (g_entry <= 9999999) {
      g_entry = g_entry * 10 + (key - '0');
    }
    return;
  }

  if (key == '=') {
    if (applyPendingOp()) {
      g_entry = g_accumulator;
      g_pendingOp = 0;
      g_hasAccumulator = false;
    }
    return;
  }

  if (applyPendingOp()) {
    g_pendingOp = key;
    g_entry = 0;
  }
}

static void drawCalculator(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(4, lavaTextYCentered(0, 20), "Calculator", LAVA_BLACK, LAVA_YELLOW);

  char line[32];
  lavaDrawRect(20, 32, 280, 24, LAVA_BLUE);
  if (g_error) {
    snprintf(line, sizeof(line), "Error: divide by 0");
  } else {
    snprintf(line, sizeof(line), "%ld", static_cast<long>(g_entry));
  }
  lavaDrawText(28, 40, line, g_error ? LAVA_RED : LAVA_WHITE, LAVA_BLACK);

  if (g_pendingOp != 0) {
    snprintf(line, sizeof(line), "%ld %c", static_cast<long>(g_accumulator),
             g_pendingOp);
    lavaDrawText(28, 64, line, LAVA_GRAY, LAVA_BLACK);
  } else {
    lavaDrawText(28, 64, "A:OK", LAVA_GRAY, LAVA_BLACK);
  }

  for (uint8_t i = 0; i < KEY_COUNT; ++i) {
    const int16_t x = 40 + (i % 4) * 60;
    const int16_t y = 92 + (i / 4) * 32;
    const bool selected = i == g_selectedKey;
    const uint8_t bg = selected ? LAVA_BLUE : LAVA_BLACK;
    const uint8_t fg = selected ? LAVA_YELLOW : LAVA_WHITE;
    char keyText[2] = {KEYS[i], 0};

    lavaFillRect(x, y, 48, 24, bg);
    lavaDrawRect(x, y, 48, 24, selected ? LAVA_YELLOW : LAVA_GRAY);
    lavaDrawText(x + 20, y + 8, keyText, fg, bg);
  }

  lavaDrawText(20, 222, "SEL+ST Exit", LAVA_GRAY, LAVA_BLACK);

  lavaPresent();
}

static void calculatorBegin(AppContext &context) {
  g_selectedKey = 0;
  resetCalculator();
  drawCalculator(context);
}

static void calculatorTick(AppContext &context, uint32_t nowMs) {
  (void)nowMs;

  bool changed = false;
  if (context.buttons[2].pressed && g_selectedKey >= 4) {
    g_selectedKey -= 4;
    changed = true;
  }
  if (context.buttons[3].pressed && g_selectedKey + 4 < KEY_COUNT) {
    g_selectedKey += 4;
    changed = true;
  }
  if (context.buttons[4].pressed && g_selectedKey % 4 > 0) {
    --g_selectedKey;
    changed = true;
  }
  if (context.buttons[5].pressed && g_selectedKey % 4 < 3) {
    ++g_selectedKey;
    changed = true;
  }
  if (context.buttons[0].pressed) {
    pressCalculatorKey(KEYS[g_selectedKey]);
    changed = true;
  }

  if (changed) {
    drawCalculator(context);
  }
}

static void calculatorEnd(AppContext &context) {
  (void)context;
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

const LauncherApp &calculatorApp() {
  static const LauncherApp app = {"Calculator", calculatorBegin, calculatorTick,
                                  calculatorEnd};
  return app;
}
