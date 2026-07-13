#include "droid_gbk_renderer.h"

#include <stddef.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "droid_gbk_index.h"
#include "gbk_unicode_map.h"
#include "lava_font.h"

namespace {

constexpr size_t DROID_DATA_BYTES = 456429;
constexpr uint8_t FONT_HEIGHT = 16;
constexpr uint8_t FALLBACK_WIDTH = 8;

constexpr uint32_t ASCII_OFFSETS[] = {
    0x00000000u, 0x00000007u, 0x00000010u, 0x00000019u, 0x00000028u, 0x00000036u, 0x00000046u, 0x00000056u,
    0x0000005Eu, 0x0000006Au, 0x00000076u, 0x00000082u, 0x0000008Du, 0x00000095u, 0x0000009Du, 0x000000A5u,
    0x000000B1u, 0x000000BEu, 0x000000C9u, 0x000000D6u, 0x000000E3u, 0x000000F2u, 0x000000FFu, 0x0000010Cu,
    0x00000119u, 0x00000126u, 0x00000133u, 0x0000013Bu, 0x00000145u, 0x00000150u, 0x0000015Au, 0x00000165u,
    0x00000172u, 0x00000185u, 0x00000195u, 0x000001A3u, 0x000001B1u, 0x000001C0u, 0x000001CDu, 0x000001DAu,
    0x000001E9u, 0x000001F8u, 0x00000204u, 0x00000210u, 0x0000021Eu, 0x0000022Bu, 0x0000023Du, 0x0000024Cu,
    0x0000025Cu, 0x0000026Au, 0x0000027Cu, 0x0000028Au, 0x00000297u, 0x000002A6u, 0x000002B5u, 0x000002C4u,
    0x000002D8u, 0x000002E7u, 0x000002F6u, 0x00000304u, 0x00000310u, 0x0000031Cu, 0x00000328u, 0x00000334u,
    0x0000033Cu, 0x00000344u, 0x00000350u, 0x0000035Fu, 0x0000036Au, 0x00000378u, 0x00000384u, 0x00000392u,
    0x000003A0u, 0x000003AEu, 0x000003B7u, 0x000003C3u, 0x000003D1u, 0x000003DAu, 0x000003E9u, 0x000003F5u,
    0x00000402u, 0x00000411u, 0x0000041Fu, 0x0000042Au, 0x00000435u, 0x00000441u, 0x0000044Du, 0x0000045Au,
    0x00000469u, 0x00000476u, 0x00000485u, 0x00000491u, 0x0000049Eu, 0x000004A7u, 0x000004B4u,
};

int decode_utf8(const char **cursor) {
  const uint8_t first = static_cast<uint8_t>(**cursor);
  if (first < 0x80) {
    ++*cursor;
    return first;
  }

  uint8_t count = 0;
  int codepoint = 0;
  if ((first & 0xE0) == 0xC0) {
    count = 1;
    codepoint = first & 0x1F;
  } else if ((first & 0xF0) == 0xE0) {
    count = 2;
    codepoint = first & 0x0F;
  } else if ((first & 0xF8) == 0xF0) {
    count = 3;
    codepoint = first & 0x07;
  } else {
    ++*cursor;
    return -1;
  }

  const char *next = *cursor + 1;
  for (uint8_t i = 0; i < count; ++i) {
    const uint8_t byte = static_cast<uint8_t>(next[i]);
    if (byte == 0 || (byte & 0xC0) != 0x80) {
      ++*cursor;
      return -1;
    }
    codepoint = (codepoint << 6) | (byte & 0x3F);
  }
  *cursor = next + count;
  return codepoint;
}

const uint8_t *find_glyph(int codepoint) {
  if (codepoint >= 0x20 && codepoint <= 0x7E) {
    return fontDroidGbk12.data + ASCII_OFFSETS[codepoint - 0x20];
  }
  if (codepoint < 0x80 || codepoint > 0xFFFF) {
    return nullptr;
  }
  uint16_t gbk = 0;
  if (!gbkUnicodeToCode(static_cast<uint16_t>(codepoint), &gbk)) {
    return nullptr;
  }
  const uint32_t offset = DROID_GBK12_GLYPH_OFFSETS[gbk];
  if (offset == DROID_GBK12_NO_GLYPH || offset > DROID_DATA_BYTES - 7) {
    return nullptr;
  }
  return fontDroidGbk12.data + offset;
}

void set_pixel(uint8_t *pixels, int32_t width, int32_t height, int32_t x,
               int32_t y, uint8_t color) {
  if (x >= 0 && x < width && y >= 0 && y < height) {
    pixels[y * width + x] = color;
  }
}

void fill_background(uint8_t *pixels, int32_t width, int32_t height, int32_t x,
                     int32_t y, uint8_t advance, uint8_t bg) {
  for (uint8_t row = 0; row < FONT_HEIGHT; ++row) {
    for (uint8_t col = 0; col < advance; ++col) {
      set_pixel(pixels, width, height, x + col, y + row, bg);
    }
  }
}

uint8_t draw_missing(uint8_t *pixels, int32_t width, int32_t height, int32_t x,
                     int32_t y, uint8_t fg, uint8_t bg) {
  fill_background(pixels, width, height, x, y, FALLBACK_WIDTH, bg);
  for (uint8_t row = 0; row < FONT_HEIGHT; ++row) {
    for (uint8_t col = 0; col < FALLBACK_WIDTH; ++col) {
      if (row == 0 || row == FONT_HEIGHT - 1 || col == 0 || col == FALLBACK_WIDTH - 1) {
        set_pixel(pixels, width, height, x + col, y + row, fg);
      }
    }
  }
  return FALLBACK_WIDTH;
}

uint8_t draw_glyph(uint8_t *pixels, int32_t width, int32_t height, int32_t x,
                   int32_t y, const uint8_t *glyph, uint8_t fg, uint8_t bg) {
  if (glyph == nullptr) {
    return draw_missing(pixels, width, height, x, y, fg, bg);
  }
  const uint8_t y_offset = glyph[2];
  const uint8_t glyph_width = glyph[3];
  const uint8_t glyph_height = glyph[4];
  const int8_t x_offset = static_cast<int8_t>(glyph[5]);
  const uint8_t x_delta = glyph[6];
  if (glyph_width == 0 && glyph_height == 0 && x_delta > 0) {
    fill_background(pixels, width, height, x, y, x_delta, bg);
    return x_delta;
  }
  if (glyph_width == 0 || glyph_width > 31 || glyph_height == 0 ||
      glyph_height > FONT_HEIGHT || y_offset + glyph_height > FONT_HEIGHT) {
    return draw_missing(pixels, width, height, x, y, fg, bg);
  }
  const size_t bitmap_bytes = ((static_cast<size_t>(glyph_width) * glyph_height) + 7) / 8;
  const uint8_t *data = glyph + 7;
  const uint8_t *font_end = fontDroidGbk12.data + DROID_DATA_BYTES;
  if (data < fontDroidGbk12.data || data > font_end ||
      static_cast<size_t>(font_end - data) < bitmap_bytes) {
    return draw_missing(pixels, width, height, x, y, fg, bg);
  }

  const uint8_t advance = glyph_width > x_delta ? glyph_width : x_delta;
  fill_background(pixels, width, height, x, y, advance, bg);
  size_t bit_index = 0;
  for (uint8_t row = 0; row < glyph_height; ++row) {
    for (uint8_t col = 0; col < glyph_width; ++col, ++bit_index) {
      if ((data[bit_index / 8] & (0x80u >> (bit_index % 8))) != 0) {
        set_pixel(pixels, width, height, x + x_offset + col, y + y_offset + row, fg);
      }
    }
  }
  return advance;
}

}

int32_t droid_gbk_draw_text(uint8_t *pixels, int32_t surface_width,
                            int32_t surface_height, int32_t x, int32_t y,
                            const char *text, uint8_t fg, uint8_t bg) {
  if (pixels == nullptr || text == nullptr) {
    return 0;
  }
  int32_t cursor_x = x;
  uint32_t rendered = 0;
  for (const char *cursor = text; *cursor != '\0';) {
    const int codepoint = decode_utf8(&cursor);
    cursor_x += draw_glyph(pixels, surface_width, surface_height, cursor_x, y,
                           find_glyph(codepoint), fg, bg);
    if ((++rendered % 8) == 0) {
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }
  return cursor_x - x;
}
