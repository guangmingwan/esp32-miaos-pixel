#include "apps/sd_browser_app.h"

#include <Arduino.h>
#include <SD.h>

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

static constexpr uint8_t MAX_FILES = 8;
static String g_files[MAX_FILES];
static uint8_t g_fileCount = 0;
static uint8_t g_selectedFile = 0;

static void scanRootFiles(AppContext &context) {
  g_fileCount = 0;
  g_selectedFile = 0;
  if (!context.sdReady) {
    return;
  }

  File root = SD.open("/");
  if (!root) {
    return;
  }

  while (g_fileCount < MAX_FILES) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }
    g_files[g_fileCount] = entry.name();
    if (entry.isDirectory()) {
      g_files[g_fileCount] += "/";
    }
    ++g_fileCount;
    entry.close();
  }
  root.close();
}

static void drawSdBrowser(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 16, LAVA_DARK_BLUE);
  lavaDrawText(4, 4, "SD Browser", LAVA_WHITE, LAVA_DARK_BLUE);

  if (!context.sdReady) {
    lavaDrawText(16, 48, "SD card unavailable", LAVA_RED, LAVA_BLACK);
    lavaDrawText(20, 96, "A:Rescan B:Back", LAVA_GRAY, LAVA_BLACK);
    lavaPresent();
    return;
  }

  if (g_fileCount == 0) {
    lavaDrawText(34, 48, "No files", LAVA_YELLOW, LAVA_BLACK);
  }

  for (uint8_t i = 0; i < g_fileCount; ++i) {
    const int16_t y = 24 + i * 11;
    const bool selected = i == g_selectedFile;
    const char *name = g_files[i].c_str();
    lavaFillRect(4, y - 1, 152, 10, selected ? LAVA_BLUE : LAVA_BLACK);
    lavaDrawText(8, y, name, selected ? LAVA_YELLOW : LAVA_WHITE,
                 selected ? LAVA_BLUE : LAVA_BLACK);
  }

  lavaDrawText(8, 116, "UP/DN Select A:Rescan", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

static void sdBrowserBegin(AppContext &context) {
  scanRootFiles(context);
  drawSdBrowser(context);
}

static void sdBrowserTick(AppContext &context, uint32_t nowMs) {
  (void)nowMs;
  bool changed = false;
  if (context.buttons[2].pressed && g_selectedFile > 0) {
    --g_selectedFile;
    changed = true;
  }
  if (context.buttons[3].pressed && g_selectedFile + 1 < g_fileCount) {
    ++g_selectedFile;
    changed = true;
  }
  if (context.buttons[0].pressed) {
    scanRootFiles(context);
    changed = true;
  }
  if (changed) {
    drawSdBrowser(context);
  }
}

static void sdBrowserEnd(AppContext &context) {
  (void)context;
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

const LauncherApp &sdBrowserApp() {
  static const LauncherApp app = {"SD Browser", sdBrowserBegin, sdBrowserTick,
                                  sdBrowserEnd};
  return app;
}
