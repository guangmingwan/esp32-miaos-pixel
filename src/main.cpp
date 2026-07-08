#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

#include <cstring>

#include <esp_ota_ops.h>
#include <esp_rom_crc.h>
#include <esp_timer.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>

#include <esp_vfs_fat.h>
#include <driver/sdspi_host.h>
#include <sdmmc_cmd.h>
#include <FSImpl.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

#include "app.h"
#include "apps/about_app.h"
#include "apps/log_viewer_app.h"
#include "apps/serial_transfer_app.h"
#include "launcher_log.h"
#include "ota_app_flash.h"
#include "lcd_ili9342.h"
#include "lava_text.h"
#include "lava_native_display.h"
#include "mia_i18n.h"
#include "pins.h"
#include "rtc_clock.h"
#include "sd_app_loader.h"

SPIClass tftSpi(FSPI);

enum class SdAppAction : uint8_t {
  DownloadAndRun = 0,
  UploadToSd = 1,
};

// Helper to set SD mountpoint after ESP-IDF VFS mount
struct SDFSAccess : public fs::SDFS {
  void setMountPt(const char *mp) { _impl->mountpoint(mp); }
};

AppContext g_context = {};
ButtonState g_allButtons[ALL_BUTTON_COUNT] = {};
static bool g_allButtonLast[ALL_BUTTON_COUNT] = {};
static uint8_t g_hc165State = 0xFF;
static int64_t g_lastWatchdogYieldUs = 0;
static uint32_t g_lastLauncherRenderMs = 0;
static bool g_launcherNeedsInitialRender = true;
static uint32_t g_launcherInitialRenderAtMs = 0;
static constexpr uint8_t INITIAL_DRAW_ITEM_LIMIT = 1;
static uint8_t g_selectedApp = 0;
static const LauncherApp *g_activeApp = nullptr;
static SdAppLoaderResult g_sdScan = {SdAppLoaderStatus::SdUnavailable, 0, 0};
static SdAppManifestSummary g_sdApps[16] = {};
static SdAppLoaderResult g_lastSdRun = {SdAppLoaderStatus::Ok, 0, 0};
static constexpr uint8_t BUTTON_INDEX_START = 1;
static constexpr uint8_t BUTTON_INDEX_SELECT = 5;
static constexpr uint8_t BUTTON_INDEX_A = 6;
static constexpr uint8_t BUTTON_INDEX_B = 7;
static constexpr uint8_t BUTTON_INDEX_UP = 10;
static constexpr uint8_t BUTTON_INDEX_DOWN = 11;
static constexpr uint8_t BUTTON_INDEX_LEFT = 12;
static constexpr uint8_t BUTTON_INDEX_RIGHT = 13;
static constexpr uint8_t BUTTON_INDEX_L = 3;
static constexpr uint8_t BUTTON_INDEX_R = 4;
static constexpr uint8_t SYSTEM_TAB_INDEX = 0;
static uint8_t g_selectedTab = SYSTEM_TAB_INDEX;
static bool g_bootLoaderHintVisible = false;
static bool g_sdActionMenuVisible = false;
static uint8_t g_sdActionMenuSelection = 0;

static bool g_otaExportConfirmVisible = false;
static bool g_otaExportResultVisible = false;
static OtaAppManifest g_otaExportManifest = {};
static bool g_otaExportSuccess = false;
static bool g_fontRestartPromptVisible = false;
static LavaFontFace g_pendingFontFace = LavaFontFace::Basic8;

static const SdAppManifestSummary *selectedSdApp(uint8_t itemIndex);
static inline void launcherRenderYield();
static const char *sdScanStatusText();
static const char *launcherTabName(uint8_t tabIndex);
static void drawLauncher();

static bool needsSafeLauncherFont(LavaFontFace face) {
  return miaLanguage() == MiaLanguage::Chinese && face != LavaFontFace::DroidGbk12 &&
         face != LavaFontFace::Small5x7;
}

static const LauncherApp *const BUILTIN_APPS[] = {
    &serialTransferApp(),
    &logViewerApp(),
    &aboutApp(),
};
static constexpr uint8_t BUILTIN_APP_COUNT = sizeof(BUILTIN_APPS) / sizeof(BUILTIN_APPS[0]);

static constexpr uint8_t LANGUAGE_MENU_INDEX = BUILTIN_APP_COUNT;
static constexpr uint8_t FONT_MENU_INDEX = BUILTIN_APP_COUNT + 1;
static constexpr uint8_t BOOTLOADER_MENU_INDEX = BUILTIN_APP_COUNT + 2;
static constexpr uint8_t EXPORT_OTA_MENU_INDEX = BUILTIN_APP_COUNT + 3;
static constexpr uint8_t TOTAL_LAUNCHER_ITEMS_FIXED = BUILTIN_APP_COUNT + 4;
static constexpr uint8_t SYSTEM_ITEM_COUNT = TOTAL_LAUNCHER_ITEMS_FIXED;

static void showBootloaderInstructions() {
  launcherTrace("[boot-loader] showing manual entry instructions");
  g_bootLoaderHintVisible = true;
}

static void openSdActionMenu() {
  if (g_selectedTab == SYSTEM_TAB_INDEX) {
    return;
  }
  if (selectedSdApp(g_selectedApp) == nullptr) {
    return;
  }
  launcherTrace("[sd-menu] open");
  g_sdActionMenuVisible = true;
  g_sdActionMenuSelection = 0;
}

static void closeSdActionMenu() {
  if (!g_sdActionMenuVisible) {
    return;
  }
  launcherTrace("[sd-menu] close");
  g_sdActionMenuVisible = false;
}

enum LavaPalette : uint8_t {
  LAVA_BLACK = 0,
  LAVA_WHITE = 1,
  LAVA_BLUE = 2,
  LAVA_GREEN = 3,
  LAVA_RED = 4,
  LAVA_YELLOW = 5,
  LAVA_CYAN = 6,
  LAVA_GRAY = 7,
  LAVA_DARK_BLUE = 8,
  LAVA_LIGHT_GRAY = 15,
};

static constexpr uint8_t LOADING_BITMAP_H = 22;
static constexpr uint8_t LOADING_BITMAP_BYTES_PER_ROW = 15;
static constexpr uint8_t LOADING_LINE1_W = 101;
static constexpr uint8_t LOADING_LINE2_W = 120;

