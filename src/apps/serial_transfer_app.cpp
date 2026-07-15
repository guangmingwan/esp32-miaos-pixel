#include "apps/serial_transfer_app.h"

#include <Arduino.h>
#include <stdio.h>

#include "lava_native_display.h"
#include "serial_file_service.h"

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

uint32_t g_lastRenderMs = 0;

void drawSerialTransfer(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  const SerialFileServiceSnapshot snapshot = serialFileServiceSnapshot();
  char line[48];
  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(4, 6, "Serial Files", LAVA_BLACK, LAVA_YELLOW);
  lavaDrawText(8, 34, snapshot.sdReady ? "USB serial file service" : "SD unavailable",
               snapshot.sdReady ? LAVA_CYAN : LAVA_RED, LAVA_BLACK);
  lavaDrawText(8, 54, "Host: tools/serial_sd_client.py", LAVA_WHITE, LAVA_BLACK);
  lavaDrawText(8, 74, snapshot.status, LAVA_YELLOW, LAVA_BLACK);
  lavaDrawText(8, 94, snapshot.targetPath, LAVA_GRAY, LAVA_BLACK);
  if (snapshot.uploadActive || snapshot.downloadActive) {
    snprintf(line, sizeof(line), "%lu / %lu bytes", static_cast<unsigned long>(snapshot.transferredBytes),
             static_cast<unsigned long>(snapshot.totalBytes));
    lavaDrawText(8, 114, line, LAVA_GREEN, LAVA_BLACK);
  } else {
    lavaDrawText(8, 114, "Commands: PING LIST MKDIR DELETE PUT GET", LAVA_GREEN, LAVA_BLACK);
  }
  lavaDrawText(8, 222, "SEL+ST:Exit", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

void serialTransferBegin(AppContext &context) {
  serialFileServiceBegin(context.sdReady);
  g_lastRenderMs = 0;
  drawSerialTransfer(context);
}

void serialTransferTick(AppContext &context, uint32_t nowMs) {
  serialFileServiceTick();
  const SerialFileServiceSnapshot snapshot = serialFileServiceSnapshot();
  const uint32_t refreshIntervalMs = (snapshot.uploadActive || snapshot.downloadActive) ? 1000 : 200;
  if (nowMs - g_lastRenderMs >= refreshIntervalMs) {
    g_lastRenderMs = nowMs;
    drawSerialTransfer(context);
  }
}

void serialTransferEnd(AppContext &context) {
  serialFileServiceEnd();
  (void)context;
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

}

const LauncherApp &serialTransferApp() {
  static const LauncherApp app = {"Serial Files", serialTransferBegin, serialTransferTick,
                                  serialTransferEnd};
  return app;
}
