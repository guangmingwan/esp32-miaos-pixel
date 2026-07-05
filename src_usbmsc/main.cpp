#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <esp_ota_ops.h>
#include <esp_rom_crc.h>
#include <ff.h>        // FatFs types (BYTE, DRESULT, etc.)
#include <diskio.h>    // FatFs disk_read / disk_write prototypes
#include "USB.h"
#include "USBMSC.h"
#include "lcd_ili9342.h"
#include "pins.h"
#include "lava_native_display.h"

static SPIClass spiLCD(HSPI);
static SPIClass spiSD(FSPI);

static bool lavaReady = false;
static bool sdReady = false;
static uint64_t sdSectors = 0;
static USBMSC msc;

#define logf(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)

static void showScreen(const char *line1, const char *line2) {
  if (!lavaReady) return;
  lavaClear(1);
  if (line1) lavaDrawText(20, 90, line1, 0, 1);
  if (line2) lavaDrawText(20, 110, line2, 0, 1);
  lavaPresent();
}

// Exit trigger: SELECT + START (matches launcher convention)
static bool exitTriggered() {
  return digitalRead(KEY_SELECT_PIN) == 0 && digitalRead(KEY_BOOT_PIN) == 0;
}

// === USB MSC callbacks ===
static int32_t onMscRead(uint32_t lba, uint32_t offset, void *buf, uint32_t len) {
  if (!sdReady) return -1;
  // Calculate actual sector and byte offset
  uint32_t sector = lba + offset / 512;
  uint32_t boff = offset % 512;
  uint32_t count = (boff + len + 511) / 512;
  // Use a stack/heap buffer for partial-sector reads
  if (boff == 0 && (len % 512) == 0) {
    // Aligned read
    if (disk_read(0, (BYTE*)buf, sector, count) != RES_OK) return -1;
  } else {
    // Misaligned: read full sectors, copy partial
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
  if (!sdReady) return -1;
  uint32_t sector = lba + offset / 512;
  uint32_t boff = offset % 512;
  uint32_t count = (boff + len + 511) / 512;
  if (boff == 0 && (len % 512) == 0) {
    if (disk_write(0, (BYTE*)buf, sector, count) != RES_OK) return -1;
  } else {
    const uint8_t *src = (const uint8_t*)buf;
    uint32_t remain = len;
    uint32_t sec = sector;
    uint32_t skip = boff;
    while (remain > 0) {
      uint8_t tmp[512];
      // Read-modify-write: read existing sector first
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
  return true;
}

static void switchToOta0() {
  logf("[usbmsc] switchToOta0() called\n");
  typedef struct __attribute__((packed)) {
    uint32_t ota_seq;
    uint8_t  seq_label[20];
    uint32_t ota_state;
    uint32_t crc;
  } OtaEntry;
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

void setup() {
  disableLoopWDT();
  Serial.begin(115200);
  delay(100);
  logf("\n[usbmsc] ===== BOOT =====\n");

  esp_ota_mark_app_valid_cancel_rollback();
  logf("[usbmsc] mark_app_valid done\n");

  pinMode(KEY_SELECT_PIN, INPUT_PULLUP);
  pinMode(KEY_BOOT_PIN, INPUT_PULLUP);

  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, LOW);

  spiLCD.begin(TFT_SCK_PIN, TFT_MISO_PIN, TFT_MOSI_PIN, -1);
  Lcd.begin(spiLCD, TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN, -1, TFT_SPI_HZ);

  lavaDisplayInit();
  lavaReady = true;
  showScreen("USB Disk Mode", "Init...");

  spiSD.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, -1);
  sdReady = SD.begin(SD_CS_PIN, spiSD, SD_SPI_HZ);
  if (sdReady) {
    sdSectors = SD.cardSize() / 512;
    uint64_t mb = SD.cardSize() / (1024 * 1024);
    logf("[usbmsc] SD ready: %llu MB, %llu sectors\n", mb, sdSectors);
    String sdStr = "SD: " + String(mb) + " MB";
    showScreen("USB Disk Mode", sdStr.c_str());

    // Register USB MSC
    msc.vendorID("Espressif");
    msc.productID("ESP32-S3 SD");
    msc.productRevision("1.0");
    msc.onStartStop(onMscStartStop);
    msc.onRead(onMscRead);
    msc.onWrite(onMscWrite);
    msc.mediaPresent(true);
    msc.begin(sdSectors, 512);
    USB.begin();
    logf("[usbmsc] USB MSC started: %llu sectors\n", sdSectors);
  } else {
    logf("[usbmsc] SD FAILED\n");
    showScreen("USB Disk Mode", "No SD Card");
  }

  // Startup guard
  logf("[usbmsc] startup guard 3s...\n");
  for (int i = 3; i > 0; --i) {
    showScreen("USB Disk Mode", ("Starting " + String(i) + "...").c_str());
    delay(1000);
  }

  showScreen("USB Disk Mode", "SELECT+START to exit");
  logf("[usbmsc] ===== READY =====\n");
}

void loop() {
  static unsigned long lastLog = 0;
  static bool lastExit = false;
  unsigned long now = millis();

  bool ex = exitTriggered();
  if (ex && !lastExit) {
    logf("[usbmsc] SELECT+START pressed, exiting...\n");
    showScreen("USB Disk Mode", "Exiting...");
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