static constexpr uint8_t kLoadingLine1Bitmap[LOADING_BITMAP_H * LOADING_BITMAP_BYTES_PER_ROW] = {
    0x00, 0x00, 0x00, 0x18, 0x00, 0x06, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00,
    0x3F, 0xFF, 0xC0, 0x18, 0x00, 0x06, 0x0C, 0x00, 0x00, 0x06, 0x00, 0x30, 0x00, 0x00, 0x00,
    0x00, 0x70, 0x07, 0xFF, 0xFE, 0x06, 0x0C, 0x00, 0x00, 0x06, 0x00, 0x3F, 0x80, 0x00, 0x00,
    0x00, 0x20, 0x07, 0xFF, 0xFE, 0x06, 0x0C, 0x00, 0x00, 0x06, 0x00, 0x3F, 0xC0, 0x00, 0x00,
    0x00, 0x20, 0x00, 0x30, 0x00, 0x06, 0x0C, 0x00, 0x00, 0x06, 0x00, 0x30, 0x00, 0x00, 0x00,
    0x08, 0x20, 0x00, 0x61, 0x80, 0x04, 0x0C, 0x03, 0xF0, 0xF6, 0x00, 0x30, 0x00, 0x00, 0x00,
    0x0C, 0x20, 0x00, 0xE1, 0x80, 0x04, 0x0C, 0x07, 0xF1, 0xFE, 0x1F, 0xFF, 0xF0, 0x00, 0x00,
    0x0C, 0x3F, 0x81, 0xC1, 0x80, 0x04, 0x0C, 0x06, 0x01, 0x8E, 0x3F, 0xFF, 0xF0, 0x00, 0x00,
    0x0C, 0x3F, 0xC1, 0x9F, 0xFC, 0x0C, 0x1C, 0x06, 0x03, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0C, 0x20, 0x03, 0x9F, 0xFC, 0x0E, 0x1C, 0x03, 0x83, 0x06, 0x00, 0x30, 0x00, 0x00, 0x00,
    0x0C, 0x20, 0x07, 0x81, 0x80, 0x0F, 0x1E, 0x01, 0xE3, 0x06, 0x00, 0x30, 0x00, 0x00, 0x00,
    0x0C, 0x20, 0x07, 0x81, 0x80, 0x0D, 0x9E, 0x00, 0x73, 0x06, 0x00, 0x38, 0x00, 0x00, 0x00,
    0x0C, 0x20, 0x01, 0x81, 0x80, 0x18, 0xB3, 0x00, 0x33, 0x06, 0x00, 0x3E, 0x00, 0x00, 0x00,
    0x0C, 0x20, 0x01, 0x81, 0x80, 0x18, 0x73, 0x00, 0x31, 0x8E, 0x00, 0x37, 0x80, 0x00, 0x00,
    0x0C, 0x20, 0x01, 0x81, 0x80, 0x30, 0x61, 0x87, 0xF1, 0xFE, 0x00, 0x31, 0x80, 0x00, 0x00,
    0x0C, 0x70, 0x01, 0xBF, 0xFC, 0x70, 0xC0, 0xC7, 0xE0, 0xF6, 0x00, 0x30, 0x00, 0x00, 0x00,
    0x7F, 0xFF, 0xE1, 0xBF, 0xFE, 0x61, 0x80, 0x60, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static constexpr uint8_t kLoadingLine2Bitmap[LOADING_BITMAP_H * LOADING_BITMAP_BYTES_PER_ROW] = {
    0x0C, 0x00, 0x00, 0x30, 0x58, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x06, 0x00,
    0x0C, 0x1F, 0xC3, 0xFE, 0x4C, 0x00, 0x30, 0x01, 0xFF, 0xFC, 0x7F, 0xF0, 0xC3, 0xFF, 0xFC,
    0x0C, 0x1F, 0xC3, 0xFE, 0x4C, 0x1F, 0xFF, 0xE1, 0x86, 0x0C, 0x06, 0x0C, 0xC3, 0xFF, 0xFC,
    0x7F, 0xD8, 0x40, 0x30, 0x40, 0x1F, 0xFF, 0xC1, 0x86, 0x0C, 0x0C, 0x0C, 0xC0, 0x06, 0x00,
    0x7F, 0xD8, 0x40, 0x70, 0xE0, 0x10, 0x00, 0x01, 0x86, 0x0C, 0x0C, 0x0C, 0xC1, 0xFF, 0xF8,
    0x0C, 0xD8, 0x47, 0xFF, 0xFE, 0x10, 0x20, 0x01, 0xFF, 0xFC, 0x0F, 0xCC, 0xC3, 0xFF, 0xFC,
    0x0C, 0xD8, 0x40, 0x40, 0x60, 0x10, 0x30, 0xC1, 0xFF, 0xFC, 0x1F, 0xCC, 0xC0, 0x06, 0x00,
    0x0C, 0xD8, 0x40, 0x60, 0x64, 0x13, 0x30, 0xC1, 0x86, 0x0C, 0x18, 0xCC, 0xC7, 0xFF, 0xFE,
    0x08, 0xD8, 0x43, 0xFE, 0x6C, 0x13, 0x19, 0x81, 0x86, 0x0C, 0x30, 0xCC, 0xC7, 0xFF, 0xFE,
    0x08, 0xD8, 0x41, 0xB0, 0x6C, 0x11, 0x99, 0x81, 0x86, 0x0C, 0x38, 0xCC, 0xC0, 0x3B, 0x00,
    0x18, 0xD8, 0x41, 0xB0, 0x68, 0x11, 0x99, 0x81, 0xFF, 0xFC, 0x6D, 0x8C, 0xC0, 0x71, 0x0C,
    0x18, 0xD8, 0x43, 0xFE, 0x78, 0x31, 0x9B, 0x01, 0xFF, 0xFC, 0x05, 0x8C, 0xC0, 0xE1, 0xB8,
    0x18, 0xD8, 0x41, 0xFC, 0x38, 0x30, 0xC3, 0x03, 0x06, 0x0C, 0x03, 0x0C, 0xC3, 0xE0, 0xE0,
    0x18, 0xD8, 0x40, 0x36, 0x30, 0x30, 0x83, 0x03, 0x06, 0x0C, 0x07, 0x00, 0xC7, 0x60, 0xE0,
    0x30, 0x9F, 0xC3, 0xFE, 0x72, 0x30, 0x06, 0x03, 0x06, 0x0C, 0x0E, 0x00, 0xC0, 0x60, 0x70,
    0x31, 0x9F, 0xC7, 0xF0, 0xFE, 0x67, 0xFF, 0xE6, 0x06, 0x0C, 0x1C, 0x00, 0xC0, 0x6E, 0x3C,
    0x67, 0x98, 0x40, 0x30, 0x1E, 0x67, 0xFF, 0xE6, 0x06, 0x7C, 0x38, 0x07, 0xC0, 0x7E, 0x0E,
    0x47, 0x00, 0x00, 0x30, 0x0C, 0x40, 0x00, 0x02, 0x00, 0x38, 0x20, 0x07, 0x80, 0x20, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static void drawLoadingBitmap(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t width,
                              uint8_t color) {
  for (uint8_t row = 0; row < LOADING_BITMAP_H; ++row) {
    for (uint8_t col = 0; col < width; ++col) {
      const uint8_t byte = bitmap[row * LOADING_BITMAP_BYTES_PER_ROW + col / 8];
      if ((byte & (0x80 >> (col % 8))) != 0) {
        lavaFillRect(x + col, y + row, 1, 1, color);
      }
    }
  }
}

static void drawSdAppLoadingMessage() {
  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(6, 2, miaTr("MiaOS Launcher"), LAVA_BLACK, LAVA_YELLOW);

  const int16_t line1X = (LAVA_SCREEN_W - LOADING_LINE1_W) / 2;
  const int16_t line2X = (LAVA_SCREEN_W - LOADING_LINE2_W) / 2;
  drawLoadingBitmap(line1X, 96, kLoadingLine1Bitmap, LOADING_LINE1_W, LAVA_CYAN);
  drawLoadingBitmap(line2X, 124, kLoadingLine2Bitmap, LOADING_LINE2_W, LAVA_WHITE);
  lavaPresent();
}

static String macAddress() {
  const uint64_t mac = ESP.getEfuseMac();
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
           static_cast<uint8_t>(mac >> 40), static_cast<uint8_t>(mac >> 32),
           static_cast<uint8_t>(mac >> 24), static_cast<uint8_t>(mac >> 16),
           static_cast<uint8_t>(mac >> 8), static_cast<uint8_t>(mac));
  return String(buffer);
}

static bool readPhysicalButton(size_t index) {
  if (index >= ALL_BUTTON_COUNT) return false;
  const ButtonProbe &b = ALL_BUTTONS[index];
  if (b.source == SRC_GPIO) {
    if (b.gpio < 0) return false;
    return digitalRead(b.gpio) == LOW;
  }
  return (g_hc165State & (1 << b.shiftBit)) == 0;
}

static void scanHc165() {
  digitalWrite(HC165_PL_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(HC165_PL_PIN, HIGH);

  uint8_t val = 0;
  for (uint8_t i = 0; i < 8; ++i) {
    val <<= 1;
    if (digitalRead(HC165_DAT_PIN) == HIGH) {
      val |= 1;
    }
    digitalWrite(HC165_CLK_PIN, HIGH);
    delayMicroseconds(2);
    digitalWrite(HC165_CLK_PIN, LOW);
  }
  g_hc165State = val;
}

static void configurePins() {
  pinMode(TFT_CS_PIN, OUTPUT);
  pinMode(TFT_DC_PIN, OUTPUT);
  digitalWrite(TFT_CS_PIN, HIGH);

  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);

  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, LOW);

  pinMode(BEEP_PIN, OUTPUT);
  digitalWrite(BEEP_PIN, LOW);

  pinMode(AMP_CTRL_PIN, OUTPUT);
  digitalWrite(AMP_CTRL_PIN, LOW);

  for (size_t i = 0; i < ALL_BUTTON_COUNT; ++i) {
    if (ALL_BUTTONS[i].source == SRC_GPIO && ALL_BUTTONS[i].gpio >= 0) {
      pinMode(ALL_BUTTONS[i].gpio, INPUT_PULLUP);
    }
  }

  pinMode(HC165_PL_PIN, OUTPUT);
  pinMode(HC165_CLK_PIN, OUTPUT);
  pinMode(HC165_DAT_PIN, INPUT);
  digitalWrite(HC165_PL_PIN, HIGH);
  digitalWrite(HC165_CLK_PIN, LOW);
}

