#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_rom_crc.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_attr.h"

static const char *TAG = "startup-menu";

#define SCREEN_W 320
#define SCREEN_H 240
#define PRESENT_ROWS 8
#define PRESENT_PIXELS (SCREEN_W * PRESENT_ROWS)
#define TRANSPARENT_INDEX UINT8_MAX

#define LCD_MOSI_PIN  GPIO_NUM_11
#define LCD_CLK_PIN   GPIO_NUM_12
#define LCD_MISO_PIN  GPIO_NUM_48
#define LCD_CS_PIN    GPIO_NUM_10
#define LCD_DC_PIN    GPIO_NUM_9
#define LCD_RST_PIN   GPIO_NUM_3
#define LCD_BL_PIN    GPIO_NUM_13

#define HC165_PL_PIN   GPIO_NUM_2
#define HC165_CLK_PIN  GPIO_NUM_39
#define HC165_DAT_PIN  GPIO_NUM_38

#define KEY_M_PIN      GPIO_NUM_8
#define KEY_SELECT_PIN GPIO_NUM_21
#define KEY_START_PIN  GPIO_NUM_0

#define HC165_LEFT  0
#define HC165_DOWN  1
#define HC165_UP    2
#define HC165_A     6
#define HC165_B     7

static spi_device_handle_t g_lcd;

typedef struct { uint8_t r, g, b; } color_t;
static color_t g_palette[17];
static uint8_t g_pixels[SCREEN_W * SCREEN_H];
DMA_ATTR static uint16_t g_chunk[PRESENT_PIXELS];

extern const uint8_t recover_small_bmp_start[] asm("_binary_recover_small_bmp_start");
extern const uint8_t recover_small_bmp_end[] asm("_binary_recover_small_bmp_end");

static const uint8_t *g_background_pixels;
static uint16_t g_background_palette[256];
static int g_background_x;
static int g_background_width;
static int g_background_height;
static size_t g_background_stride;
static bool g_background_top_down;

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

static const char *const MENU_ITEMS[] = {"Clear NVRAM", "Boot ota_0", "Boot ota_1"};
#define MENU_COUNT 3

static uint16_t to_rgb565(const color_t *c) {
    return (uint16_t)(((c->r & 0xF8) << 8) | ((c->g & 0xFC) << 3) | (c->b >> 3));
}

static uint16_t read_le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void background_init(void) {
    const uint8_t *bmp = recover_small_bmp_start;
    const size_t bmp_size = (size_t)(recover_small_bmp_end - recover_small_bmp_start);
    if (bmp_size < 54 || bmp[0] != 'B' || bmp[1] != 'M') {
        ESP_LOGW(TAG, "invalid recovery background BMP header");
        return;
    }

    const uint32_t pixel_offset = read_le32(bmp + 10);
    const uint32_t dib_size = read_le32(bmp + 14);
    const int32_t width = (int32_t)read_le32(bmp + 18);
    const int32_t signed_height = (int32_t)read_le32(bmp + 22);
    const uint32_t compression = read_le32(bmp + 30);
    uint32_t palette_count = read_le32(bmp + 46);
    if (palette_count == 0) palette_count = 256;

    if (width <= 0 || width > SCREEN_W || signed_height == 0 ||
        signed_height < -SCREEN_H || signed_height > SCREEN_H) {
        ESP_LOGW(TAG, "unsupported recovery background BMP dimensions");
        return;
    }

    const int32_t height = signed_height < 0 ? -signed_height : signed_height;
    const size_t stride = ((size_t)width + 3u) & ~3u;
    const size_t palette_offset = 14u + dib_size;
    if (dib_size < 40 || read_le16(bmp + 26) != 1 || read_le16(bmp + 28) != 8 ||
        compression != 0 ||
        palette_count > 256 || palette_offset + palette_count * 4u > pixel_offset ||
        pixel_offset > bmp_size || stride * (size_t)height > bmp_size - pixel_offset) {
        ESP_LOGW(TAG, "unsupported recovery background BMP");
        return;
    }

    for (uint32_t i = 0; i < palette_count; ++i) {
        const uint8_t *entry = bmp + palette_offset + i * 4u;
        const color_t color = {entry[2], entry[1], entry[0]};
        g_background_palette[i] = to_rgb565(&color);
    }
    for (uint32_t i = palette_count; i < 256; ++i) {
        g_background_palette[i] = 0;
    }

    g_background_pixels = bmp + pixel_offset;
    g_background_width = width;
    g_background_height = height;
    g_background_stride = stride;
    g_background_top_down = signed_height < 0;
    g_background_x = SCREEN_W - width;
}

