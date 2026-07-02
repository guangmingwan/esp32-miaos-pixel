#pragma once

#include <Arduino.h>

// Pin map: SCH_2.31寸彩屏掌机320_240_2026-07-02.pdf (authoritative).
// LCD pins verified working from datasheet.png. SD pins per user confirmation.

// --- LCD (HD231005C10 / ILI9342, verified working) ---
constexpr int TFT_SCK_PIN  = 12;
constexpr int TFT_MOSI_PIN = 11;
constexpr int TFT_MISO_PIN = -1;
constexpr int TFT_CS_PIN   = 10;
constexpr int TFT_DC_PIN   = 9;
constexpr int TFT_RST_PIN  = 3;
constexpr int TFT_BL_PIN   = 13;
constexpr uint32_t TFT_SPI_HZ = 40000000;

// --- SD card (independent SPI bus) ---
constexpr int SD_SCK_PIN  = 7;
constexpr int SD_MOSI_PIN = 6;
constexpr int SD_MISO_PIN = 15;
constexpr int SD_CS_PIN   = 5;
constexpr uint32_t SD_SPI_HZ = 20000000;

// --- Audio (NS4168 I2S amplifier) ---
constexpr int I2S_WS_PIN   = 42;
constexpr int I2S_BCK_PIN  = 41;
constexpr int I2S_DATA_PIN = 40;
constexpr int AMP_CTRL_PIN = 46;  // NS4168 EN, active-HIGH (R15 10k pulldown)

// --- Battery voltage (200k/200k divider, ratio x2) ---
constexpr int VBAT_ADC_PIN   = 1;
constexpr float VBAT_DIVIDER = 2.0f;

// --- I2C (PCF8563 RTC + expansion) ---
constexpr int I2C_SCL_PIN = 4;
constexpr int I2C_SDA_PIN = 16;

// --- Buzzer ---
constexpr int BEEP_PIN = 14;

// --- Direct GPIO keys ---
constexpr int KEY_BOOT_PIN   = 0;
constexpr int KEY_M_PIN      = 8;   // Modifier
constexpr int KEY_L_PIN      = 17;  // Left shoulder
constexpr int KEY_R_PIN      = 18;  // Right shoulder
constexpr int KEY_SELECT_PIN = 21;
constexpr int KEY_START_PIN  = KEY_BOOT_PIN;

// --- 74HC165 shift register (D-pad + ABXY) ---
constexpr int HC165_PL_PIN  = 2;   // K_PL (parallel load, active-LOW)
constexpr int HC165_CLK_PIN = 39;  // K_CLK
constexpr int HC165_DAT_PIN = 38;  // K_DAT (Q7 serial out)

// 74HC165 parallel inputs (corrected from on-device button verification):
// D0=LEFT D1=DOWN D2=UP D3=RIGHT D4=Y D5=X D6=A D7=B
// D7 shifts out first, so Q7 stream order is: B,A,X,Y,RIGHT,UP,DOWN,LEFT
enum Hc165Bit : uint8_t {
  HC165_LEFT  = 0,  // D0
  HC165_DOWN  = 1,  // D1
  HC165_UP    = 2,  // D2
  HC165_RIGHT = 3,  // D3
  HC165_Y     = 4,  // D4
  HC165_X     = 5,  // D5
  HC165_A     = 6,  // D6
  HC165_B     = 7,  // D7
};

// --- Button abstraction for apps (legacy 6-index: A/B/UP/DN/LT/RT) ---
struct AppButton {
  const char *label;
};

constexpr AppButton APP_BUTTONS[] = {
    {"A"}, {"B"}, {"UP"}, {"DN"}, {"LT"}, {"RT"},
};
constexpr size_t APP_BUTTON_COUNT = sizeof(APP_BUTTONS) / sizeof(APP_BUTTONS[0]);

// --- Full physical button table (14 keys: 6 direct + 8 scanned) ---
enum ButtonSource : uint8_t { SRC_GPIO, SRC_HC165 };

struct ButtonProbe {
  const char *label;
  ButtonSource source;
  int8_t gpio;
  uint8_t shiftBit;
};

constexpr ButtonProbe ALL_BUTTONS[] = {
    {"BOOT", SRC_GPIO,  KEY_BOOT_PIN,   0},
    {"ST",   SRC_GPIO,  KEY_START_PIN,  0},
    {"M",    SRC_GPIO,  KEY_M_PIN,      0},
    {"L",    SRC_GPIO,  KEY_L_PIN,      0},
    {"R",    SRC_GPIO,  KEY_R_PIN,      0},
    {"SEL",  SRC_GPIO,  KEY_SELECT_PIN, 0},
    {"A",    SRC_HC165, 0, HC165_A},
    {"B",    SRC_HC165, 0, HC165_B},
    {"X",    SRC_HC165, 0, HC165_X},
    {"Y",    SRC_HC165, 0, HC165_Y},
    {"UP",   SRC_HC165, 0, HC165_UP},
    {"DN",   SRC_HC165, 0, HC165_DOWN},
    {"LF",   SRC_HC165, 0, HC165_LEFT},
    {"RT",   SRC_HC165, 0, HC165_RIGHT},
};
constexpr size_t ALL_BUTTON_COUNT = sizeof(ALL_BUTTONS) / sizeof(ALL_BUTTONS[0]);