static void initDisplay() {
  tftSpi.begin(TFT_SCK_PIN, TFT_MISO_PIN, TFT_MOSI_PIN, TFT_CS_PIN);
  Lcd.begin(tftSpi, TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN, TFT_BL_PIN, TFT_SPI_HZ);
  lavaDisplayInit();
  g_context.tftReady = true;
}

static void initSdCard() {
  digitalWrite(TFT_CS_PIN, HIGH);
#ifdef MIA_ENABLE_SD
  pinMode(SD_MISO_PIN, INPUT_PULLUP);
  g_context.sdReady = false;

  // Initialize SPI bus for SD card on HSPI (SPI3_HOST)
  spi_bus_config_t sd_bus_cfg = {
      .mosi_io_num = SD_MOSI_PIN,
      .miso_io_num = SD_MISO_PIN,
      .sclk_io_num = SD_SCK_PIN,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .data4_io_num = -1,
      .data5_io_num = -1,
      .data6_io_num = -1,
      .data7_io_num = -1,
      .max_transfer_sz = 4092,
      .flags = 0,
      .intr_flags = 0,
  };
  esp_err_t bus_ret = spi_bus_initialize(SPI3_HOST, &sd_bus_cfg,
                                          SPI_DMA_CH_AUTO);
  if (bus_ret != ESP_OK) {
    launcherTracef("[sd] spi_bus_initialize failed: %d", bus_ret);
  } else {
    sdspi_device_config_t dev_cfg = {
        .host_id = SPI3_HOST,
        .gpio_cs = (gpio_num_t)SD_CS_PIN,
        .gpio_cd = SDSPI_SLOT_NO_CD,
        .gpio_wp = SDSPI_SLOT_NO_WP,
        .gpio_int = GPIO_NUM_NC,
    };

    esp_vfs_fat_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };
    sdmmc_host_t sd_host = SDSPI_HOST_DEFAULT();
    sdmmc_card_t *card = nullptr;
    const esp_err_t ret = esp_vfs_fat_sdspi_mount("/sd", &sd_host, &dev_cfg,
                                                  &mount_cfg, &card);
    if (ret == ESP_OK) {
      reinterpret_cast<SDFSAccess *>(&SD)->setMountPt("/sd");
      g_context.sdReady = true;
      launcherTrace("[sd] mounted via ESP-IDF sdspi");
    } else {
      launcherTracef("[sd] mount failed: %d", ret);
      spi_bus_free(SPI3_HOST);
    }
  }

  if (!g_context.sdReady) {
    launcherTrace("[sd] continuing without SD");
  }