static bool background_color_at(int x, int y, uint16_t *color) {
    if (g_background_pixels == NULL || x < g_background_x ||
        x >= g_background_x + g_background_width || y < 0 || y >= g_background_height) {
        return false;
    }

    const int source_y = g_background_top_down ? y : g_background_height - 1 - y;
    const uint8_t index = g_background_pixels[(size_t)source_y * g_background_stride +
                                               (size_t)(x - g_background_x)];
    *color = g_background_palette[index];
    return true;
}

static void tx_bytes(int dc, const void *data, size_t len) {
    spi_transaction_t t = {};
    gpio_set_level(LCD_DC_PIN, dc);
    gpio_set_level(LCD_CS_PIN, 0);
    t.length = len * 8;
    t.tx_buffer = data;
    spi_device_polling_transmit(g_lcd, &t);
    gpio_set_level(LCD_CS_PIN, 1);
}

static void tx_cmd(uint8_t cmd) { tx_bytes(0, &cmd, 1); }

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t buf[4];
    tx_cmd(0x2A);
    buf[0] = x0 >> 8; buf[1] = x0; buf[2] = x1 >> 8; buf[3] = x1;
    tx_bytes(1, buf, 4);
    tx_cmd(0x2B);
    buf[0] = y0 >> 8; buf[1] = y0; buf[2] = y1 >> 8; buf[3] = y1;
    tx_bytes(1, buf, 4);
    tx_cmd(0x2C);
}

