/*
 * USB Disk - Standalone USB MSC firmware for ESP32-S3
 *
 * Exposes the SD card as a USB Mass Storage device using the same ESP-IDF
 * SD mount + ILI9342 init sequence as the launcher for reliable operation.
 *
 * Category: System, Name: usb disk (OTA app manifest appended post-build).
 * Exits back to launcher (ota_0) via SELECT + START.
 */

#include <Arduino.h>
#include <SPI.h>
#include <esp_ota_ops.h>
#include <esp_rom_crc.h>
#include <esp_vfs_fat.h>
#include <driver/sdspi_host.h>
#include <sdmmc_cmd.h>
#include <ff.h>
#include <diskio.h>
#include "USB.h"
#include "USBMSC.h"

/* ------------------------------------------------------------------ */
/* Pin constants (from include/pins.h)                                */
/* ------------------------------------------------------------------ */
static constexpr int TFT_SCK_PIN  = 12;
static constexpr int TFT_MOSI_PIN = 11;
static constexpr int TFT_MISO_PIN = 48;
static constexpr int TFT_CS_PIN   = 10;
static constexpr int TFT_DC_PIN   = 9;
static constexpr int TFT_RST_PIN  = 3;
static constexpr int TFT_BL_PIN   = 13;
static constexpr uint32_t TFT_SPI_HZ = 40000000;

static constexpr int SD_SCK_PIN   = 7;
static constexpr int SD_MOSI_PIN  = 6;
static constexpr int SD_MISO_PIN  = 15;
static constexpr int SD_CS_PIN    = 5;

static constexpr int KEY_SELECT_PIN = 21;
static constexpr int KEY_BOOT_PIN   = 0;

/* ------------------------------------------------------------------ */
/* ILI9342 driver (same init sequence as launcher's lcd_ili9342.cpp)  */
/* ------------------------------------------------------------------ */
static SPIClass tftSpi(FSPI);

static void lcdCsLow()  { digitalWrite(TFT_CS_PIN, LOW); }
static void lcdCsHigh() { digitalWrite(TFT_CS_PIN, HIGH); }
static void lcdDcCmd()  { digitalWrite(TFT_DC_PIN, LOW); }
static void lcdDcDat()  { digitalWrite(TFT_DC_PIN, HIGH); }

static void lcdWriteCmd(uint8_t cmd) {
  lcdCsLow();
  lcdDcCmd();
  tftSpi.transfer(cmd);
  lcdDcDat();
  lcdCsHigh();
}

static void lcdWriteData(uint8_t data) {
  lcdCsLow();
  tftSpi.transfer(data);
  lcdCsHigh();
}

static void lcdWriteBuf(const uint8_t *data, size_t len) {
  lcdCsLow();
  tftSpi.transfer((void *)data, len);
  lcdCsHigh();
}

static void lcdReset() {
  pinMode(TFT_RST_PIN, OUTPUT);
  digitalWrite(TFT_RST_PIN, HIGH);
  delay(1);
  digitalWrite(TFT_RST_PIN, LOW);
  delay(10);
  digitalWrite(TFT_RST_PIN, HIGH);
  delay(120);
}