#else
  g_context.sdReady = false;
#endif
  g_sdScan = scanSdApps(g_sdApps, sizeof(g_sdApps) / sizeof(g_sdApps[0]),
                        g_context.sdReady);
}

void updateAllButtons() {
  scanHc165();
  for (size_t i = 0; i < ALL_BUTTON_COUNT; ++i) {
    const bool down = readPhysicalButton(i);
    g_allButtons[i].down = down;
    g_allButtons[i].pressed = down && !g_allButtonLast[i];
    g_allButtons[i].released = !down && g_allButtonLast[i];
    g_allButtonLast[i] = down;
  }

  static const uint8_t appMap[6] = {BUTTON_INDEX_A, BUTTON_INDEX_B,
                                    BUTTON_INDEX_UP, BUTTON_INDEX_DOWN,
                                    BUTTON_INDEX_L, BUTTON_INDEX_R};
  for (size_t i = 0; i < 6; ++i) {
    g_context.buttons[i] = g_allButtons[appMap[i]];
  }
}

static bool systemExitPressed() {
  const ButtonState &start = g_allButtons[BUTTON_INDEX_START];
  const ButtonState &select = g_allButtons[BUTTON_INDEX_SELECT];
  return (select.down && start.pressed) || (start.down && select.pressed);
}

static void updateBeep() {
  bool anyPressed = false;
  for (size_t i = 0; i < ALL_BUTTON_COUNT; ++i) {
    anyPressed = anyPressed || g_allButtons[i].down;
  }
  digitalWrite(BEEP_PIN, anyPressed ? HIGH : LOW);
}

static const char *sdScanStatusText() {
  switch (g_sdScan.status) {
    case SdAppLoaderStatus::Ok:
      return miaTr("SD card:ready");
    case SdAppLoaderStatus::SdUnavailable:
      return miaTr("SD card:unavailable");
    case SdAppLoaderStatus::NoAppsFound:
      return miaTr("SD card:no apps");
    case SdAppLoaderStatus::ReadError:
      return miaTr("SD card:read error");
    case SdAppLoaderStatus::RunError:
      return miaTr("SD card:run error");
  }
  return miaTr("SD card:unknown");
}

static bool sameSdCategory(const char *lhs, const char *rhs) {
  return strcmp(lhs, rhs) == 0;
}

static uint8_t sdCategoryTabCount() {
  uint8_t count = 0;
  for (uint8_t index = 0;
       index < g_sdScan.appCount && index < sizeof(g_sdApps) / sizeof(g_sdApps[0]); ++index) {
    bool seen = false;
    for (uint8_t previous = 0; previous < index; ++previous) {
      if (sameSdCategory(g_sdApps[index].category, g_sdApps[previous].category)) {
        seen = true;
        break;
      }
    }
    if (!seen && count < UINT8_MAX) {
      ++count;
    }
  }
  return count;
}

static uint8_t launcherTabCount() {
  return 1 + sdCategoryTabCount();
}

static const char *launcherTabName(uint8_t tabIndex) {
  if (tabIndex == SYSTEM_TAB_INDEX) {
    return miaTr("System");
  }

  const uint8_t sdTabIndex = tabIndex - 1;
  uint8_t count = 0;
  for (uint8_t index = 0;
       index < g_sdScan.appCount && index < sizeof(g_sdApps) / sizeof(g_sdApps[0]); ++index) {
    bool seen = false;
    for (uint8_t previous = 0; previous < index; ++previous) {
      if (sameSdCategory(g_sdApps[index].category, g_sdApps[previous].category)) {
        seen = true;
        break;
      }
    }
    if (!seen) {
      if (count == sdTabIndex) {
        return g_sdApps[index].category;
      }
      ++count;
    }
  }
  return nullptr;
}

static uint8_t sdAppCountForTab(uint8_t tabIndex) {
  const char *tabName = launcherTabName(tabIndex);
  if (tabName == nullptr || tabIndex == SYSTEM_TAB_INDEX) {
    return 0;
  }

  uint8_t count = 0;
  for (uint8_t index = 0;
       index < g_sdScan.appCount && index < sizeof(g_sdApps) / sizeof(g_sdApps[0]); ++index) {
    if (sameSdCategory(g_sdApps[index].category, tabName) && count < UINT8_MAX) {
      ++count;
    }
  }
  return count;
}

static uint8_t totalLauncherItems() {
  return g_selectedTab == SYSTEM_TAB_INDEX ? SYSTEM_ITEM_COUNT : sdAppCountForTab(g_selectedTab);
}

static const SdAppManifestSummary *selectedSdApp(uint8_t itemIndex) {
  const char *tabName = launcherTabName(g_selectedTab);
  if (tabName == nullptr || g_selectedTab == SYSTEM_TAB_INDEX) {
    return nullptr;
  }

  uint8_t currentIndex = 0;
  for (uint8_t index = 0;
       index < g_sdScan.appCount && index < sizeof(g_sdApps) / sizeof(g_sdApps[0]); ++index) {
    if (!sameSdCategory(g_sdApps[index].category, tabName)) {
      continue;
    }
    if (currentIndex == itemIndex) {
      return &g_sdApps[index];
    }
    ++currentIndex;
  }
  return nullptr;
}

static void clampLauncherSelection() {
  const uint8_t itemCount = totalLauncherItems();
  if (itemCount == 0) {
    g_selectedApp = 0;
  } else if (g_selectedApp >= itemCount) {
    g_selectedApp = itemCount - 1;
  }
}

static void switchLauncherTab(int8_t delta) {
  const uint8_t tabCount = launcherTabCount();
  if (tabCount <= 1) {
    return;
  }

  int nextTab = static_cast<int>(g_selectedTab) + delta;
  if (nextTab < 0) {
    nextTab += tabCount;
  }
  if (nextTab >= tabCount) {
    nextTab -= tabCount;
  }
  g_selectedTab = static_cast<uint8_t>(nextTab);
  clampLauncherSelection();
}

