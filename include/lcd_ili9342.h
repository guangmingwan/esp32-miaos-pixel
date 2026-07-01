#pragma once

#include <Arduino.h>
#include <SPI.h>

// Standalone ILI9342 driver for HD231005C10 (2.31" 320x240 IPS).
// Vendor init sequence mirrored from lib/TFT320240_ILI9342_test.ino.
// No Adafruit dependency; uses ESP32-S3 hardware SPI.

class LcdIli9342 {
 public:
  void begin(SPIClass &spi, int8_t cs, int8_t dc, int8_t rst, int8_t bl,
             uint32_t spiHz);
  void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
  void pushColors(const uint16_t *colors, size_t count);
  void fillScreen(uint16_t color);

 private:
  void writeCommand(uint8_t cmd);
  void writeData(uint8_t data);
  void writeData(const uint8_t *data, size_t len);
  void hardwareReset();

  SPIClass *spi_ = nullptr;
  int8_t cs_ = -1;
  int8_t dc_ = -1;
  int8_t rst_ = -1;
  int8_t bl_ = -1;
};

extern LcdIli9342 Lcd;