static void lcdInit() {
  pinMode(TFT_CS_PIN, OUTPUT);
  pinMode(TFT_DC_PIN, OUTPUT);
  pinMode(TFT_BL_PIN, OUTPUT);
  lcdCsHigh();
  digitalWrite(TFT_BL_PIN, LOW);

  tftSpi.begin(TFT_SCK_PIN, TFT_MISO_PIN, TFT_MOSI_PIN, TFT_CS_PIN);
  tftSpi.beginTransaction(SPISettings(TFT_SPI_HZ, MSBFIRST, SPI_MODE0));

  lcdReset();

  static const uint8_t pwctrB[] = {0xFF, 0x93, 0x42};
  lcdWriteCmd(0xC8);
  lcdWriteBuf(pwctrB, 3);

  lcdWriteCmd(0x36);
  lcdWriteData(0xC8);

  lcdWriteCmd(0x3A);
  lcdWriteData(0x55);

  static const uint8_t pwctr1[] = {0x10, 0x10};
  lcdWriteCmd(0xC0);
  lcdWriteBuf(pwctr1, 2);

  lcdWriteCmd(0xC1);
  lcdWriteData(0x01);

  lcdWriteCmd(0xC5);
  lcdWriteData(0xCD);

  static const uint8_t frmctr1[] = {0x00, 0x1B};
  lcdWriteCmd(0xB1);
  lcdWriteBuf(frmctr1, 2);

  lcdWriteCmd(0xB4);
  lcdWriteData(0x02);

  static const uint8_t gammaPos[] = {
      0x0F, 0x14, 0x17, 0x07, 0x16, 0x0A, 0x3F, 0x68,
      0x4C, 0x06, 0x0F, 0x0D, 0x18, 0x1A, 0x00};
  lcdWriteCmd(0xE0);
  lcdWriteBuf(gammaPos, sizeof(gammaPos));

  static const uint8_t gammaNeg[] = {
      0x00, 0x29, 0x29, 0x04, 0x0F, 0x04, 0x3C, 0x24,
      0x4B, 0x02, 0x0B, 0x09, 0x32, 0x37, 0x0F};
  lcdWriteCmd(0xE1);
  lcdWriteBuf(gammaNeg, sizeof(gammaNeg));

  lcdWriteCmd(0x11);
  delay(120);

  lcdWriteCmd(0x29);

  tftSpi.endTransaction();
}

static void lcdSetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  tftSpi.beginTransaction(SPISettings(TFT_SPI_HZ, MSBFIRST, SPI_MODE0));
  uint8_t buf[4];
  lcdWriteCmd(0x2A);
  buf[0] = x0 >> 8; buf[1] = x0; buf[2] = x1 >> 8; buf[3] = x1;
  lcdWriteBuf(buf, 4);
  lcdWriteCmd(0x2B);
  buf[0] = y0 >> 8; buf[1] = y0; buf[2] = y1 >> 8; buf[3] = y1;
  lcdWriteBuf(buf, 4);
  lcdWriteCmd(0x2C);
  lcdCsHigh();
  tftSpi.endTransaction();
}

static void lcdFill(uint16_t color) {
  lcdSetWindow(0, 0, 319, 239);
  tftSpi.beginTransaction(SPISettings(TFT_SPI_HZ, MSBFIRST, SPI_MODE0));
  lcdCsLow();
  lcdDcDat();
  for (uint32_t i = 0; i < 320UL * 240; ++i) {
    tftSpi.transfer16(color);
  }
  lcdCsHigh();
  tftSpi.endTransaction();
}