static void drawLauncherClock() {
  RtcDateTime rtcNow = {2000, 1, 1, 0, 0, 0, 6};
  char clockText[32];
  if (rtcReadDateTime(rtcNow)) {
    snprintf(clockText, sizeof(clockText), "%s %04u-%02u-%02u %02u:%02u:%02u",
             rtcWeekdayShortName(rtcNow.weekday), rtcNow.year, rtcNow.month,
             rtcNow.day, rtcNow.hour, rtcNow.minute, rtcNow.second);
  } else {
    snprintf(clockText, sizeof(clockText), "%s", miaTr("RTC unavailable"));
  }
  const int16_t textX = LAVA_SCREEN_W - 6 - lavaTextWidth(clockText);
  lavaDrawText(textX, 2, clockText, LAVA_BLACK, LAVA_YELLOW);
}

extern "C" void esp32_task_wdt_reset(void) {
  const int64_t now_us = esp_timer_get_time();
  if (now_us - g_lastWatchdogYieldUs >= 500000) {
    delay(1);
    g_lastWatchdogYieldUs = now_us;
  }
}

static inline void launcherRenderYield() {
  esp32_task_wdt_reset();
}

static void drawLauncher() {
  if (!g_context.tftReady) {
    return;
  }

  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(6, 2, miaTr("MiaOS Launcher"), LAVA_BLACK, LAVA_YELLOW);
  drawLauncherClock();
  launcherRenderYield();
  if (g_launcherNeedsInitialRender) {
    lavaPresent();
    launcherRenderYield();
    return;
  }

  int16_t tabX = 8;
  const uint8_t tabCount = launcherTabCount();
  for (uint8_t tab = 0; tab < tabCount; ++tab) {
    const char *tabNameKey = launcherTabName(tab);
    if (tabNameKey == nullptr) {
      continue;
    }
    const char *tabName = miaTr(tabNameKey);
    const uint8_t bg = tab == g_selectedTab ? LAVA_BLUE : LAVA_BLACK;
    const uint8_t fg = tab == g_selectedTab ? LAVA_YELLOW : LAVA_GRAY;
    const int16_t tabWidth = static_cast<int16_t>(lavaTextWidth(tabName) + 12);
    lavaFillRect(tabX, 28, tabWidth, 20, bg);
    lavaDrawText(tabX + 6, 30, tabName, fg, bg);
    tabX += tabWidth + 4;
    launcherRenderYield();
  }
  launcherRenderYield();

  const uint8_t maxVisibleApps = 7;
  uint8_t firstApp = 0;
  const uint8_t itemCount = totalLauncherItems();
  if (itemCount > maxVisibleApps && g_selectedApp >= maxVisibleApps) {
    firstApp = g_selectedApp - maxVisibleApps + 1;
  }
  uint8_t visibleEnd = min<uint8_t>(itemCount, firstApp + maxVisibleApps);
  if (g_launcherNeedsInitialRender) {
    visibleEnd = min<uint8_t>(visibleEnd, firstApp + INITIAL_DRAW_ITEM_LIMIT);
  }
  for (uint8_t i = firstApp; i < visibleEnd; ++i) {
    const int16_t y = 62 + (i - firstApp) * 20;
    const bool selected = i == g_selectedApp;
    const bool sdApp = g_selectedTab != SYSTEM_TAB_INDEX;
    const bool isLanguage = g_selectedTab == SYSTEM_TAB_INDEX && i == LANGUAGE_MENU_INDEX;
    const bool isFont = g_selectedTab == SYSTEM_TAB_INDEX && i == FONT_MENU_INDEX;
    const bool isBootloader = g_selectedTab == SYSTEM_TAB_INDEX && i == BOOTLOADER_MENU_INDEX;
    const bool isExportOta = g_selectedTab == SYSTEM_TAB_INDEX && i == EXPORT_OTA_MENU_INDEX;
    const char *name;
    char languageLine[48];
    char fontLine[48];
    if (isLanguage) {
      snprintf(languageLine, sizeof(languageLine), "%s: %s", miaTr("Language"),
               miaLanguageName(miaLanguage()));
      name = languageLine;
    } else if (isFont) {
      snprintf(fontLine, sizeof(fontLine), "%s: %s", miaTr("Font"),
               lavaFontName(lavaFontFace()));
      name = fontLine;
    } else if (isBootloader) {
      name = miaTr("Boot Loader");
    } else if (isExportOta) {
      name = miaTr("Export OTA to SD");
    } else if (sdApp) {
      const SdAppManifestSummary *app = selectedSdApp(i);
      name = app == nullptr ? miaTr("<missing>") : miaTr(app->name);
    } else {
      name = miaTr(BUILTIN_APPS[i]->name);
    }
    const uint8_t itemBg = selected ? LAVA_BLUE : LAVA_BLACK;
    const uint8_t itemText = selected ? LAVA_BLACK : LAVA_WHITE;
    const uint8_t itemTag = selected ? LAVA_BLACK : LAVA_GREEN;
    const uint8_t itemCursor = selected ? LAVA_YELLOW : LAVA_YELLOW;
    lavaFillRect(6, y - 2, 308, 18, itemBg);
    lavaDrawText(12, y, selected ? ">" : " ", itemCursor, itemBg);
    lavaDrawText(28, y, sdApp ? "[sd]" : "", itemTag, itemBg);
    lavaDrawText(sdApp ? 52 : 28, y, name, itemText, itemBg);
    launcherRenderYield();
  }
  launcherRenderYield();

  const char *sdStatus = sdScanStatusText();
  const char *controlText = miaTr("A:Open UP/DN:Move LEFT/RIGHT:Tab");
  const int16_t sdStatusWidth = lavaTextWidth(sdStatus);
  const int16_t controlWidth = lavaTextWidth(controlText);
  const bool splitFooter = sdStatusWidth + controlWidth + 24 > LAVA_SCREEN_W;
  const int16_t sdStatusX = LAVA_SCREEN_W - 8 - sdStatusWidth;
  lavaDrawText(sdStatusX, splitFooter ? 204 : 224, sdStatus,
               g_context.sdReady ? LAVA_GREEN : LAVA_RED, LAVA_BLACK);
  if (g_lastSdRun.status != SdAppLoaderStatus::Ok) {
    char errorLine[32];
    snprintf(errorLine, sizeof(errorLine), "%s %s:%d",
             miaTr(sdAppLoaderStatusText(g_lastSdRun.status)), miaTr("code"),
             g_lastSdRun.errorCode);
    lavaDrawText(8, 214, errorLine, LAVA_RED, LAVA_BLACK);
  }
  lavaDrawText(8, 224, controlText, LAVA_GRAY, LAVA_BLACK);
  launcherRenderYield();

  if (g_bootLoaderHintVisible) {
    lavaFillRect(24, 44, 272, 156, LAVA_DARK_BLUE);
    lavaFillRect(24, 44, 272, 20, LAVA_YELLOW);
    lavaDrawText(30, 46, miaTr("Boot Loader"), LAVA_BLACK, LAVA_YELLOW);
    lavaDrawText(40, 82, miaTr("1. Hold ST"), LAVA_WHITE, LAVA_DARK_BLUE);
    lavaDrawText(40, 100, miaTr("2. Press RESET"), LAVA_WHITE, LAVA_DARK_BLUE);
    lavaDrawText(40, 118, miaTr("3. Release RESET into"), LAVA_WHITE, LAVA_DARK_BLUE);
    lavaDrawText(58, 136, miaTr("download mode"), LAVA_CYAN, LAVA_DARK_BLUE);
    lavaDrawText(40, 162, miaTr("RESET alone returns to"), LAVA_GRAY, LAVA_DARK_BLUE);
    lavaDrawText(58, 180, miaTr("normal boot"), LAVA_GRAY, LAVA_DARK_BLUE);
    lavaDrawText(86, 198, miaTr("A/B:Back"), LAVA_YELLOW, LAVA_DARK_BLUE);
    launcherRenderYield();
  }

  if (g_sdActionMenuVisible) {
    static constexpr const char *MENU_ITEMS[] = {
        "Download and run",
        "Upload to SD",
    };
    const SdAppManifestSummary *app = selectedSdApp(g_selectedApp);
    lavaFillRect(26, 52, 268, 136, LAVA_GRAY);
    lavaFillRect(28, 54, 264, 132, LAVA_LIGHT_GRAY);
    lavaFillRect(28, 54, 264, 20, LAVA_YELLOW);
    lavaDrawText(34, 56, app == nullptr ? miaTr("SD App") : miaTr(app->name), LAVA_BLACK,
                 LAVA_YELLOW);
    for (uint8_t i = 0; i < 2; ++i) {
      const bool selected = i == g_sdActionMenuSelection;
      const uint8_t bg = selected ? LAVA_BLUE : LAVA_LIGHT_GRAY;
      const uint8_t fg = selected ? LAVA_YELLOW : LAVA_BLACK;
      lavaFillRect(40, 88 + i * 22, 240, 16, bg);
      lavaDrawText(52, 92 + i * 22, miaTr(MENU_ITEMS[i]), fg, bg);
    }
    lavaDrawText(52, 148, miaTr("A:Confirm  B/SEL:Back"), LAVA_DARK_BLUE, LAVA_LIGHT_GRAY);
    launcherRenderYield();
  }

  if (g_otaExportConfirmVisible) {
    constexpr int16_t boxW = 280;
    constexpr int16_t boxH = 160;
    constexpr int16_t boxX = (LAVA_SCREEN_W - boxW) / 2;
    constexpr int16_t boxY = (LAVA_SCREEN_H - boxH) / 2;
    lavaFillRect(boxX, boxY, boxW, boxH, LAVA_GRAY);
    lavaFillRect(boxX + 2, boxY + 2, boxW - 4, boxH - 4, LAVA_LIGHT_GRAY);
    lavaFillRect(boxX + 2, boxY + 2, boxW - 4, 18, LAVA_YELLOW);
    lavaDrawText(boxX + 8, boxY + 4, miaTr("Export OTA to SD"), LAVA_BLACK, LAVA_YELLOW);
    char catLine[48];
    snprintf(catLine, sizeof(catLine), miaTr("Category: %s"), miaTr(g_otaExportManifest.category));
    lavaDrawText(boxX + 12, boxY + 30, catLine, LAVA_BLACK, LAVA_LIGHT_GRAY);
    char nameLine[48];
    snprintf(nameLine, sizeof(nameLine), miaTr("Name: %s"), miaTr(g_otaExportManifest.name));
    lavaDrawText(boxX + 12, boxY + 48, nameLine, LAVA_BLACK, LAVA_LIGHT_GRAY);
    char pathLine[120];
    snprintf(pathLine, sizeof(pathLine), miaTr("To: /MiaOS/%s/%s.app/%s.bin"),
             g_otaExportManifest.category, g_otaExportManifest.name,
             g_otaExportManifest.name);
    lavaDrawText(boxX + 12, boxY + 72, pathLine, LAVA_BLACK, LAVA_LIGHT_GRAY);
    lavaDrawText(boxX + 12, boxY + 96, miaTr("Press A to export, B to cancel"),
                 LAVA_DARK_BLUE, LAVA_LIGHT_GRAY);
    launcherRenderYield();
  }

  if (g_otaExportResultVisible) {
    constexpr int16_t boxW = 240;
    constexpr int16_t boxH = 80;
    constexpr int16_t boxX = (LAVA_SCREEN_W - boxW) / 2 + 5;
    constexpr int16_t boxY = (LAVA_SCREEN_H - boxH) / 2;
    lavaFillRect(boxX, boxY, boxW, boxH, LAVA_GRAY);
    lavaFillRect(boxX + 2, boxY + 2, boxW - 4, boxH - 4, LAVA_LIGHT_GRAY);
    lavaFillRect(boxX + 2, boxY + 2, boxW - 4, 18, g_otaExportSuccess ? LAVA_GREEN : LAVA_RED);
    lavaDrawText(boxX + 8, boxY + 4, g_otaExportSuccess ? miaTr("Export OK") : miaTr("Export Failed"),
                 LAVA_BLACK, g_otaExportSuccess ? LAVA_GREEN : LAVA_RED);
    const char *msg = g_otaExportSuccess
        ? miaTr("OTA app exported to SD.")
        : miaTr("No valid manifest in ota_1.");
    lavaDrawText(boxX + 12, boxY + 36, msg, LAVA_BLACK, LAVA_LIGHT_GRAY);
    lavaDrawText(boxX + 12, boxY + 58, miaTr("Press any button"), LAVA_DARK_BLUE, LAVA_LIGHT_GRAY);
    launcherRenderYield();
  }

  if (g_fontRestartPromptVisible) {
    constexpr int16_t boxW = 248;
    constexpr int16_t boxH = 90;
    constexpr int16_t boxX = (LAVA_SCREEN_W - boxW) / 2;
    constexpr int16_t boxY = (LAVA_SCREEN_H - boxH) / 2;
    lavaFillRect(boxX, boxY, boxW, boxH, LAVA_GRAY);
    lavaFillRect(boxX + 2, boxY + 2, boxW - 4, boxH - 4, LAVA_LIGHT_GRAY);
    lavaFillRect(boxX + 2, boxY + 2, boxW - 4, 18, LAVA_YELLOW);
    lavaDrawText(boxX + 8, boxY + 4, miaTr("Font"), LAVA_BLACK, LAVA_YELLOW);
    char pendingFontLine[48];
    snprintf(pendingFontLine, sizeof(pendingFontLine), "%s: %s", miaTr("Apply font"),
             lavaFontName(g_pendingFontFace));
    lavaDrawText(boxX + 12, boxY + 30, pendingFontLine, LAVA_BLACK, LAVA_LIGHT_GRAY);
    lavaDrawText(boxX + 12, boxY + 48, miaTr("Press A to apply and restart"), LAVA_DARK_BLUE,
                 LAVA_LIGHT_GRAY);
    lavaDrawText(boxX + 12, boxY + 66, miaTr("B/SEL:Later"), LAVA_DARK_BLUE,
                 LAVA_LIGHT_GRAY);
    launcherRenderYield();
  }

  launcherRenderYield();
  lavaPresent();
  launcherRenderYield();
}

