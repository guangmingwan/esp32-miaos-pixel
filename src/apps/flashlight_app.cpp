#include "apps/flashlight_app.h"

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

static bool g_lightOn = true;

static void drawFlashlight(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  lavaClear(g_lightOn ? LAVA_WHITE : LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(4, 6, "Flashlight", LAVA_BLACK, LAVA_YELLOW);
  lavaDrawText(132, 104, g_lightOn ? "LIGHT ON" : "LIGHT OFF",
                g_lightOn ? LAVA_BLACK : LAVA_WHITE, g_lightOn ? LAVA_WHITE : LAVA_BLACK);
  lavaDrawText(84, 222, "A:Toggle  SEL+ST:Exit", g_lightOn ? LAVA_BLACK : LAVA_GRAY,
                g_lightOn ? LAVA_WHITE : LAVA_BLACK);
  lavaPresent();
}

static void flashlightBegin(AppContext &context) {
  g_lightOn = true;
  if (TFT_BL_PIN >= 0) {
    digitalWrite(TFT_BL_PIN, LOW);
  }
  drawFlashlight(context);
}

static void flashlightTick(AppContext &context, uint32_t nowMs) {
  (void)nowMs;
  if (context.buttons[0].pressed) {
    g_lightOn = !g_lightOn;
    if (TFT_BL_PIN >= 0) {
      digitalWrite(TFT_BL_PIN, g_lightOn ? LOW : HIGH);
    }
    drawFlashlight(context);
  }
}

static void flashlightEnd(AppContext &context) {
  (void)context;
  if (TFT_BL_PIN >= 0) {
    digitalWrite(TFT_BL_PIN, LOW);
  }
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

const LauncherApp &flashlightApp() {
  static const LauncherApp app = {"Flashlight", flashlightBegin, flashlightTick,
                                  flashlightEnd};
  return app;
}
