#pragma once

#include <Arduino.h>

#include "lava_native_display.h"

enum class LavaFontFace : uint8_t {
  Small5x7 = 0,
  Basic8 = 1,
  Basic12 = 2,
  Basic16 = 3,
  DejaVu12 = 4,
  DejaVu15 = 5,
  VeraBold11 = 6,
  VeraBold14 = 7,
  DroidGbk12 = 8,
};

LavaFontFace lavaFontFace();
void lavaSetFontFace(LavaFontFace face);
void lavaUseFontFaceForSession(LavaFontFace face);
void lavaCycleFontFace();
void lavaFontSkipPersisted(void);
const char *lavaFontName(LavaFontFace face);
int16_t lavaFontHeight();
int16_t lavaTextWidth(const char *text);
int16_t lavaDrawTextUtf8(LavaSurface &surface, int16_t x, int16_t y,
                         const char *text, uint8_t fg, uint8_t bg);