static void runSelectedSdAction() {
  const SdAppManifestSummary *app = selectedSdApp(g_selectedApp);
  if (app == nullptr) {
    closeSdActionMenu();
    return;
  }

  if (g_sdActionMenuSelection == static_cast<uint8_t>(SdAppAction::DownloadAndRun)) {
    g_lastSdRun = runSdAppByPath(app->path, g_context.sdReady);
    launcherLogRecordSdRun(*app, g_lastSdRun);
    return;
  }

  if (g_sdActionMenuSelection == static_cast<uint8_t>(SdAppAction::UploadToSd)) {
    g_lastSdRun = exportSdAppByPath(app->path, g_context.sdReady);
    launcherLogRecordSdRun(*app, g_lastSdRun);
  }
  closeSdActionMenu();
  drawLauncher();
}

static void enterSelectedApp() {
  if (g_selectedApp >= totalLauncherItems()) {
    return;
  }

  if (g_selectedTab == SYSTEM_TAB_INDEX && g_selectedApp == BOOTLOADER_MENU_INDEX) {
    showBootloaderInstructions();
    drawLauncher();
    return;
  }

  if (g_selectedTab == SYSTEM_TAB_INDEX && g_selectedApp == LANGUAGE_MENU_INDEX) {
    miaCycleLanguage();
    launcherTracef("[language] switched to %s", miaLanguageName(miaLanguage()));
    drawLauncher();
    return;
  }

  if (g_selectedTab == SYSTEM_TAB_INDEX && g_selectedApp == FONT_MENU_INDEX) {
    const uint8_t next = static_cast<uint8_t>(lavaFontFace()) + 1;
    g_pendingFontFace = next > static_cast<uint8_t>(LavaFontFace::DroidGbk12)
        ? LavaFontFace::Small5x7
        : static_cast<LavaFontFace>(next);
    launcherTracef("[font] pending switch to %s", lavaFontName(g_pendingFontFace));
    g_fontRestartPromptVisible = true;
    drawLauncher();
    return;
  }

  if (g_selectedTab == SYSTEM_TAB_INDEX && g_selectedApp == EXPORT_OTA_MENU_INDEX) {
    launcherTrace("[ota-export-menu] Export OTA to SD selected");
    if (!g_context.sdReady) {
      launcherTrace("[ota-export-menu] SD unavailable");
      g_lastSdRun = {SdAppLoaderStatus::SdUnavailable, 0, 0};
      drawLauncher();
      return;
    }
    if (!miaReadOtaManifest(&g_otaExportManifest)) {
      launcherTrace("[ota-export-menu] no manifest in ota_1");
      g_otaExportSuccess = false;
      g_otaExportResultVisible = true;
      drawLauncher();
      return;
    }
    g_otaExportConfirmVisible = true;
    drawLauncher();
    return;
  }

  if (g_selectedTab != SYSTEM_TAB_INDEX) {
    const SdAppManifestSummary *app = selectedSdApp(g_selectedApp);
    if (app == nullptr) {
      return;
    }

    // If the SD firmware manifest matches ota_1, skip flash and just boot
    if (sdManifestMatchesOta(app->path)) {
      launcherTracef("[sd-run] manifest matches ota_1, skipping flash for %s", app->path);
      miaBootAppSlot();
      return;  // miaBootAppSlot reboots, never returns
    }

    g_lastSdRun = runSdAppByPath(app->path, g_context.sdReady);
    launcherLogRecordSdRun(*app, g_lastSdRun);
    drawLauncher();
    return;
  }

  g_activeApp = BUILTIN_APPS[g_selectedApp];
  if (g_activeApp->begin != nullptr) {
    g_activeApp->begin(g_context);
  }
}

