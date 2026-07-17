#include "apps/timer_app.h"

#include <Arduino.h>

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

static bool g_running = false;
static uint32_t g_elapsedMs = 0;
static uint32_t g_lastStartMs = 0;
static uint32_t g_lastDrawMs = 0;

static uint32_t currentElapsed(uint32_t nowMs) {
  if (!g_running) {
    return g_elapsedMs;
  }
  return g_elapsedMs + nowMs - g_lastStartMs;
}

static void drawTimer(AppContext &context, uint32_t nowMs) {
  if (!context.tftReady) {
    return;
  }

  const uint32_t totalSeconds = currentElapsed(nowMs) / 1000;
  const uint32_t minutes = totalSeconds / 60;
  const uint32_t seconds = totalSeconds % 60;
  const uint32_t tenths = (currentElapsed(nowMs) / 100) % 10;
  char line[24];

  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(4, lavaTextYCentered(0, 20), "Timer", LAVA_BLACK, LAVA_YELLOW);
  lavaDrawText(118, 54, g_running ? "RUNNING" : "PAUSED", g_running ? LAVA_GREEN : LAVA_YELLOW,
                LAVA_BLACK);
  snprintf(line, sizeof(line), "%02lu:%02lu.%lu", static_cast<unsigned long>(minutes),
           static_cast<unsigned long>(seconds), static_cast<unsigned long>(tenths));
  lavaDrawText(110, 96, line, LAVA_WHITE, LAVA_BLACK);
  lavaDrawText(82, 206, "A:Start/Pause", LAVA_GRAY, LAVA_BLACK);
  lavaDrawText(58, 222, "LT+RT:Reset  SEL+ST:Exit", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

static void timerBegin(AppContext &context) {
  g_running = false;
  g_elapsedMs = 0;
  g_lastStartMs = millis();
  g_lastDrawMs = 0;
  drawTimer(context, millis());
}

static void timerTick(AppContext &context, uint32_t nowMs) {
  bool changed = false;
  if (context.buttons[0].pressed) {
    if (g_running) {
      g_elapsedMs += nowMs - g_lastStartMs;
      g_running = false;
    } else {
      g_lastStartMs = nowMs;
      g_running = true;
    }
    changed = true;
  }

  if (context.buttons[4].pressed && context.buttons[5].down) {
    g_running = false;
    g_elapsedMs = 0;
    changed = true;
  }

  if (changed || nowMs - g_lastDrawMs >= 100) {
    g_lastDrawMs = nowMs;
    drawTimer(context, nowMs);
  }
}

static void timerEnd(AppContext &context) {
  (void)context;
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

const LauncherApp &timerApp() {
  static const LauncherApp app = {"Timer", timerBegin, timerTick, timerEnd};
  return app;
}
