#include "lava_native_display.h"

#include <algorithm>
#include <cstring>

#include <esp_timer.h>

#include "esp_heap_caps.h"
#include "lcd_ili9342.h"
#include "lava_text.h"

extern "C" __attribute__((weak)) void esp32_task_wdt_reset(void) {
  static int64_t last_us = 0;
  const int64_t now_us = esp_timer_get_time();
  if (now_us - last_us >= 500000) {
    delay(1);
    last_us = now_us;
  }
}

static constexpr size_t PIXEL_COUNT = LAVA_SCREEN_W * LAVA_SCREEN_H;
static constexpr int16_t PRESENT_ROWS_PER_CHUNK = 8;
static constexpr size_t PRESENT_CHUNK_PIXELS = LAVA_SCREEN_W * PRESENT_ROWS_PER_CHUNK;
static constexpr int16_t DRAW_ROWS_PER_YIELD = 8;

static uint8_t *g_pixels = nullptr;
static uint16_t g_rgb565Chunk[PRESENT_CHUNK_PIXELS];
static LavaColor g_palette[256];
static LavaSurface g_screen = {LAVA_SCREEN_W, LAVA_SCREEN_H, LAVA_SCREEN_W, nullptr};
static bool g_displayReady = false;

static inline void lavaRenderYield() {
  esp32_task_wdt_reset();
}

static uint16_t toRgb565(const LavaColor &color) {
  return ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3);
}

void lavaDisplayInit() {
  if (g_pixels == nullptr) {
    g_pixels = static_cast<uint8_t *>(heap_caps_malloc(PIXEL_COUNT, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    g_screen.pixels = g_pixels;
  }
  if (g_pixels == nullptr) {
    g_displayReady = false;
    return;
  }

  const LavaColor defaultPalette[] = {
      {0, 0, 0},       {255, 255, 255}, {0, 96, 255},   {0, 220, 80},
      {255, 48, 48},   {255, 220, 0},   {0, 220, 220},  {120, 120, 120},
      {24, 24, 56},    {32, 64, 120},   {12, 128, 44},  {128, 20, 20},
      {168, 124, 0},   {0, 112, 112},   {64, 64, 64},   {210, 210, 210},
  };
  lavaSetPalette(0, sizeof(defaultPalette) / sizeof(defaultPalette[0]), defaultPalette);
  lavaClear(0);
  g_displayReady = true;
}

bool lavaDisplayReady() { return g_displayReady; }

void lavaSetPalette(uint8_t first, uint8_t count, const LavaColor *colors) {
  if (colors == nullptr) {
    return;
  }

  for (uint16_t i = 0; i < count && first + i < 256; ++i) {
    g_palette[first + i] = colors[i];
  }
}

LavaSurface &lavaScreen() { return g_screen; }

void lavaClear(uint8_t color) {
  if (g_pixels == nullptr) {
    return;
  }
  memset(g_pixels, color, PIXEL_COUNT);
}

void lavaFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
  if (g_pixels == nullptr) {
    return;
  }
  if (w <= 0 || h <= 0 || x >= LAVA_SCREEN_W || y >= LAVA_SCREEN_H) {
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
    return;
  }

  for (int16_t row = 0; row < h; ++row) {
    memset(g_pixels + (y + row) * LAVA_SCREEN_W + x, color, w);
    if (((row + 1) % DRAW_ROWS_PER_YIELD) == 0) {
      lavaRenderYield();
    }
  }
}

void lavaDrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
  lavaFillRect(x, y, w, 1, color);
  lavaFillRect(x, y + h - 1, w, 1, color);
  lavaFillRect(x, y, 1, h, color);
  lavaFillRect(x + w - 1, y, 1, h, color);
}

void lavaDrawChar(int16_t x, int16_t y, char ch, uint8_t fg, uint8_t bg) {
  const char text[2] = {ch, '\0'};
  lavaDrawText(x, y, text, fg, bg);
}

void lavaDrawText(int16_t x, int16_t y, const char *text, uint8_t fg, uint8_t bg) {
  if (g_pixels != nullptr) {
    lavaDrawTextUtf8(g_screen, x, y, text, fg, bg);
  }
}

int16_t lavaTextYCentered(int16_t areaY, int16_t areaHeight) {
  const int16_t offset = (areaHeight - lavaFontHeight()) / 2;
  return areaY + (offset > 0 ? offset : 0);
}

void lavaPresent() {
  if (!g_displayReady) {
    return;
  }
  for (int16_t y = 0; y < LAVA_SCREEN_H; y += PRESENT_ROWS_PER_CHUNK) {
    const int16_t rows = std::min<int16_t>(PRESENT_ROWS_PER_CHUNK, LAVA_SCREEN_H - y);
    const size_t chunkPixels = static_cast<size_t>(LAVA_SCREEN_W) * rows;

    for (int16_t row = 0; row < rows; ++row) {
      const size_t srcOffset = static_cast<size_t>(y + row) * LAVA_SCREEN_W;
      const size_t dstOffset = static_cast<size_t>(row) * LAVA_SCREEN_W;
      for (int16_t x = 0; x < LAVA_SCREEN_W; ++x) {
        g_rgb565Chunk[dstOffset + x] = toRgb565(g_palette[g_pixels[srcOffset + x]]);
      }
    }

    Lcd.setWindow(0, y, LAVA_SCREEN_W - 1, y + rows - 1);
    Lcd.pushColors(g_rgb565Chunk, chunkPixels);
    lavaRenderYield();
  }
}