static void lcd_init(void) {
    static const color_t defaults[] = {
        {0,0,0},{255,255,255},{0,96,255},{0,220,80},
        {255,48,48},{255,220,0},{0,220,220},{120,120,120},{24,24,56},
        {150,70,200},{210,210,210},{55,55,55},{170,170,170},
        {255,165,40},{255,175,195},{230,200,100},{255,230,180},
    };
    memcpy(g_palette, defaults, sizeof(defaults));

    spi_bus_config_t bus = {};
    bus.mosi_io_num = LCD_MOSI_PIN;
    bus.miso_io_num = LCD_MISO_PIN;
    bus.sclk_io_num = LCD_CLK_PIN;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = PRESENT_PIXELS * 2 + 8;
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev = {};
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
    gpio_reset_pin(LCD_RST_PIN);
    gpio_set_direction(LCD_RST_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LCD_BL_PIN);
    gpio_set_direction(LCD_BL_PIN, GPIO_MODE_OUTPUT);

    gpio_set_level(LCD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(LCD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(LCD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    static const uint8_t pwctrB[] = {0xFF,0x93,0x42};
    static const uint8_t madctl[] = {0xC8};
    static const uint8_t pixfmt[] = {0x55};
    static const uint8_t pwctr1[] = {0x10,0x10};
    static const uint8_t pwctr2[] = {0x01};
    static const uint8_t vmctr[] = {0xCD};
    static const uint8_t frmctr1[] = {0x00,0x1B};
    static const uint8_t invctr[] = {0x02};
    static const uint8_t gammaPos[] = {0x0F,0x14,0x17,0x07,0x16,0x0A,0x3F,0x68,0x4C,0x06,0x0F,0x0D,0x18,0x1A,0x00};
    static const uint8_t gammaNeg[] = {0x00,0x29,0x29,0x04,0x0F,0x04,0x3C,0x24,0x4B,0x02,0x0B,0x09,0x32,0x37,0x0F};

    tx_cmd(0xC8);  tx_bytes(1, pwctrB, sizeof(pwctrB));
    tx_cmd(0x36);  tx_bytes(1, madctl, sizeof(madctl));
    tx_cmd(0x3A);  tx_bytes(1, pixfmt, sizeof(pixfmt));
    tx_cmd(0xC0);  tx_bytes(1, pwctr1, sizeof(pwctr1));
    tx_cmd(0xC1);  tx_bytes(1, pwctr2, sizeof(pwctr2));
    tx_cmd(0xC5);  tx_bytes(1, vmctr, sizeof(vmctr));
    tx_cmd(0xB1);  tx_bytes(1, frmctr1, sizeof(frmctr1));
    tx_cmd(0xB4);  tx_bytes(1, invctr, sizeof(invctr));
    tx_cmd(0xE0);  tx_bytes(1, gammaPos, sizeof(gammaPos));
    tx_cmd(0xE1);  tx_bytes(1, gammaNeg, sizeof(gammaNeg));
    tx_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));
    tx_cmd(0x29);

    gpio_set_level(LCD_BL_PIN, 0);
}

static void clear_screen(uint8_t color) {
    memset(g_pixels, color, SCREEN_W * SCREEN_H);
}

static void draw_char(int x, int y, char ch, uint8_t fg, uint8_t bg) {
    uint8_t idx = (uint8_t)ch;
    if (idx < 32 || idx > 126) return;
    const uint8_t *glyph = FONT[idx - 32];
    for (int col = 0; col < 5; ++col) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; ++row) {
            int px = x + col, py = y + row;
            if (px < 0 || px >= SCREEN_W || py < 0 || py >= SCREEN_H) continue;
            g_pixels[py * SCREEN_W + px] = ((bits >> row) & 1u) ? fg : bg;
        }
    }
}

static void draw_text(int x, int y, const char *text, uint8_t fg, uint8_t bg) {
    for (int i = 0; text[i] != '\0'; ++i) {
        draw_char(x + i * 6, y, text[i], fg, bg);
    }
}

static void fill_rect(int x, int y, int w, int h, uint8_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;
    for (int row = 0; row < h; ++row)
        memset(g_pixels + (y + row) * SCREEN_W + x, color, (size_t)w);
}

static void draw_char_2x(int x, int y, char ch, uint8_t fg, uint8_t bg) {
    uint8_t idx = (uint8_t)ch;
    if (idx < 32 || idx > 126) return;
    const uint8_t *glyph = FONT[idx - 32];
    for (int col = 0; col < 5; ++col) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; ++row) {
            uint8_t c = ((bits >> row) & 1u) ? fg : bg;
            fill_rect(x + col * 2, y + row * 2, 2, 2, c);
        }
    }
}

static void draw_text_big(int x, int y, const char *text, uint8_t fg, uint8_t bg) {
    for (int i = 0; text[i] != '\0'; ++i)
        draw_char_2x(x + i * 12, y, text[i], fg, bg);
}

