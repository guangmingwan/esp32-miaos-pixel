#pragma once

#include <Arduino.h>

constexpr int LAVA_SCREEN_W = 320;
constexpr int LAVA_SCREEN_H = 240;

struct LavaColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct LavaSurface {
  int16_t w;
  int16_t h;
  int16_t pitch;
  uint8_t *pixels;
};

void lavaDisplayInit();
bool lavaDisplayReady();
void lavaSetPalette(uint8_t first, uint8_t count, const LavaColor *colors);
LavaSurface &lavaScreen();
void lavaClear(uint8_t color);
void lavaFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void lavaDrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void lavaDrawChar(int16_t x, int16_t y, char ch, uint8_t fg, uint8_t bg);
void lavaDrawText(int16_t x, int16_t y, const char *text, uint8_t fg, uint8_t bg);
int16_t lavaTextYCentered(int16_t areaY, int16_t areaHeight);
void lavaPresent();