static void exitActiveApp() {
  if (g_activeApp != nullptr && g_activeApp->end != nullptr) {
    g_activeApp->end(g_context);
  }
  g_activeApp = nullptr;
  g_lastLauncherRenderMs = 0;
  drawLauncher();
}

static void tickLauncher(uint32_t nowMs) {
  if (g_bootLoaderHintVisible) {
    if (g_context.buttons[0].pressed || g_context.buttons[1].pressed || systemExitPressed()) {
      g_bootLoaderHintVisible = false;
      drawLauncher();
    }
    return;
  }

  if (g_sdActionMenuVisible) {
    if (g_context.buttons[2].pressed && g_sdActionMenuSelection > 0) {
      --g_sdActionMenuSelection;
      drawLauncher();
      return;
    }
    if (g_context.buttons[3].pressed && g_sdActionMenuSelection < 1) {
      ++g_sdActionMenuSelection;
      drawLauncher();
      return;
    }
    if (g_context.buttons[0].pressed) {
      runSelectedSdAction();
      return;
    }
    if (g_context.buttons[1].pressed || g_allButtons[BUTTON_INDEX_SELECT].pressed || systemExitPressed()) {
      closeSdActionMenu();
      drawLauncher();
      return;
    }
    return;
  }

  if (g_otaExportConfirmVisible) {
    if (g_context.buttons[0].pressed) {
      g_otaExportConfirmVisible = false;
      launcherTrace("[ota-export] A pressed, starting export");
      OtaAppExportResult result = miaExportOtaToSd(g_context.sdReady);
      launcherTracef("[ota-export] result status=%s code=%d",
                     miaOtaAppExportStatusText(result.status), result.errorCode);
      g_otaExportSuccess = (result.status == OtaAppExportStatus::Ok);
      g_otaExportResultVisible = true;
      drawLauncher();
      return;
    }
    if (g_context.buttons[1].pressed || g_allButtons[BUTTON_INDEX_SELECT].pressed || systemExitPressed()) {
      g_otaExportConfirmVisible = false;
      launcherTrace("[ota-export] cancelled");
      drawLauncher();
      return;
    }
    return;
  }

  if (g_otaExportResultVisible) {
    if (g_context.buttons[0].pressed || g_context.buttons[1].pressed ||
        g_allButtons[BUTTON_INDEX_SELECT].pressed || systemExitPressed()) {
      g_otaExportResultVisible = false;
      drawLauncher();
      return;
    }
    return;
  }

  if (g_fontRestartPromptVisible) {
    if (g_context.buttons[0].pressed) {
      lavaSetFontFace(g_pendingFontFace);
      launcherTracef("[font] applied %s, restart confirmed", lavaFontName(g_pendingFontFace));
      delay(50);
      ESP.restart();
      return;
    }
    if (g_context.buttons[1].pressed || g_allButtons[BUTTON_INDEX_SELECT].pressed ||
        systemExitPressed()) {
      g_fontRestartPromptVisible = false;
      launcherTrace("[font] restart deferred");
      drawLauncher();
      return;
    }
    return;
  }

  if (g_allButtons[BUTTON_INDEX_LEFT].pressed) {
    switchLauncherTab(-1);
    drawLauncher();
    return;
  }
  if (g_allButtons[BUTTON_INDEX_RIGHT].pressed) {
    switchLauncherTab(1);
    drawLauncher();
    return;
  }

  if (g_context.buttons[2].pressed && g_selectedApp > 0) {
    --g_selectedApp;
    drawLauncher();
  }
  if (g_context.buttons[3].pressed && g_selectedApp + 1 < totalLauncherItems()) {
    ++g_selectedApp;
    drawLauncher();
  }
  if (g_context.buttons[0].pressed) {
    enterSelectedApp();
    return;
  }
  if (g_allButtons[BUTTON_INDEX_SELECT].pressed && g_selectedTab != SYSTEM_TAB_INDEX) {
    openSdActionMenu();
    drawLauncher();
    return;
  }

  if (nowMs - g_lastLauncherRenderMs >= 1000) {
    g_lastLauncherRenderMs = nowMs;
    drawLauncher();
  }
}

