#include "mia_host_abi.h"

#include <Arduino.h>
#include <stdint.h>

#include "lava_native_display.h"

uint32_t mia_host_abi_version(void) { return 1; }

void mia_host_log(const char *message) {
  Serial.printf("[mia_host_abi] %s\n", message == nullptr ? "<null>" : message);
}

int32_t mia_host_screen_width(void) { return LAVA_SCREEN_W; }

int32_t mia_host_screen_height(void) { return LAVA_SCREEN_H; }

void mia_host_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color) {
  Serial.printf("[mia_host_abi] fill_rect x=%d y=%d w=%d h=%d color=%u ready=%d\n", x, y,
                w, h, static_cast<unsigned>(color), lavaDisplayReady() ? 1 : 0);
  if (!lavaDisplayReady()) {
    Serial.println("[mia_host_abi] fill_rect skipped: display not ready");
    return;
  }
  if (w <= 0 || h <= 0 || x >= LAVA_SCREEN_W || y >= LAVA_SCREEN_H) {
    Serial.println("[mia_host_abi] fill_rect skipped: outside screen");
    return;
  }
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > LAVA_SCREEN_W) {
    w = LAVA_SCREEN_W - x;
  }
  if (y + h > LAVA_SCREEN_H) {
    h = LAVA_SCREEN_H - y;
  }
  if (w <= 0 || h <= 0) {
    Serial.println("[mia_host_abi] fill_rect skipped: clipped empty");
    return;
  }
  lavaFillRect(static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(w),
               static_cast<int16_t>(h), color);
  Serial.println("[mia_host_abi] fill_rect done");
}

void mia_host_draw_text(int32_t x, int32_t y, const char *text, uint8_t fg,
                        uint8_t bg) {
  Serial.printf("[mia_host_abi] draw_text x=%d y=%d text='%s' fg=%u bg=%u ready=%d\n", x,
                y, text == nullptr ? "<null>" : text, static_cast<unsigned>(fg),
                static_cast<unsigned>(bg), lavaDisplayReady() ? 1 : 0);
  if (!lavaDisplayReady()) {
    Serial.println("[mia_host_abi] draw_text skipped: display not ready");
    return;
  }
  if (text == nullptr || x >= LAVA_SCREEN_W || y < -7 || y >= LAVA_SCREEN_H) {
    Serial.println("[mia_host_abi] draw_text skipped: invalid args");
    return;
  }
  if (x < -LAVA_SCREEN_W) {
    x = -LAVA_SCREEN_W;
  }
  lavaDrawText(static_cast<int16_t>(x), static_cast<int16_t>(y), text, fg, bg);
  Serial.println("[mia_host_abi] draw_text done");
}

void mia_host_present(void) {
  Serial.printf("[mia_host_abi] present ready=%d\n", lavaDisplayReady() ? 1 : 0);
  if (lavaDisplayReady()) {
    lavaPresent();
    Serial.println("[mia_host_abi] present done");
  } else {
    Serial.println("[mia_host_abi] present skipped: display not ready");
  }
}
