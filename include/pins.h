#pragma once

#include <Arduino.h>

// IO mapping from datasheet.png (ESP32-S3-WROOM-1-N16R8, HD231005C10 / ILI9342).
// Full map: retro-pixel-datasheet.md.

constexpr int TFT_SCK_PIN  = 12;   // LCD_CLK
constexpr int TFT_MOSI_PIN = 11;   // LCD_MOSI
constexpr int TFT_MISO_PIN = -1;   // NC (LCD is write-only)
constexpr int TFT_CS_PIN   = 10;   // LCD_CS
constexpr int TFT_DC_PIN   = 9;    // LCD_DC
constexpr int TFT_RST_PIN  = 3;    // LCD_RST
constexpr int TFT_BL_PIN   = 13;   // LCD_BCKL
constexpr uint32_t TFT_SPI_HZ = 40000000;
constexpr uint8_t TFT_ROTATION = 1;

constexpr int SD_SCK_PIN  = 7;     // SDSPI_CLK
constexpr int SD_MOSI_PIN = 6;     // SDSPI_MOSI
constexpr int SD_MISO_PIN = 15;    // SDSPI_MISO
constexpr int SD_CS_PIN   = 5;     // SDSPI_CS
constexpr uint32_t SD_SPI_HZ = 20000000;

constexpr int BEEP_PIN = 14;       // BEEP

constexpr int VBAT_ADC_PIN = 1;    // VBAT_VOLTAGE

constexpr int KEY_BOOT_PIN   = 0;  // KEY_BOOT / KEY_START
constexpr int KEY_L_PIN      = 17; // KEY_L
constexpr int KEY_R_PIN      = 18; // KEY_R
constexpr int KEY_M_PIN      = 8;  // KEY_M
constexpr int KEY_SELECT_PIN = 21; // KEY_SELECT

constexpr int I2C_SCL_PIN = 4;     // XSCL
constexpr int I2C_SDA_PIN = 16;    // XSDA

constexpr int UART0_TX_PIN = 43;   // TXD0
constexpr int UART0_RX_PIN = 44;   // RXD0

struct ButtonProbe {
  const char *label;
  uint8_t pin;
  bool externalPullup;
};

// Apps index buttons as: [0]=A(confirm) [1]=B(back) [2]=UP [3]=DN [4]=LT [5]=RT.
// Retro-Pixel has 5 keys; LT/RT reuse BOOT and SELECT as auxiliaries.
constexpr ButtonProbe BUTTONS[] = {
    {"A",  KEY_M_PIN,      false},
    {"B",  KEY_SELECT_PIN, false},
    {"UP", KEY_L_PIN,      false},
    {"DN", KEY_R_PIN,      false},
    {"LT", KEY_BOOT_PIN,   true},
    {"RT", KEY_BOOT_PIN,   true},
};

constexpr size_t BUTTON_COUNT = sizeof(BUTTONS) / sizeof(BUTTONS[0]);