static void printStartupInfo() {
  launcherTrace("");
  launcherTrace("ESP32-S3 Retro-Pixel launcher (ILI9342 320x240)");
  launcherTracef("Chip: %s rev%d, CPU %uMHz", ESP.getChipModel(),
                 ESP.getChipRevision(), ESP.getCpuFreqMHz());
  launcherTracef("Flash: %u bytes, heap: %u bytes", ESP.getFlashChipSize(),
                 ESP.getFreeHeap());
  launcherTracef("MAC: %s", macAddress().c_str());
  launcherTracef("TFT: ILI9342 320x240 SCK=%d MOSI=%d CS=%d DC=%d RST=%d BL=%d",
                 TFT_SCK_PIN, TFT_MOSI_PIN, TFT_CS_PIN, TFT_DC_PIN,
                 TFT_RST_PIN, TFT_BL_PIN);
  launcherTracef("SD: SCK=%d MOSI=%d MISO=%d CS=%d", SD_SCK_PIN, SD_MOSI_PIN,
                 SD_MISO_PIN, SD_CS_PIN);
  launcherTracef("Keys: BOOT=%d M=%d L=%d R=%d SEL=%d ST=%d", KEY_BOOT_PIN,
                 KEY_M_PIN, KEY_L_PIN, KEY_R_PIN, KEY_SELECT_PIN, KEY_START_PIN);
  launcherTracef("HC165: PL=%d CLK=%d DAT=%d", HC165_PL_PIN, HC165_CLK_PIN,
                 HC165_DAT_PIN);
  launcherTracef("Audio: I2S WS=%d BCK=%d DAT=%d AMP=%d", I2S_WS_PIN,
                 I2S_BCK_PIN, I2S_DATA_PIN, AMP_CTRL_PIN);
  launcherTracef("Misc: VBAT=%d BEEP=%d I2C SCL=%d SDA=%d", VBAT_ADC_PIN,
                 BEEP_PIN, I2C_SCL_PIN, I2C_SDA_PIN);
}

void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("ESP32-S3 Retro-Pixel launcher (ILI9342 320x240)");
  Serial.printf("Chip: %s rev%d, CPU %uMHz\n", ESP.getChipModel(),
                ESP.getChipRevision(), ESP.getCpuFreqMHz());
  launcherTrace("[setup] configurePins");
  configurePins();
  launcherTrace("[setup] Wire.begin");
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  launcherTrace("[setup] updateAllButtons");
  updateAllButtons();

  launcherTrace("[setup] printStartupInfo");
  printStartupInfo();
  initDisplay();
  launcherTrace("[setup] initDisplay done");
  const LavaFontFace persistedFont = lavaFontFace();
  if (needsSafeLauncherFont(persistedFont)) {
    launcherTracef("[font] startup session override %s -> %s", lavaFontName(persistedFont),
                   lavaFontName(LavaFontFace::DroidGbk12));
    lavaUseFontFaceForSession(LavaFontFace::DroidGbk12);
  }
  initSdCard();
  launcherTrace("[setup] initSdCard done");

  drawSdAppLoadingMessage();
  launcherTrace("[setup] drawSdAppLoadingMessage done");

  clampLauncherSelection();
  g_launcherInitialRenderAtMs = millis() + 500;
}

void loop() {
  const uint32_t now = millis();

  updateAllButtons();
  updateBeep();

  if (g_activeApp != nullptr) {
    if (systemExitPressed()) {
      exitActiveApp();
      return;
    }

    if (g_activeApp->tick != nullptr) {
      g_activeApp->tick(g_context, now);
    }
    return;
  }

  if (g_launcherNeedsInitialRender) {
    if (now < g_launcherInitialRenderAtMs) {
      return;
    }
    g_launcherNeedsInitialRender = false;
    g_lastLauncherRenderMs = now;
    drawLauncher();
    return;
  }

  tickLauncher(now);
}

#if defined(MIA_ESPIDF_APP_MAIN)
extern "C" void app_main(void) {
  initArduino();
  setup();
  while (true) {
    loop();
    vTaskDelay(1);
  }
}
#endif
