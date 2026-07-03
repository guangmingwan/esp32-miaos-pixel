#include "apps/log_viewer_app.h"

#include <Arduino.h>
#include <string.h>

#include "launcher_log.h"
#include "lava_native_display.h"

namespace {

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

constexpr uint8_t MAX_LOG_LINES = 48;
constexpr uint16_t LOG_BUFFER_SIZE = 2048;
constexpr uint8_t VISIBLE_LOG_LINES = 9;
char g_logBuffer[LOG_BUFFER_SIZE];
char *g_logLines[MAX_LOG_LINES];
uint8_t g_logLineCount = 0;
uint8_t g_firstVisibleLogLine = 0;
bool g_logLoaded = false;

void splitLogBuffer() {
  g_logLineCount = 0;
  if (g_logBuffer[0] == '\0') {
    return;
  }

  g_logLines[g_logLineCount++] = g_logBuffer;
  for (uint16_t index = 0; g_logBuffer[index] != '\0' && g_logLineCount < MAX_LOG_LINES; ++index) {
    if (g_logBuffer[index] == '\n' || g_logBuffer[index] == '\r') {
      g_logBuffer[index] = '\0';
      if (g_logBuffer[index + 1] != '\0') {
        g_logLines[g_logLineCount++] = &g_logBuffer[index + 1];
      }
    }
  }
}

void loadLogFile(AppContext &context) {
  g_firstVisibleLogLine = 0;
  g_logLoaded = false;
  g_logBuffer[0] = '\0';
  if (!context.sdReady) {
    strncpy(g_logBuffer, "SD unavailable", sizeof(g_logBuffer) - 1);
    g_logBuffer[sizeof(g_logBuffer) - 1] = '\0';
    splitLogBuffer();
    return;
  }

  if (launcherLogRead(g_logBuffer, sizeof(g_logBuffer))) {
    g_logLoaded = true;
  } else {
    strncpy(g_logBuffer, "No launcher log on SD", sizeof(g_logBuffer) - 1);
    g_logBuffer[sizeof(g_logBuffer) - 1] = '\0';
  }
  splitLogBuffer();
}

void drawLogViewer(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(4, 6, "Logs", LAVA_BLACK, LAVA_YELLOW);
  lavaDrawText(4, 24, launcherLogPath(), LAVA_CYAN, LAVA_BLACK);

  if (g_logLineCount == 0) {
    lavaDrawText(92, 96, "No log lines", LAVA_YELLOW, LAVA_BLACK);
  } else {
    const uint8_t visibleEnd = min<uint8_t>(g_logLineCount, g_firstVisibleLogLine + VISIBLE_LOG_LINES);
    for (uint8_t index = g_firstVisibleLogLine; index < visibleEnd; ++index) {
      const int16_t y = 44 + static_cast<int16_t>(index - g_firstVisibleLogLine) * 18;
      lavaDrawText(8, y, g_logLines[index], index == 0 && g_logLoaded ? LAVA_WHITE : LAVA_GRAY,
                   LAVA_BLACK);
    }
  }

  lavaDrawText(8, 222, "A:Reload UP/DN:Scroll SEL+ST:Exit", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

void logViewerBegin(AppContext &context) {
  loadLogFile(context);
  drawLogViewer(context);
}

void logViewerTick(AppContext &context, uint32_t nowMs) {
  (void)nowMs;
  if (context.buttons[2].pressed && g_firstVisibleLogLine > 0) {
    --g_firstVisibleLogLine;
    drawLogViewer(context);
  }
  if (context.buttons[3].pressed && g_firstVisibleLogLine + VISIBLE_LOG_LINES < g_logLineCount) {
    ++g_firstVisibleLogLine;
    drawLogViewer(context);
  }
  if (context.buttons[0].pressed) {
    loadLogFile(context);
    drawLogViewer(context);
  }
}

void logViewerEnd(AppContext &context) {
  (void)context;
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

}

const LauncherApp &logViewerApp() {
  static const LauncherApp app = {"Logs", logViewerBegin, logViewerTick, logViewerEnd};
  return app;
}
