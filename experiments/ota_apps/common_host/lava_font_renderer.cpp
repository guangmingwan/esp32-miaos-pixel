#include "lava_font_renderer.h"

#include <stddef.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "lava_font.h"

namespace {

constexpr uint8_t MAX_FONT_HEIGHT = 16;
constexpr uint8_t FALLBACK_WIDTH = 8;

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

const LavaFont *font_for_face(uint8_t face) {
  switch (face) {
    case 1:
    case 2:
    case 3:
      return &fontBasic8x8;
    case 4:
      return &fontDejaVu12;
    case 5:
      return &fontDejaVu15;
    case 6:
      return &fontVeraBold11;
    case 7:
      return &fontVeraBold14;
    default:
      return nullptr;
  }
}

uint8_t points_for_face(uint8_t face) {
  if (face == 2) return 12;
  if (face == 3) return 16;
  const LavaFont *font = font_for_face(face);
  return font == nullptr ? 7 : font->height;
}

size_t glyph_size(const uint8_t *glyph) {
  const uint8_t width = glyph[3];
  const uint8_t height = glyph[4];
  return 7 + (static_cast<size_t>(width) * height + 7) / 8;
}

const uint8_t *find_glyph(const LavaFont *font, int codepoint) {
  if (font == nullptr || codepoint < 0 || codepoint > 0xFFFF) return nullptr;
  const uint8_t *glyph = font->data;
  for (size_t i = 0; i < font->chars; ++i) {
    const uint16_t code = static_cast<uint16_t>(glyph[0]) |
                          (static_cast<uint16_t>(glyph[1]) << 8);
    if (code == codepoint) return glyph;
    if (code == 0) break;
    glyph += glyph_size(glyph);
  }
  return nullptr;
}

void set_pixel(uint8_t *pixels, int32_t width, int32_t height, int32_t x,
               int32_t y, uint8_t color) {
  if (x >= 0 && x < width && y >= 0 && y < height) {
    pixels[y * width + x] = color;
  }
}

void fill_background(uint8_t *pixels, int32_t width, int32_t height, int32_t x,
                     int32_t y, uint8_t advance, uint8_t points, uint8_t bg) {
  for (uint8_t row = 0; row < points; ++row) {
    for (uint8_t col = 0; col < advance; ++col) {
      set_pixel(pixels, width, height, x + col, y + row, bg);
    }
  }
}

uint8_t draw_missing(uint8_t *pixels, int32_t width, int32_t height, int32_t x,
                     int32_t y, uint8_t advance, uint8_t points, uint8_t fg,
                     uint8_t bg) {
  fill_background(pixels, width, height, x, y, advance, points, bg);
  for (uint8_t row = 0; row < points; ++row) {
    for (uint8_t col = 0; col < advance; ++col) {
      if (row == 0 || row + 1 == points || col == 0 || col + 1 == advance) {
        set_pixel(pixels, width, height, x + col, y + row, fg);
      }
    }
  }
  return advance;
}

uint8_t draw_glyph(uint8_t *pixels, int32_t width, int32_t height, int32_t x,
                   int32_t y, const LavaFont *font, uint8_t points,
                   const uint8_t *glyph, uint8_t fg, uint8_t bg) {
  const uint8_t fallback = font->width == 0 ? FALLBACK_WIDTH : font->width;
  if (glyph == nullptr) {
    return draw_missing(pixels, width, height, x, y, fallback, points, fg, bg);
  }

  const uint8_t y_offset = glyph[2];
  const uint8_t glyph_width = glyph[3];
  const uint8_t glyph_height = glyph[4];
  const int8_t x_offset = static_cast<int8_t>(glyph[5]);
  const uint8_t x_delta = glyph[6];
  if (glyph_width == 0 && glyph_height == 0 && x_delta > 0) {
    const uint8_t advance = points == font->height
                                ? x_delta
                                : static_cast<uint8_t>((x_delta * points) / font->height);
    fill_background(pixels, width, height, x, y, advance, points, bg);
    return advance;
  }
  if (glyph_width == 0 || glyph_width > 31 || glyph_height == 0 ||
      glyph_height > MAX_FONT_HEIGHT || y_offset + glyph_height > MAX_FONT_HEIGHT) {
    return draw_missing(pixels, width, height, x, y, fallback, points, fg, bg);
  }

  const uint8_t raw_advance = glyph_width > x_delta ? glyph_width : x_delta;
  uint8_t advance = points == font->height
                        ? raw_advance
                        : static_cast<uint8_t>((raw_advance * points) / font->height);
  if (advance == 0) advance = 1;
  fill_background(pixels, width, height, x, y, advance, points, bg);

  uint32_t native_rows[MAX_FONT_HEIGHT] = {};
  const uint8_t *data = glyph + 7;
  size_t bit = 0;
  for (uint8_t row = 0; row < glyph_height; ++row) {
    uint32_t row_bits = 0;
    for (uint8_t col = 0; col < glyph_width; ++col, ++bit) {
      const int draw_x = x_offset + col;
      if ((data[bit / 8] & (0x80u >> (bit % 8))) != 0 &&
          draw_x >= 0 && draw_x < 32) {
        row_bits |= 1u << draw_x;
      }
    }
    native_rows[y_offset + row] = row_bits;
  }

  for (uint8_t row = 0; row < points; ++row) {
    const uint32_t row_bits = native_rows[(row * font->height) / points];
    for (uint8_t col = 0; col < 32; ++col) {
      if ((row_bits & (1u << col)) != 0) {
        set_pixel(pixels, width, height, x + col, y + row, fg);
      }
    }
  }
  return advance;
}

}  // namespace

int32_t lava_font_renderer_draw_text(
    uint8_t *pixels, int32_t surface_width, int32_t surface_height, int32_t x,
    int32_t y, const char *text, uint8_t fg, uint8_t bg, uint8_t font_face) {
  const LavaFont *font = font_for_face(font_face);
  if (pixels == nullptr || text == nullptr || font == nullptr) return -1;
  const uint8_t points = points_for_face(font_face);
  int32_t cursor_x = x;
  uint32_t rendered = 0;
  for (const char *cursor = text; *cursor != '\0';) {
    const int codepoint = decode_utf8(&cursor);
    cursor_x += draw_glyph(pixels, surface_width, surface_height, cursor_x, y,
                           font, points, find_glyph(font, codepoint), fg, bg);
    if ((++rendered % 8) == 0) vTaskDelay(pdMS_TO_TICKS(1));
  }
  return cursor_x - x;
}

int32_t lava_font_renderer_text_height(uint8_t font_face) {
  return font_for_face(font_face) == nullptr ? -1 : points_for_face(font_face);
}
