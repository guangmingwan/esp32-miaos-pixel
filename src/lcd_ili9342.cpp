#include "lcd_ili9342.h"

LcdIli9342 Lcd;

void LcdIli9342::begin(SPIClass &spi, int8_t cs, int8_t dc, int8_t rst, int8_t bl,
                       uint32_t spiHz) {
  spi_ = &spi;
  cs_ = cs;
  dc_ = dc;
  rst_ = rst;
  bl_ = bl;

  pinMode(cs_, OUTPUT);
  digitalWrite(cs_, HIGH);
  pinMode(dc_, OUTPUT);
  digitalWrite(dc_, HIGH);

  hardwareReset();

  // HD231005C10 backlight is active-LOW.
  if (bl_ >= 0) {
    pinMode(bl_, OUTPUT);
    digitalWrite(bl_, LOW);
  }

  spi_->beginTransaction(SPISettings(spiHz, MSBFIRST, SPI_MODE0));
  digitalWrite(cs_, LOW);

  static const uint8_t pwctrB[] = {0xFF, 0x93, 0x42};
  writeCommand(0xC8);
  writeData(pwctrB, 3);

  static const uint8_t madctl[] = {0xC8};
  writeCommand(0x36);
  writeData(madctl, 1);

  static const uint8_t pixfmt[] = {0x55};
  writeCommand(0x3A);
  writeData(pixfmt, 1);

  static const uint8_t pwctr1[] = {0x10, 0x10};
  writeCommand(0xC0);
  writeData(pwctr1, 2);

  static const uint8_t pwctr2[] = {0x01};
  writeCommand(0xC1);
  writeData(pwctr2, 1);

  static const uint8_t vmctr[] = {0xCD};
  writeCommand(0xC5);
  writeData(vmctr, 1);

  static const uint8_t frmctr1[] = {0x00, 0x1B};
  writeCommand(0xB1);
  writeData(frmctr1, 2);

  static const uint8_t invctr[] = {0x02};
  writeCommand(0xB4);
  writeData(invctr, 1);

  static const uint8_t gammaPos[] = {
      0x0F, 0x14, 0x17, 0x07, 0x16, 0x0A, 0x3F, 0x68,
      0x4C, 0x06, 0x0F, 0x0D, 0x18, 0x1A, 0x00};
  writeCommand(0xE0);
  writeData(gammaPos, sizeof(gammaPos));

  static const uint8_t gammaNeg[] = {
      0x00, 0x29, 0x29, 0x04, 0x0F, 0x04, 0x3C, 0x24,
      0x4B, 0x02, 0x0B, 0x09, 0x32, 0x37, 0x0F};
  writeCommand(0xE1);
  writeData(gammaNeg, sizeof(gammaNeg));

  writeCommand(0x11);
  delay(120);
  writeCommand(0x29);

  digitalWrite(cs_, HIGH);
  spi_->endTransaction();
}

void LcdIli9342::setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  spi_->beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
  digitalWrite(cs_, LOW);

  uint8_t buf[4];
  writeCommand(0x2A);
  buf[0] = x0 >> 8; buf[1] = x0;
  buf[2] = x1 >> 8; buf[3] = x1;
  writeData(buf, 4);

  writeCommand(0x2B);
  buf[0] = y0 >> 8; buf[1] = y0;
  buf[2] = y1 >> 8; buf[3] = y1;
  writeData(buf, 4);

  writeCommand(0x2C);

  digitalWrite(cs_, HIGH);
  spi_->endTransaction();
}

void LcdIli9342::pushColors(const uint16_t *colors, size_t count) {
  spi_->beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
  digitalWrite(cs_, LOW);
  digitalWrite(dc_, HIGH);
  spi_->transfer((uint8_t *)colors, count * 2);
  digitalWrite(cs_, HIGH);
  spi_->endTransaction();
}

void LcdIli9342::fillScreen(uint16_t color) {
  setWindow(0, 0, 319, 239);
  spi_->beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
  digitalWrite(cs_, LOW);
  digitalWrite(dc_, HIGH);
  for (uint32_t i = 0; i < 320UL * 240; ++i) {
    spi_->transfer16(color);
  }
  digitalWrite(cs_, HIGH);
  spi_->endTransaction();
}

void LcdIli9342::hardwareReset() {
  if (rst_ < 0) return;
  pinMode(rst_, OUTPUT);
  digitalWrite(rst_, HIGH);
  delay(1);
  digitalWrite(rst_, LOW);
  delay(10);
  digitalWrite(rst_, HIGH);
  delay(120);
}

void LcdIli9342::writeCommand(uint8_t cmd) {
  digitalWrite(dc_, LOW);
  spi_->transfer(cmd);
  digitalWrite(dc_, HIGH);
}

void LcdIli9342::writeData(uint8_t data) {
  spi_->transfer(data);
}

void LcdIli9342::writeData(const uint8_t *data, size_t len) {
  spi_->transfer((void *)data, len);
}