/* 8x8 bitmap font (ASCII 0x20-0x7E) */
static const uint8_t font8x8[95][8] = {
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
  {0x00,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x6C,0xFE,0x6C,0x6C,0xFE,0x6C,0x00},
  {0x18,0x7E,0xC0,0x7C,0x06,0xC6,0x7C,0x18},
  {0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00},
  {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00},
  {0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x0C,0x18,0x30,0x30,0x18,0x0C,0x00},
  {0x00,0x30,0x18,0x0C,0x0C,0x18,0x30,0x00},
  {0x00,0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00},
  {0x00,0x00,0x18,0x18,0x7E,0x18,0x18,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},
  {0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
  {0x00,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00},
  {0x00,0x7C,0xC6,0xCE,0xD6,0xE6,0x7C,0x00},
  {0x00,0x18,0x38,0x18,0x18,0x18,0x7E,0x00},
  {0x00,0x7C,0xC6,0x06,0x1C,0x70,0xFE,0x00},
  {0x00,0x7E,0x0C,0x18,0x3C,0x06,0xFC,0x00},
  {0x00,0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x00},
  {0x00,0xFC,0xC0,0xFC,0x06,0xC6,0x7C,0x00},
  {0x00,0x7C,0xC0,0xFC,0xC6,0xC6,0x7C,0x00},
  {0x00,0xFE,0x06,0x0C,0x18,0x30,0x30,0x00},
  {0x00,0x7C,0xC6,0x7C,0xC6,0xC6,0x7C,0x00},
  {0x00,0x7C,0xC6,0xC6,0x7E,0x06,0x7C,0x00},
  {0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x00},
  {0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x30},
  {0x00,0x06,0x1C,0x70,0x1C,0x06,0x00,0x00},
  {0x00,0x00,0x00,0xFE,0x00,0xFE,0x00,0x00},
  {0x00,0x60,0x38,0x0E,0x38,0x60,0x00,0x00},
  {0x00,0x7C,0xC6,0x0C,0x18,0x00,0x18,0x00},
  {0x7C,0xC6,0xDE,0xF6,0xF6,0xDE,0xC0,0x7C},
  {0x00,0x7C,0xC6,0xC6,0xFE,0xC6,0xC6,0x00},
  {0x00,0xFC,0xC6,0xFC,0xC6,0xC6,0xFC,0x00},
  {0x00,0x7C,0xC6,0xC0,0xC0,0xC6,0x7C,0x00},
  {0x00,0xFC,0xC6,0xC6,0xC6,0xC6,0xFC,0x00},
  {0x00,0xFE,0xC0,0xFC,0xC0,0xC0,0xFE,0x00},
  {0x00,0xFE,0xC0,0xFC,0xC0,0xC0,0xC0,0x00},
  {0x00,0x7C,0xC6,0xC0,0xCE,0xC6,0x7E,0x00},
  {0x00,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},
  {0x00,0x7E,0x18,0x18,0x18,0x18,0x7E,0x00},
  {0x00,0x06,0x06,0x06,0x06,0xC6,0x7C,0x00},
  {0x00,0xC6,0xCC,0xF8,0xCC,0xC6,0xC6,0x00},
  {0x00,0xC0,0xC0,0xC0,0xC0,0xC0,0xFE,0x00},
  {0x00,0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0x00},
  {0x00,0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0x00},
  {0x00,0x7C,0xC6,0xC6,0xC6,0xC6,0x7C,0x00},
  {0x00,0xFC,0xC6,0xC6,0xFC,0xC0,0xC0,0x00},
  {0x00,0x7C,0xC6,0xC6,0xD6,0xCE,0x7C,0x06},
  {0x00,0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0x00},
  {0x00,0x7C,0xC6,0x70,0x1C,0xC6,0x7C,0x00},
  {0x00,0xFF,0x18,0x18,0x18,0x18,0x18,0x00},
  {0x00,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00},
  {0x00,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x00},
  {0x00,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00},
  {0x00,0xC6,0x6C,0x38,0x38,0x6C,0xC6,0x00},
  {0x00,0xC6,0xC6,0x6C,0x38,0x38,0x38,0x00},
  {0x00,0xFE,0x0C,0x18,0x30,0x60,0xFE,0x00},
  {0x00,0x3C,0x30,0x30,0x30,0x30,0x3C,0x00},
  {0x00,0xC0,0x60,0x30,0x18,0x0C,0x06,0x00},
  {0x00,0x78,0x18,0x18,0x18,0x18,0x78,0x00},
  {0x00,0x10,0x38,0x6C,0xC6,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
  {0x00,0x18,0x18,0x0C,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x7C,0x06,0x7E,0xCE,0x00},
  {0x00,0xC0,0xC0,0xFC,0xC6,0xC6,0xFC,0x00},
  {0x00,0x00,0x00,0x7C,0xC0,0xC0,0x7C,0x00},
  {0x00,0x06,0x06,0x7E,0xC6,0xC6,0x7E,0x00},
  {0x00,0x00,0x00,0x7C,0xFE,0xC0,0x7C,0x00},
  {0x00,0x1C,0x36,0x30,0xFC,0x30,0x30,0x00},
  {0x00,0x00,0x00,0x7E,0xC6,0x7E,0x06,0x7C},
  {0x00,0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0x00},
  {0x00,0x18,0x00,0x38,0x18,0x18,0x3C,0x00},
  {0x00,0x0C,0x00,0x1C,0x0C,0x0C,0xCC,0x78},
  {0x00,0xC0,0xC0,0xCC,0xF8,0xCC,0xC6,0x00},
  {0x00,0x38,0x18,0x18,0x18,0x18,0x3C,0x00},
  {0x00,0x00,0x00,0xEC,0xFE,0xD6,0xC6,0x00},
  {0x00,0x00,0x00,0xFC,0xC6,0xC6,0xC6,0x00},
  {0x00,0x00,0x00,0x7C,0xC6,0xC6,0x7C,0x00},
  {0x00,0x00,0x00,0xFC,0xC6,0xC6,0xFC,0xC0},
  {0x00,0x00,0x00,0x7E,0xC6,0xC6,0x7E,0x06},
  {0x00,0x00,0x00,0xFC,0xC6,0xC0,0xC0,0x00},
  {0x00,0x00,0x00,0x7E,0xE0,0x3E,0xFC,0x00},
  {0x00,0x30,0x30,0xFE,0x30,0x30,0x1E,0x00},
  {0x00,0x00,0x00,0xC6,0xC6,0xC6,0x7E,0x00},
  {0x00,0x00,0x00,0xC6,0xC6,0x6C,0x38,0x00},
  {0x00,0x00,0x00,0xC6,0xD6,0xFE,0x6C,0x00},
  {0x00,0x00,0x00,0xC6,0x6C,0x6C,0xC6,0x00},
  {0x00,0x00,0x00,0xC6,0xC6,0x7E,0x06,0x7C},
  {0x00,0x00,0x00,0xFC,0x18,0x60,0xFC,0x00},
  {0x00,0x0C,0x18,0x70,0x18,0x0C,0x00,0x00},
  {0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
  {0x00,0x30,0x18,0x0E,0x18,0x30,0x00,0x00},
  {0x00,0x00,0x00,0x76,0xDC,0x00,0x00,0x00},
};

static void lcdDrawChar(int16_t x, int16_t y, char c, uint16_t fg, uint16_t bg) {
  if (c < 0x20 || c > 0x7E) c = ' ';
  const uint8_t *glyph = font8x8[c - 0x20];
  lcdSetWindow(x, y, x + 7, y + 7);
  tftSpi.beginTransaction(SPISettings(TFT_SPI_HZ, MSBFIRST, SPI_MODE0));
  lcdCsLow();
  lcdDcDat();
  for (int row = 0; row < 8; ++row) {
    uint8_t bits = glyph[row];
    for (int col = 0; col < 8; ++col) {
      tftSpi.transfer16((bits & 0x80) ? fg : bg);
      bits <<= 1;
    }
  }
  lcdCsHigh();
  tftSpi.endTransaction();
}

static void lcdDrawStr(int16_t x, int16_t y, const char *str, uint16_t fg, uint16_t bg) {
  while (*str) {
    lcdDrawChar(x, y, *str, fg, bg);
    x += 9;
    ++str;
  }
}

static constexpr uint16_t C_BLACK  = 0x0000;
static constexpr uint16_t C_WHITE  = 0xFFFF;
static constexpr uint16_t C_GREEN  = 0x07E0;
static constexpr uint16_t C_RED    = 0xF800;
static constexpr uint16_t C_CYAN   = 0x07FF;
static constexpr uint16_t C_GRAY   = 0xAD55;
static constexpr uint16_t C_YELLOW = 0xFFE0;

#define logf(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)

/* ------------------------------------------------------------------ */
/* SD card mount via ESP-IDF (same as launcher)                       */
/* ------------------------------------------------------------------ */
static bool sdMounted = false;
static sdmmc_card_t *sdCard = nullptr;

static bool initSdCard() {
  pinMode(SD_MISO_PIN, INPUT_PULLUP);
  pinMode(TFT_CS_PIN, OUTPUT);
  digitalWrite(TFT_CS_PIN, HIGH);

  spi_bus_config_t bus_cfg = {
      .mosi_io_num = SD_MOSI_PIN,
      .miso_io_num = SD_MISO_PIN,
      .sclk_io_num = SD_SCK_PIN,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4092,
      .flags = 0,
      .intr_flags = 0,
  };
  esp_err_t ret = spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK) {
    logf("[usbmsc] SD spi_bus_initialize failed: %d\n", ret);
    return false;
  }

  sdspi_device_config_t dev_cfg = {
      .host_id = SPI3_HOST,
      .gpio_cs = (gpio_num_t)SD_CS_PIN,
      .gpio_cd = SDSPI_SLOT_NO_CD,
      .gpio_wp = SDSPI_SLOT_NO_WP,
      .gpio_int = GPIO_NUM_NC,
  };

  sdmmc_host_t sd_host = SDSPI_HOST_DEFAULT();
  esp_vfs_fat_mount_config_t mount_cfg = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024,
  };
  sdmmc_card_t *card = nullptr;
  ret = esp_vfs_fat_sdspi_mount("/sd", &sd_host, &dev_cfg,
                                &mount_cfg, &card);
  if (ret != ESP_OK) {
    logf("[usbmsc] SD mount failed: %d\n", ret);
    spi_bus_free(SPI3_HOST);
    return false;
  }
  sdCard = card;
  sdMounted = true;
  logf("[usbmsc] SD mounted: %llu MB\n",
       (uint64_t)sdCard->csd.capacity * sdCard->csd.sector_size / (1024 * 1024));
  return true;
}

/* ------------------------------------------------------------------ */
/* USB MSC callbacks                                                   */
/* ------------------------------------------------------------------ */
static USBMSC msc;

static int32_t onMscRead(uint32_t lba, uint32_t offset, void *buf, uint32_t len) {
  if (!sdMounted) return -1;
  uint32_t sector = lba + offset / 512;
  uint32_t boff = offset % 512;
  if (boff == 0 && (len % 512) == 0) {
    if (disk_read(0, (BYTE*)buf, sector, len / 512) != RES_OK) return -1;
  } else {
    uint8_t *dst = (uint8_t*)buf;
    uint32_t remain = len;
    uint32_t sec = sector;
    uint32_t skip = boff;
    while (remain > 0) {
      uint8_t tmp[512];
      if (disk_read(0, tmp, sec, 1) != RES_OK) return -1;
      uint32_t cpy = 512 - skip;
      if (cpy > remain) cpy = remain;
      memcpy(dst, tmp + skip, cpy);
      dst += cpy;
      remain -= cpy;
      sec++;
      skip = 0;
    }
  }
  return len;
}

static int32_t onMscWrite(uint32_t lba, uint32_t offset, uint8_t *buf, uint32_t len) {
  if (!sdMounted) return -1;
  uint32_t sector = lba + offset / 512;
  uint32_t boff = offset % 512;
  if (boff == 0 && (len % 512) == 0) {
    if (disk_write(0, (BYTE*)buf, sector, len / 512) != RES_OK) return -1;
  } else {
    const uint8_t *src = (const uint8_t*)buf;
    uint32_t remain = len;
    uint32_t sec = sector;
    uint32_t skip = boff;
    while (remain > 0) {
      uint8_t tmp[512];
      if (disk_read(0, tmp, sec, 1) != RES_OK) return -1;
      uint32_t cpy = 512 - skip;
      if (cpy > remain) cpy = remain;
      memcpy(tmp + skip, src, cpy);
      if (disk_write(0, tmp, sec, 1) != RES_OK) return -1;
      src += cpy;
      remain -= cpy;
      sec++;
      skip = 0;
    }
  }
  return len;
}

static bool onMscStartStop(uint8_t power_condition, bool start, bool load_eject) {
  (void)power_condition;
  (void)start;
  (void)load_eject;
  return true;
}

/* ------------------------------------------------------------------ */
/* Exit to launcher (ota_0) using raw otadata writes                  */
/* ------------------------------------------------------------------ */
static void switchToOta0() {
  logf("[usbmsc] switchToOta0() called\n");
  struct __attribute__((packed)) OtaEntry {
    uint32_t ota_seq;
    uint8_t  seq_label[20];
    uint32_t ota_state;
    uint32_t crc;
  };
  auto setEntry = [](OtaEntry *e, uint32_t seq, uint32_t state) {
    memset(e, 0, sizeof(OtaEntry));
    e->ota_seq = seq;
    e->ota_state = state;
    e->crc = esp_rom_crc32_le(UINT32_MAX, (uint8_t *)&e->ota_seq, 4);
  };
  const esp_partition_t *otap = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
  if (otap) {
    OtaEntry entries[2];
    setEntry(&entries[0], 1, 2);
    setEntry(&entries[1], 3, 2);
    esp_partition_erase_range(otap, 0, otap->size);
    esp_partition_write(otap, 0,    &entries[0], sizeof(OtaEntry));
    esp_partition_write(otap, 4096, &entries[1], sizeof(OtaEntry));
    logf("[usbmsc] otadata written for ota_0\n");
  } else {
    logf("[usbmsc] ERROR: otadata partition NOT FOUND\n");
  }
  delay(100);
  logf("[usbmsc] ESP.restart()\n");
  ESP.restart();
}

static bool exitTriggered() {
  return digitalRead(KEY_SELECT_PIN) == 0 && digitalRead(KEY_BOOT_PIN) == 0;
}

/* ------------------------------------------------------------------ */
/* Setup / Loop                                                        */
/* ------------------------------------------------------------------ */
void setup() {
  disableLoopWDT();
  Serial.begin(115200);
  delay(100);
  logf("\n[usbmsc] ===== BOOT =====\n");

  esp_ota_mark_app_valid_cancel_rollback();
  logf("[usbmsc] mark_app_valid done\n");

  pinMode(KEY_SELECT_PIN, INPUT_PULLUP);
  pinMode(KEY_BOOT_PIN, INPUT_PULLUP);

  lcdInit();
  lcdFill(C_BLACK);
  lcdDrawStr(60, 90, "USB Disk Mode", C_WHITE, C_BLACK);
  lcdDrawStr(80, 108, "Init SD...", C_CYAN, C_BLACK);

  bool sdOk = initSdCard();
  if (sdOk) {
    uint64_t totalBytes = (uint64_t)sdCard->csd.capacity * sdCard->csd.sector_size;
    uint64_t mb = totalBytes / (1024 * 1024);
    logf("[usbmsc] SD ready: %llu MB\n", mb);

    lcdFill(C_BLACK);
    lcdDrawStr(60, 90, "USB Disk Mode", C_WHITE, C_BLACK);

    char line[32];
    snprintf(line, sizeof(line), "SD: %llu MB", mb);
    lcdDrawStr(74, 108, line, C_GREEN, C_BLACK);

    msc.vendorID("Espressif");
    msc.productID("ESP32-S3 SD");
    msc.productRevision("1.0");
    msc.onStartStop(onMscStartStop);
    msc.onRead(onMscRead);
    msc.onWrite(onMscWrite);
    msc.mediaPresent(true);
    msc.begin(sdCard->csd.capacity, sdCard->csd.sector_size);
    USB.begin();
    logf("[usbmsc] USB MSC started: %llu sectors\n", sdCard->csd.capacity);
  } else {
    logf("[usbmsc] SD FAILED\n");
    lcdFill(C_BLACK);
    lcdDrawStr(60, 90, "USB Disk Mode", C_RED, C_BLACK);
    lcdDrawStr(50, 108, "SD Card Init Failed", C_RED, C_BLACK);
  }

  logf("[usbmsc] startup guard 3s...\n");
  for (int i = 3; i > 0; --i) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Starting %d...", i);
    if (sdOk) {
      lcdFill(C_BLACK);
      lcdDrawStr(60, 90, "USB Disk Mode", C_WHITE, C_BLACK);
      lcdDrawStr(80, 108, buf, C_CYAN, C_BLACK);
    }
    delay(1000);
  }

  lcdFill(C_BLACK);
  lcdDrawStr(60, 90, "USB Disk Mode", C_WHITE, C_BLACK);
  lcdDrawStr(44, 108, "SELECT+START to exit", C_GRAY, C_BLACK);
  logf("[usbmsc] ===== READY =====\n");
}

void loop() {
  static unsigned long lastLog = 0;
  static bool lastExit = false;
  unsigned long now = millis();

  bool ex = exitTriggered();
  if (ex && !lastExit) {
    logf("[usbmsc] SELECT+START pressed, exiting...\n");
    lcdFill(C_BLACK);
    lcdDrawStr(60, 90, "USB Disk Mode", C_YELLOW, C_BLACK);
    lcdDrawStr(74, 108, "Exiting...", C_YELLOW, C_BLACK);
    delay(300);
    switchToOta0();
  }
  lastExit = ex;

  if (now - lastLog >= 5000) {
    lastLog = now;
    logf("[usbmsc] alive heap=%u\n", ESP.getFreeHeap());
  }

  delay(50);
}
