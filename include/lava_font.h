#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed)) LavaFontGlyph {
  uint16_t code;
  uint8_t yOffset;
  uint8_t width;
  uint8_t height;
  uint8_t xOffset;
  uint8_t xDelta;
  uint8_t data[];
} LavaFontGlyph;

typedef struct LavaFont {
  char name[16];
  uint8_t type;
  uint8_t width;
  uint8_t height;
  size_t chars;
  uint8_t data[];
} LavaFont;

extern const LavaFont fontDroidGbk12;
extern const LavaFont fontBasic8x8;
extern const LavaFont fontDejaVu12;
extern const LavaFont fontDejaVu15;
extern const LavaFont fontVeraBold11;
extern const LavaFont fontVeraBold14;

#ifdef __cplusplus
}
#endif
