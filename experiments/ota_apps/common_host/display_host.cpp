#include "display_host.h"
#include "mia_host_abi.h"

#include <stddef.h>
#include <stdint.h>

#ifndef MIA_DISPLAY_HOST_NATIVE_TEST

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_attr.h>
#include <esp_heap_caps.h>
#include <esp_rom_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs.h>
#include <string.h>

#ifdef MIA_DISPLAY_DROID_GBK
#include "droid_gbk_renderer.h"
#endif
#endif

using Rgb565ChunkWriter = int32_t (*)(uint32_t, uint32_t, const uint16_t *, size_t, void *);

#ifdef MIA_DISPLAY_HOST_NATIVE_TEST
extern "C" int32_t display_host_test_transport_rgb565(
#else
static int32_t transport_rgb565(
#endif
    const uint16_t *pixels, uint32_t width, uint32_t height, uint32_t pitch_bytes,
    uint8_t ready, uint16_t *staging, uint32_t staging_rows, Rgb565ChunkWriter writer,
    void *context) {
  constexpr uint32_t width_pixels = 320;
  constexpr uint32_t height_pixels = 240;
  if (pixels == nullptr || (reinterpret_cast<uintptr_t>(pixels) & 1u) != 0u ||
      width != width_pixels || height != height_pixels || (pitch_bytes & 1u) != 0u ||
      pitch_bytes < width_pixels * sizeof(uint16_t)) {
    return MIA_HOST_RESULT_INVALID_ARGUMENT;
  }
  if (ready == 0 || staging == nullptr || staging_rows == 0 || writer == nullptr) {
    return MIA_HOST_RESULT_NOT_READY;
  }
  const uint8_t *source = reinterpret_cast<const uint8_t *>(pixels);
  for (uint32_t y = 0; y < height_pixels; y += staging_rows) {
    const uint32_t rows =
        (height_pixels - y) < staging_rows ? height_pixels - y : staging_rows;
    for (uint32_t row = 0; row < rows; ++row) {
      const uint16_t *source_row =
          reinterpret_cast<const uint16_t *>(source + (y + row) * pitch_bytes);
      for (uint32_t x = 0; x < width_pixels; ++x) {
        staging[row * width_pixels + x] = __builtin_bswap16(source_row[x]);
      }
    }
    if (writer(y, rows, staging, rows * width_pixels, context) != MIA_HOST_RESULT_OK) {
      return MIA_HOST_RESULT_IO;
    }
  }
  return MIA_HOST_RESULT_OK;
}

#ifndef MIA_DISPLAY_HOST_NATIVE_TEST

namespace {

constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;
#ifndef MIA_DISPLAY_PRESENT_ROWS
#define MIA_DISPLAY_PRESENT_ROWS 8
#endif
constexpr int PRESENT_ROWS = MIA_DISPLAY_PRESENT_ROWS;
constexpr int PRESENT_PIXELS = SCREEN_W * PRESENT_ROWS;
constexpr gpio_num_t LCD_MISO_PIN = GPIO_NUM_48;
constexpr gpio_num_t LCD_MOSI_PIN = GPIO_NUM_11;
constexpr gpio_num_t LCD_CLK_PIN = GPIO_NUM_12;
constexpr gpio_num_t LCD_CS_PIN = GPIO_NUM_10;
constexpr gpio_num_t LCD_DC_PIN = GPIO_NUM_9;
constexpr gpio_num_t LCD_BL_PIN = GPIO_NUM_13;
constexpr gpio_num_t LCD_RST_PIN = GPIO_NUM_3;

struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

static spi_device_handle_t g_lcd = nullptr;
static bool g_ready = false;
static uint8_t *g_pixels = nullptr;
DMA_ATTR static uint16_t g_chunk[PRESENT_PIXELS];
static Color g_palette[256];
#ifdef MIA_DISPLAY_DROID_GBK
static bool g_use_droid_gbk = false;
#endif

static const uint8_t FONT[][5] = {
    {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},{0x14,0x7F,0x14,0x7F,0x14},
    {0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},
    {0x00,0x1C,0x22,0x41,0x00},{0x00,0x41,0x22,0x1C,0x00},{0x14,0x08,0x3E,0x08,0x14},{0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},{0x20,0x10,0x08,0x04,0x02},
    {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},{0x00,0x56,0x36,0x00,0x00},
    {0x00,0x08,0x14,0x22,0x41},{0x14,0x14,0x14,0x14,0x14},{0x41,0x22,0x14,0x08,0x00},{0x02,0x01,0x51,0x09,0x06},
    {0x32,0x49,0x79,0x41,0x3E},{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x01,0x01},{0x3E,0x41,0x41,0x51,0x32},
    {0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40},{0x7F,0x02,0x04,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},{0x7F,0x20,0x18,0x20,0x7F},
    {0x63,0x14,0x08,0x14,0x63},{0x03,0x04,0x78,0x04,0x03},{0x61,0x51,0x49,0x45,0x43},{0x00,0x7F,0x41,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x41,0x7F,0x00},{0x04,0x02,0x01,0x02,0x04},{0x40,0x40,0x40,0x40,0x40},
    {0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},{0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},
    {0x38,0x44,0x44,0x48,0x7F},{0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x08,0x14,0x54,0x54,0x3C},
    {0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},{0x00,0x7F,0x10,0x28,0x44},
    {0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x18,0x04,0x78},{0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},
    {0x7C,0x14,0x14,0x14,0x08},{0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},{0x3C,0x40,0x30,0x40,0x3C},
    {0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},{0x44,0x64,0x54,0x4C,0x44},{0x00,0x08,0x36,0x41,0x00},
    {0x00,0x00,0x7F,0x00,0x00},{0x00,0x41,0x36,0x08,0x00},{0x02,0x01,0x02,0x04,0x02},
};

static uint16_t to_rgb565(const Color &c) {
  return (uint16_t)(((c.r & 0xF8) << 8) | ((c.g & 0xFC) << 3) | (c.b >> 3));
}

static void yield_once() {
  vTaskDelay(pdMS_TO_TICKS(1));
}

static esp_err_t tx_bytes(int dc, const void *data, size_t len) {
  spi_transaction_t t = {};
  gpio_set_level(LCD_DC_PIN, dc);
  gpio_set_level(LCD_CS_PIN, 0);
  t.length = len * 8;
  t.tx_buffer = data;
  esp_err_t err = spi_device_polling_transmit(g_lcd, &t);
  gpio_set_level(LCD_CS_PIN, 1);
  return err;
}

static esp_err_t tx_cmd(uint8_t cmd) { return tx_bytes(0, &cmd, 1); }

static void lcd_reset() {
  gpio_set_level(LCD_RST_PIN, 1);
  vTaskDelay(pdMS_TO_TICKS(1));
  gpio_set_level(LCD_RST_PIN, 0);
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(LCD_RST_PIN, 1);
  vTaskDelay(pdMS_TO_TICKS(120));
}

static esp_err_t lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  uint8_t buf[4];
  ESP_ERROR_CHECK(tx_cmd(0x2A));
  buf[0] = (uint8_t)(x0 >> 8);
  buf[1] = (uint8_t)x0;
  buf[2] = (uint8_t)(x1 >> 8);
  buf[3] = (uint8_t)x1;
  ESP_ERROR_CHECK(tx_bytes(1, buf, 4));
  ESP_ERROR_CHECK(tx_cmd(0x2B));
  buf[0] = (uint8_t)(y0 >> 8);
  buf[1] = (uint8_t)y0;
  buf[2] = (uint8_t)(y1 >> 8);
  buf[3] = (uint8_t)y1;
  ESP_ERROR_CHECK(tx_bytes(1, buf, 4));
  return tx_cmd(0x2C);
}

static esp_err_t lcd_set_full_width_window(uint16_t y0, uint16_t y1) {
  const uint8_t columns[] = {0, 0, (uint8_t)((SCREEN_W - 1) >> 8), (uint8_t)(SCREEN_W - 1)};
  const uint8_t rows[] = {(uint8_t)(y0 >> 8), (uint8_t)y0, (uint8_t)(y1 >> 8), (uint8_t)y1};
  esp_err_t err = tx_cmd(0x2A);
  if (err == ESP_OK) err = tx_bytes(1, columns, sizeof(columns));
  if (err == ESP_OK) err = tx_cmd(0x2B);
  if (err == ESP_OK) err = tx_bytes(1, rows, sizeof(rows));
  if (err == ESP_OK) err = tx_cmd(0x2C);
  return err;
}

static int32_t write_rgb565_chunk(uint32_t y, uint32_t rows, const uint16_t *wire_pixels,
                                  size_t pixel_count, void *) {
  if (lcd_set_full_width_window((uint16_t)y, (uint16_t)(y + rows - 1)) != ESP_OK ||
      tx_bytes(1, wire_pixels, pixel_count * sizeof(uint16_t)) != ESP_OK) {
    return MIA_HOST_RESULT_IO;
  }
  yield_once();
  return MIA_HOST_RESULT_OK;
}

}

