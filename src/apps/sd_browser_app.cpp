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

static constexpr uint16_t MAX_FILES = 1024;
static constexpr uint8_t VISIBLE_FILES = 10;
static String *g_files = nullptr;
static uint16_t g_fileCount = 0;
static uint16_t g_selectedFile = 0;
static String g_currentPath = "/";

static bool ensureFileBuffer() {
  if (g_files != nullptr) {
    return true;
  }

  g_files = new String[MAX_FILES];
  return g_files != nullptr;
}

static void appendEntry(File &entry) {
  if (g_fileCount >= MAX_FILES) {
    return;
  }

  g_files[g_fileCount] = entry.name();
  if (entry.isDirectory()) {
    g_files[g_fileCount] += "/";
  }
  ++g_fileCount;
}

static bool selectedEntryIsDirectory() {
  return g_fileCount > 0 && g_selectedFile < g_fileCount && g_files[g_selectedFile].endsWith("/");
}

static String selectedEntryPath() {
  if (g_fileCount == 0 || g_selectedFile >= g_fileCount) {
    return g_currentPath;
  }

  String name = g_files[g_selectedFile];
  if (name.endsWith("/")) {
    name.remove(name.length() - 1);
  }
  if (g_currentPath == "/") {
    return "/" + name;
  }
  return g_currentPath + "/" + name;
}

static void navigateToParent() {
  if (g_currentPath == "/") {
    return;
  }

  const int slash = g_currentPath.lastIndexOf('/');
  if (slash <= 0) {
    g_currentPath = "/";
  } else {
    g_currentPath = g_currentPath.substring(0, slash);
  }
}

static void scanCurrentDirectory(AppContext &context) {
  g_fileCount = 0;
  g_selectedFile = 0;
  if (!context.sdReady || !ensureFileBuffer()) {
    return;
  }

  File directory = SD.open(g_currentPath.c_str());
  if (!directory || !directory.isDirectory()) {
    if (directory) {
      directory.close();
    }
    return;
  }

  while (g_fileCount < MAX_FILES) {
    File entry = directory.openNextFile();
    if (!entry) {
      break;
    }
    if (entry.isDirectory()) {
      appendEntry(entry);
    }
    entry.close();
  }

  directory.rewindDirectory();
  while (g_fileCount < MAX_FILES) {
    File entry = directory.openNextFile();
    if (!entry) {
      break;
    }
    if (!entry.isDirectory()) {
      appendEntry(entry);
    }
    entry.close();
  }
  directory.close();
}

static void drawSdBrowser(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(4, 6, "SD Browser", LAVA_BLACK, LAVA_YELLOW);
  lavaDrawText(4, 22, g_currentPath.c_str(), LAVA_CYAN, LAVA_BLACK);

  if (!context.sdReady) {
    lavaDrawText(78, 92, "SD card unavailable", LAVA_RED, LAVA_BLACK);
    lavaDrawText(48, 222, "A:Enter  B:Up  SEL+ST:Exit", LAVA_GRAY, LAVA_BLACK);
    lavaPresent();
    return;
  }

  if (!ensureFileBuffer()) {
    lavaDrawText(98, 92, "Out of memory", LAVA_RED, LAVA_BLACK);
    lavaDrawText(48, 222, "A:Enter  B:Up  SEL+ST:Exit", LAVA_GRAY, LAVA_BLACK);
    lavaPresent();
    return;
  }

  if (g_fileCount == 0) {
    lavaDrawText(128, 92, "No files", LAVA_YELLOW, LAVA_BLACK);
  }

  uint16_t firstVisible = 0;
  if (g_selectedFile >= VISIBLE_FILES) {
    firstVisible = g_selectedFile - VISIBLE_FILES + 1;
  }
  const uint16_t visibleEnd = min<uint16_t>(g_fileCount, firstVisible + VISIBLE_FILES);
  for (uint16_t i = firstVisible; i < visibleEnd; ++i) {
    const int16_t y = 42 + static_cast<int16_t>(i - firstVisible) * 18;
    const bool selected = i == g_selectedFile;
    const char *name = g_files[i].c_str();
    lavaFillRect(4, y - 2, 312, 14, selected ? LAVA_BLUE : LAVA_BLACK);
    lavaDrawText(8, y, name, selected ? LAVA_YELLOW : LAVA_WHITE,
                 selected ? LAVA_BLUE : LAVA_BLACK);
  }

  lavaDrawText(8, 222, "UP/DN Scroll  A:Enter  B:Up", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

static void sdBrowserBegin(AppContext &context) {
  g_currentPath = "/";
  scanCurrentDirectory(context);
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
    if (selectedEntryIsDirectory()) {
      g_currentPath = selectedEntryPath();
    }
    scanCurrentDirectory(context);
    changed = true;
  }
  if (context.buttons[1].pressed) {
    navigateToParent();
    scanCurrentDirectory(context);
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