static void present(void) {
    for (int y = 0; y < SCREEN_H; y += PRESENT_ROWS) {
        int rows = (SCREEN_H - y) < PRESENT_ROWS ? (SCREEN_H - y) : PRESENT_ROWS;
        for (int row = 0; row < rows; ++row) {
            for (int x = 0; x < SCREEN_W; ++x) {
                const uint8_t index = g_pixels[(y + row) * SCREEN_W + x];
                uint16_t color;
                if (index != TRANSPARENT_INDEX) {
                    color = to_rgb565(&g_palette[index]);
                } else if (!background_color_at(x, y + row, &color)) {
                    color = to_rgb565(&g_palette[0]);
                }
                g_chunk[row * SCREEN_W + x] = __builtin_bswap16(color);
            }
        }
        lcd_set_window(0, (uint16_t)y, (uint16_t)(SCREEN_W - 1), (uint16_t)(y + rows - 1));
        tx_bytes(1, g_chunk, (size_t)rows * SCREEN_W * 2);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static uint8_t scan_hc165_once(void) {
    gpio_set_level(HC165_PL_PIN, 0);
    esp_rom_delay_us(5);
    gpio_set_level(HC165_PL_PIN, 1);
    uint8_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val <<= 1;
        if (gpio_get_level(HC165_DAT_PIN) == 1) val |= 1;
        gpio_set_level(HC165_CLK_PIN, 1);
        esp_rom_delay_us(2);
        gpio_set_level(HC165_CLK_PIN, 0);
        esp_rom_delay_us(2);
    }
    return val;
}

static uint8_t scan_hc165(void) {
    uint8_t a = scan_hc165_once();
    uint8_t b = scan_hc165_once();
    uint8_t c = scan_hc165_once();
    return (a & b) | (a & c) | (b & c);
}

static void buttons_init(void) {
    gpio_reset_pin(HC165_PL_PIN);
    gpio_set_direction(HC165_PL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HC165_PL_PIN, 1);
    gpio_reset_pin(HC165_CLK_PIN);
    gpio_set_direction(HC165_CLK_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HC165_CLK_PIN, 0);
    gpio_reset_pin(HC165_DAT_PIN);
    gpio_set_direction(HC165_DAT_PIN, GPIO_MODE_INPUT);
    gpio_reset_pin(KEY_M_PIN);
    gpio_set_direction(KEY_M_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(KEY_M_PIN, GPIO_PULLUP_ONLY);
    gpio_reset_pin(KEY_SELECT_PIN);
    gpio_set_direction(KEY_SELECT_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(KEY_SELECT_PIN, GPIO_PULLUP_ONLY);
    gpio_reset_pin(KEY_START_PIN);
    gpio_set_direction(KEY_START_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(KEY_START_PIN, GPIO_PULLUP_ONLY);
}

static int button_m(void)      { return gpio_get_level(KEY_M_PIN) == 0; }
static int button_select(void) { return gpio_get_level(KEY_SELECT_PIN) == 0; }
static int button_start(void)  { return gpio_get_level(KEY_START_PIN) == 0; }
static int button_a(void) {
    return (scan_hc165() & (1u << HC165_A)) != 0;
}
static int button_b(void) {
    return (scan_hc165() & (1u << HC165_B)) != 0;
}
static int button_up(void) {
    return (scan_hc165() & (1u << HC165_UP)) != 0;
}
static int button_down(void) {
    return (scan_hc165() & (1u << HC165_DOWN)) != 0;
}

static void render_menu(uint8_t selected, const char *status) {
    const int panel_x = 8;
    const int panel_w = 124;
    clear_screen(TRANSPARENT_INDEX);

    draw_text_big(16, 14, "MiaOS", 5, 0);
    draw_text_big(16, 32, "Recovery", 1, 0);
    draw_text(16, 56, "SYSTEM TOOLS", 7, 0);
    fill_rect(panel_x, 70, panel_w, 1, 6);

    int is_mheld = status != NULL && strstr(status, "M held") != NULL;
    int is_help = status != NULL && strstr(status, "A: confirm") != NULL;

    if (is_mheld) {
        draw_text_big(16, 88, "Release M", 5, 0);
        draw_text_big(16, 108, "to start", 1, 0);
    } else {
        for (uint8_t i = 0; i < MENU_COUNT; ++i) {
            int y = 82 + i * 26;
            int active = (i == selected);
            if (active)
                fill_rect(panel_x, y, panel_w, 20, 2);
            draw_text(14, y + 7, active ? ">" : " ", 1, active ? 2 : 0);
            draw_text(26, y + 7, MENU_ITEMS[i], 1, active ? 2 : 0);
        }
    }

    fill_rect(panel_x, 200, panel_w, 1, 6);

    if (is_help) {
        draw_text(16, 210, "UP/DN  SELECT", 7, 0);
        draw_text(16, 224, "A OK   B BACK", 7, 0);
    } else if (is_mheld) {
        draw_text(16, 217, "RELEASE M KEY", 7, 0);
    } else {
        draw_text(16, 217, status != NULL ? status : "", 5, 0);
    }

    present();
}

static int clear_nvs(void) {
    esp_err_t err = nvs_flash_erase();
    if (err == ESP_OK) err = nvs_flash_init();
    return err == ESP_OK ? 0 : (int)err;
}

static int set_boot_slot(uint8_t slot) {
    if (slot > 1) return -1;
    const esp_partition_t *ota_data = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    const esp_partition_t *target = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP,
        (esp_partition_subtype_t)(ESP_PARTITION_SUBTYPE_APP_OTA_0 + slot), NULL);
    if (ota_data == NULL || target == NULL) return -1;

    typedef struct __attribute__((packed)) {
        uint32_t ota_seq;
        uint8_t seq_label[20];
        uint32_t ota_state;
        uint32_t crc;
    } ota_entry_t;
    ota_entry_t entries[2];
    memset(entries, 0, sizeof(entries));
    for (int i = 0; i < 2; ++i) {
        entries[i].ota_seq = (uint32_t)slot + 1u + (uint32_t)i * 2u;
        entries[i].ota_state = ESP_OTA_IMG_VALID;
        entries[i].crc = esp_rom_crc32_le(UINT32_MAX, (uint8_t *)&entries[i].ota_seq,
                                          sizeof(entries[i].ota_seq));
    }
    esp_err_t err = esp_partition_erase_range(ota_data, 0, ota_data->size);
    if (err == ESP_OK) err = esp_partition_write(ota_data, 0, &entries[0], sizeof(entries[0]));
    if (err == ESP_OK) err = esp_partition_write(ota_data, 4096, &entries[1], sizeof(entries[1]));
    return err == ESP_OK ? 0 : (int)err;
}

void app_main(void) {
    ESP_LOGI(TAG, "startup-menu app starting");
    lcd_init();
    background_init();
    buttons_init();

    render_menu(0, "M held: release to continue");
    while (button_m()) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    uint8_t selected = 0;
    render_menu(selected, "A: confirm  B: cancel");
    uint8_t prev_up = 0, prev_down = 0, prev_confirm = 0, prev_cancel = 0;

    for (;;) {
        uint8_t up = button_up();
        uint8_t down = button_down();
        uint8_t confirm = button_start() || button_a();
        uint8_t cancel = button_select() || button_b();

        if (up && !prev_up) {
            selected = (selected == 0) ? (MENU_COUNT - 1) : (selected - 1);
            render_menu(selected, "A: confirm  B: cancel");
        }
        if (down && !prev_down) {
            selected = (selected == MENU_COUNT - 1) ? 0 : (selected + 1);
            render_menu(selected, "A: confirm  B: cancel");
        }
        if (cancel && !prev_cancel) {
            render_menu(selected, "Canceling...");
            vTaskDelay(pdMS_TO_TICKS(300));
            esp_restart();
        }
        if (confirm && !prev_confirm) {
            int result = (selected == 0) ? clear_nvs() : set_boot_slot((uint8_t)(selected - 1));
            if (result == 0 && selected == 0) {
                result = set_boot_slot(0);
            }
            if (result != 0) {
                render_menu(selected, "Failed. B: back");
            } else {
                render_menu(selected, "Restarting...");
                vTaskDelay(pdMS_TO_TICKS(100));
                esp_restart();
            }
        }

        prev_up = up; prev_down = down; prev_confirm = confirm; prev_cancel = cancel;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