extern "C" int display_host_init(void) {
  spi_bus_config_t bus = {};
  spi_device_interface_config_t dev = {};
  static const Color defaults[] = {
      {0, 0, 0}, {255, 255, 255}, {0, 96, 255}, {0, 220, 80},
      {255, 48, 48}, {255, 220, 0}, {0, 220, 220}, {120, 120, 120}, {24, 24, 56},
  };
  static const uint8_t pwctrB[] = {0xFF, 0x93, 0x42};
  static const uint8_t madctl[] = {0xC8};
  static const uint8_t pixfmt[] = {0x55};
  static const uint8_t pwctr1[] = {0x10, 0x10};
  static const uint8_t pwctr2[] = {0x01};
  static const uint8_t vmctr[] = {0xCD};
  static const uint8_t frmctr1[] = {0x00, 0x1B};
  static const uint8_t invctr[] = {0x02};
  static const uint8_t gammaPos[] = {0x0F,0x14,0x17,0x07,0x16,0x0A,0x3F,0x68,0x4C,0x06,0x0F,0x0D,0x18,0x1A,0x00};
  static const uint8_t gammaNeg[] = {0x00,0x29,0x29,0x04,0x0F,0x04,0x3C,0x24,0x4B,0x02,0x0B,0x09,0x32,0x37,0x0F};

  if (g_ready) {
    return 1;
  }
#ifdef MIA_DISPLAY_DROID_GBK
  g_use_droid_gbk = mia_host_language() == 1;
  nvs_handle_t font_store;
  if (nvs_open("lava-text", NVS_READONLY, &font_store) == ESP_OK) {
    uint8_t font = 0;
    if (nvs_get_u8(font_store, "font", &font) == ESP_OK && font == 8) g_use_droid_gbk = true;
    nvs_close(font_store);
  }
#endif
  bus.mosi_io_num = LCD_MOSI_PIN;
  bus.miso_io_num = LCD_MISO_PIN;
  bus.sclk_io_num = LCD_CLK_PIN;
  bus.quadwp_io_num = -1;
  bus.quadhd_io_num = -1;
  bus.max_transfer_sz = PRESENT_PIXELS * 2 + 8;
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));

  dev.clock_speed_hz = 40000000;
  dev.mode = 0;
  dev.spics_io_num = -1;
  dev.queue_size = 1;
  ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &g_lcd));

  gpio_reset_pin(LCD_CS_PIN);
  gpio_set_direction(LCD_CS_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(LCD_CS_PIN, 1);
  gpio_reset_pin(LCD_DC_PIN);
  gpio_set_direction(LCD_DC_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(LCD_DC_PIN, 1);
  gpio_reset_pin(LCD_RST_PIN);
  gpio_set_direction(LCD_RST_PIN, GPIO_MODE_OUTPUT);
  gpio_reset_pin(LCD_BL_PIN);
  gpio_set_direction(LCD_BL_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(LCD_BL_PIN, 0);

  lcd_reset();
  ESP_ERROR_CHECK(tx_cmd(0xC8));
  ESP_ERROR_CHECK(tx_bytes(1, pwctrB, sizeof(pwctrB)));
  ESP_ERROR_CHECK(tx_cmd(0x36));
  ESP_ERROR_CHECK(tx_bytes(1, madctl, sizeof(madctl)));
  ESP_ERROR_CHECK(tx_cmd(0x3A));
  ESP_ERROR_CHECK(tx_bytes(1, pixfmt, sizeof(pixfmt)));
  ESP_ERROR_CHECK(tx_cmd(0xC0));
  ESP_ERROR_CHECK(tx_bytes(1, pwctr1, sizeof(pwctr1)));
  ESP_ERROR_CHECK(tx_cmd(0xC1));
  ESP_ERROR_CHECK(tx_bytes(1, pwctr2, sizeof(pwctr2)));
  ESP_ERROR_CHECK(tx_cmd(0xC5));
  ESP_ERROR_CHECK(tx_bytes(1, vmctr, sizeof(vmctr)));
  ESP_ERROR_CHECK(tx_cmd(0xB1));
  ESP_ERROR_CHECK(tx_bytes(1, frmctr1, sizeof(frmctr1)));
  ESP_ERROR_CHECK(tx_cmd(0xB4));
  ESP_ERROR_CHECK(tx_bytes(1, invctr, sizeof(invctr)));
  ESP_ERROR_CHECK(tx_cmd(0xE0));
  ESP_ERROR_CHECK(tx_bytes(1, gammaPos, sizeof(gammaPos)));
  ESP_ERROR_CHECK(tx_cmd(0xE1));
  ESP_ERROR_CHECK(tx_bytes(1, gammaNeg, sizeof(gammaNeg)));
  ESP_ERROR_CHECK(tx_cmd(0x11));
  vTaskDelay(pdMS_TO_TICKS(120));
  ESP_ERROR_CHECK(tx_cmd(0x29));

  g_pixels = (uint8_t *)heap_caps_malloc(SCREEN_W * SCREEN_H, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (g_pixels == nullptr) {
    return 0;
  }
  memset(g_pixels, 0, SCREEN_W * SCREEN_H);
  memset(g_palette, 0, sizeof(g_palette));
  for (size_t i = 0; i < sizeof(defaults) / sizeof(defaults[0]); ++i) {
    g_palette[i] = defaults[i];
  }
  g_ready = true;
  return 1;
}

extern "C" int display_host_ready(void) { return g_ready ? 1 : 0; }
extern "C" int32_t display_host_width(void) { return SCREEN_W; }
extern "C" int32_t display_host_height(void) { return SCREEN_H; }
extern "C" void display_host_backlight_set(uint8_t enabled) { gpio_set_level(LCD_BL_PIN, enabled ? 0 : 1); }

extern "C" void display_host_clear(uint8_t color) {
  if (g_pixels != nullptr) {
    memset(g_pixels, color, SCREEN_W * SCREEN_H);
  }
}

extern "C" void display_host_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color) {
  if (g_pixels == nullptr || w <= 0 || h <= 0) {
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
  if (x + w > SCREEN_W) {
    w = SCREEN_W - x;
  }
  if (y + h > SCREEN_H) {
    h = SCREEN_H - y;
  }
  if (w <= 0 || h <= 0) {
    return;
  }
  for (int32_t row = 0; row < h; ++row) {
    memset(g_pixels + (y + row) * SCREEN_W + x, color, (size_t)w);
    if (((row + 1) % 8) == 0) {
      yield_once();
    }
  }
}

extern "C" void display_host_fill_screen_rgb565(uint16_t color) {
  uint16_t fill[PRESENT_PIXELS];
  for (int i = 0; i < PRESENT_PIXELS; ++i) {
    fill[i] = color;
  }
  for (int y = 0; y < SCREEN_H; y += PRESENT_ROWS) {
    int rows = (SCREEN_H - y) < PRESENT_ROWS ? (SCREEN_H - y) : PRESENT_ROWS;
    ESP_ERROR_CHECK(lcd_set_window(0, (uint16_t)y, (uint16_t)(SCREEN_W - 1), (uint16_t)(y + rows - 1)));
    ESP_ERROR_CHECK(tx_bytes(1, fill, (size_t)(rows * SCREEN_W * 2)));
    yield_once();
  }
}

static void draw_char(int32_t x, int32_t y, char ch, uint8_t fg, uint8_t bg) {
  if (g_pixels == nullptr) {
    return;
  }
  uint8_t idx = (uint8_t)ch;
  if (idx < 32 || idx > 126) {
    return;
  }
  const uint8_t *glyph = FONT[idx - 32];
  for (int32_t col = 0; col < 5; ++col) {
    uint8_t bits = glyph[col];
    for (int32_t row = 0; row < 7; ++row) {
      int32_t px = x + col;
      int32_t py = y + row;
      if (px < 0 || px >= SCREEN_W || py < 0 || py >= SCREEN_H) {
        continue;
      }
      g_pixels[py * SCREEN_W + px] = ((bits >> row) & 1u) ? fg : bg;
    }
  }
}

extern "C" void display_host_draw_text(int32_t x, int32_t y, const char *text, uint8_t fg, uint8_t bg) {
  if (text == nullptr) {
    return;
  }
#ifdef MIA_DISPLAY_DROID_GBK
  if (g_use_droid_gbk) {
    droid_gbk_draw_text(g_pixels, SCREEN_W, SCREEN_H, x, y, text, fg, bg);
    return;
  }
#endif
  for (int32_t i = 0; text[i] != '\0'; ++i) {
    draw_char(x + i * 6, y, text[i], fg, bg);
    if (((i + 1) % 8) == 0) {
      yield_once();
    }
  }
}

extern "C" void display_host_present(void) {
  if (!g_ready || g_pixels == nullptr) {
    return;
  }
  for (int y = 0; y < SCREEN_H; y += PRESENT_ROWS) {
    int rows = (SCREEN_H - y) < PRESENT_ROWS ? (SCREEN_H - y) : PRESENT_ROWS;
    size_t pixels = (size_t)rows * SCREEN_W;
    for (int row = 0; row < rows; ++row) {
      size_t src = (size_t)(y + row) * SCREEN_W;
      size_t dst = (size_t)row * SCREEN_W;
      for (int x = 0; x < SCREEN_W; ++x) {
        g_chunk[dst + x] = to_rgb565(g_palette[g_pixels[src + x]]);
      }
    }
    ESP_ERROR_CHECK(lcd_set_window(0, (uint16_t)y, (uint16_t)(SCREEN_W - 1), (uint16_t)(y + rows - 1)));
    ESP_ERROR_CHECK(tx_bytes(1, g_chunk, pixels * 2));
    yield_once();
  }
}

extern "C" int32_t display_host_present_rgb565(const uint16_t *pixels, uint32_t width,
                                                  uint32_t height, uint32_t pitch_bytes) {
  return transport_rgb565(
      pixels, width, height, pitch_bytes, g_ready && g_lcd != nullptr ? 1 : 0, g_chunk,
      PRESENT_ROWS, write_rgb565_chunk, nullptr);
}

extern "C" int32_t display_host_present_rgb565_region(const uint16_t *pixels, int32_t x,
                                                         int32_t y, uint32_t width,
                                                         uint32_t height,
                                                         uint32_t pitch_bytes) {
  if (!g_ready || g_lcd == nullptr) return MIA_HOST_RESULT_NOT_READY;
  if (pixels == nullptr || (reinterpret_cast<uintptr_t>(pixels) & 1u) != 0u || x < 0 ||
      y < 0 || width == 0 || height == 0 || x + width > SCREEN_W || y + height > SCREEN_H ||
      (pitch_bytes & 1u) != 0u || pitch_bytes < width * sizeof(uint16_t)) {
    return MIA_HOST_RESULT_INVALID_ARGUMENT;
  }
  const uint8_t *source = reinterpret_cast<const uint8_t *>(pixels);
  for (uint32_t row_start = 0; row_start < height; row_start += PRESENT_ROWS) {
    const uint32_t rows =
        (height - row_start) < PRESENT_ROWS ? height - row_start : PRESENT_ROWS;
    for (uint32_t row = 0; row < rows; ++row) {
      const uint16_t *source_row = reinterpret_cast<const uint16_t *>(
          source + (row_start + row) * pitch_bytes);
      for (uint32_t column = 0; column < width; ++column) {
        g_chunk[row * width + column] = __builtin_bswap16(source_row[column]);
      }
    }
    if (lcd_set_window((uint16_t)x, (uint16_t)(y + row_start),
                       (uint16_t)(x + width - 1),
                       (uint16_t)(y + row_start + rows - 1)) != ESP_OK ||
        tx_bytes(1, g_chunk, (size_t)rows * width * sizeof(uint16_t)) != ESP_OK) {
      return MIA_HOST_RESULT_IO;
    }
    yield_once();
  }
  return MIA_HOST_RESULT_OK;
}
#endif
