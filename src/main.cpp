#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

#include <cstring>

#include <esp_ota_ops.h>
#include <esp_rom_crc.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

#include "app.h"
#include "apps/about_app.h"
#include "apps/log_viewer_app.h"
#include "apps/serial_transfer_app.h"
#include "launcher_log.h"
#include "lcd_ili9342.h"
#include "lava_native_display.h"
#include "pins.h"
#include "rtc_clock.h"
#include "sd_app_loader.h"

SPIClass tftSpi(FSPI);
SPIClass sdSpi(HSPI);

AppContext g_context = {};
ButtonState g_allButtons[ALL_BUTTON_COUNT] = {};
static bool g_allButtonLast[ALL_BUTTON_COUNT] = {};
static uint8_t g_hc165State = 0xFF;
static uint32_t g_lastLauncherRenderMs = 0;
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

static const LauncherApp *const BUILTIN_APPS[] = {
    &serialTransferApp(),
    &logViewerApp(),
    &aboutApp(),
};
static constexpr uint8_t BUILTIN_APP_COUNT = sizeof(BUILTIN_APPS) / sizeof(BUILTIN_APPS[0]);

static constexpr uint8_t USB_DISK_MENU_INDEX = BUILTIN_APP_COUNT;
static constexpr uint8_t BOOTLOADER_MENU_INDEX = BUILTIN_APP_COUNT + 1;
static constexpr uint8_t TOTAL_LAUNCHER_ITEMS_FIXED = BUILTIN_APP_COUNT + 2;
static constexpr uint8_t SYSTEM_ITEM_COUNT = TOTAL_LAUNCHER_ITEMS_FIXED;

static const esp_partition_t *findOtaPartition() {
  return esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                  ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
}

// Manual OTA boot partition set — bypasses esp_image_verify inside
// esp_ota_set_boot_partition which was triggering TG1 WDT on ESP32-S3
static esp_err_t forceOtaBoot(const esp_partition_t *target) {
  const esp_partition_t *otap = findOtaPartition();
  if (!otap || !target) return ESP_ERR_NOT_FOUND;

  // Build entry matching esp_ota_select_entry_t from bootloader:
  //   ota_seq(4) + seq_label[20](20) + ota_state(4) + crc(4) = 32 bytes
  // CRC = esp_rom_crc32_le(UINT32_MAX, &ota_seq, 4)
  typedef struct __attribute__((packed)) {
    uint32_t ota_seq;
    uint8_t  seq_label[20];
    uint32_t ota_state;
    uint32_t crc;
  } OtaEntry;

  uint8_t slot = target->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_MIN;
  uint8_t ota_app_count = 2; // ota_0 and ota_1

  auto setEntry = [](OtaEntry *e, uint32_t seq, uint32_t state) {
    memset(e, 0, sizeof(OtaEntry));
    e->ota_seq = seq;
    e->ota_state = state;
    e->crc = esp_rom_crc32_le(UINT32_MAX, (uint8_t *)&e->ota_seq, 4);
  };

  // ota_slot = (seq - 1) % ota_app_count (2 slots: ota_0, ota_1)
  // For slot=1 (ota_1): need seq ≡ 2 mod 2 → 2, 4, 6...
  // Bootloader picks highest seq, so write seq=2 in sector 0, seq=4 in sector 1
  OtaEntry entries[2];
  setEntry(&entries[0], /*seq=*/slot + 1,               /*state=*/ESP_OTA_IMG_VALID);
  setEntry(&entries[1], /*seq=*/slot + 1 + ota_app_count, /*state=*/ESP_OTA_IMG_VALID); // higher seq = active

  esp_err_t err = esp_partition_erase_range(otap, 0, otap->size);
  if (err != ESP_OK) return err;

  err = esp_partition_write(otap, 0,      &entries[0], sizeof(OtaEntry));
  if (err != ESP_OK) return err;
  err = esp_partition_write(otap, 4096,   &entries[1], sizeof(OtaEntry));
  return err;
}

static void rebootToUsbDisk() {
  const esp_partition_t *ota1 = esp_partition_find_first(
      ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
  if (!ota1) {
    launcherTrace("[usb-disk] ota_1 partition NOT FOUND");
    delay(1000);
    return;
  }
  launcherTracef("[usb-disk] ota_1 at 0x%08x", (unsigned)ota1->address);

  esp_err_t err = forceOtaBoot(ota1);
  launcherTracef("[usb-disk] force_boot => %s", esp_err_to_name(err));
  if (err != ESP_OK) {
    delay(2000);
    return;
  }

  launcherTrace("[usb-disk] rebooting now");
  Serial.flush();
  delay(100);
  ESP.restart();
}

static void showBootloaderInstructions() {
  launcherTrace("[boot-loader] showing manual entry instructions");
  g_bootLoaderHintVisible = true;
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
};

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
  static const uint32_t kSdTrialHz[] = {10000000, 4000000, 1000000};
  g_context.sdReady = false;
  for (size_t i = 0; i < sizeof(kSdTrialHz) / sizeof(kSdTrialHz[0]); ++i) {
    const uint32_t hz = kSdTrialHz[i];
    launcherTracef("[sd] begin trial %u Hz", static_cast<unsigned>(hz));
    sdSpi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (SD.begin(SD_CS_PIN, sdSpi, hz)) {
      if (SD.cardType() == CARD_NONE) {
        launcherTracef("[sd] no card detected at %u Hz", static_cast<unsigned>(hz));
        SD.end();
        delay(50);
        continue;
      }
      g_context.sdReady = true;
      launcherTracef("[sd] mounted at %u Hz", static_cast<unsigned>(hz));
      break;
    }
    launcherTracef("[sd] mount failed at %u Hz", static_cast<unsigned>(hz));
    SD.end();
    delay(50);
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
      return "SD apps: ready";
    case SdAppLoaderStatus::SdUnavailable:
      return "SD apps: card unavailable";
    case SdAppLoaderStatus::NoAppsFound:
      return "SD apps: none";
    case SdAppLoaderStatus::ReadError:
      return "SD apps: read error";
    case SdAppLoaderStatus::RunError:
      return "SD apps: run error";
  }
  return "SD apps: unknown";
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
    return "System";
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
    snprintf(clockText, sizeof(clockText), "RTC unavailable");
  }
  const int16_t textX = LAVA_SCREEN_W - 6 - static_cast<int16_t>(strlen(clockText) * 6);
  lavaDrawText(textX, 6, clockText, LAVA_BLACK, LAVA_YELLOW);
}

static void drawLauncher() {
  if (!g_context.tftReady) {
    return;
  }

  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(6, 6, "MiaOS Launcher", LAVA_BLACK, LAVA_YELLOW);
  drawLauncherClock();

  int16_t tabX = 8;
  const uint8_t tabCount = launcherTabCount();
  for (uint8_t tab = 0; tab < tabCount; ++tab) {
    const char *tabName = launcherTabName(tab);
    if (tabName == nullptr) {
      continue;
    }
    const uint8_t bg = tab == g_selectedTab ? LAVA_BLUE : LAVA_BLACK;
    const uint8_t fg = tab == g_selectedTab ? LAVA_YELLOW : LAVA_GRAY;
    const int16_t tabWidth = static_cast<int16_t>(strlen(tabName) * 6 + 12);
    lavaFillRect(tabX, 28, tabWidth, 16, bg);
    lavaDrawText(tabX + 6, 33, tabName, fg, bg);
    tabX += tabWidth + 4;
  }

  lavaDrawText(8, 48, "Apps", LAVA_CYAN, LAVA_BLACK);
  const uint8_t maxVisibleApps = 8;
  uint8_t firstApp = 0;
  const uint8_t itemCount = totalLauncherItems();
  if (itemCount > maxVisibleApps && g_selectedApp >= maxVisibleApps) {
    firstApp = g_selectedApp - maxVisibleApps + 1;
  }
  const uint8_t visibleEnd = min<uint8_t>(itemCount, firstApp + maxVisibleApps);
  for (uint8_t i = firstApp; i < visibleEnd; ++i) {
    const int16_t y = 64 + (i - firstApp) * 18;
    const bool selected = i == g_selectedApp;
    const bool sdApp = g_selectedTab != SYSTEM_TAB_INDEX;
    const bool isUsbDisk = g_selectedTab == SYSTEM_TAB_INDEX && i == USB_DISK_MENU_INDEX;
    const bool isBootloader = g_selectedTab == SYSTEM_TAB_INDEX && i == BOOTLOADER_MENU_INDEX;
    const char *name;
    if (isUsbDisk) {
      name = "USB Disk";
    } else if (isBootloader) {
      name = "Boot Loader";
    } else if (sdApp) {
      const SdAppManifestSummary *app = selectedSdApp(i);
      name = app == nullptr ? "<missing>" : app->name;
    } else {
      name = BUILTIN_APPS[i]->name;
    }
    const uint8_t itemBg = selected ? LAVA_BLUE : LAVA_BLACK;
    const uint8_t itemText = selected ? LAVA_BLACK : LAVA_WHITE;
    const uint8_t itemTag = selected ? LAVA_BLACK : LAVA_GREEN;
    const uint8_t itemCursor = selected ? LAVA_YELLOW : LAVA_YELLOW;
    lavaFillRect(6, y - 3, 308, 15, itemBg);
    lavaDrawText(12, y, selected ? ">" : " ", itemCursor, itemBg);
    lavaDrawText(28, y, sdApp ? "[sd]" : ((isUsbDisk || isBootloader) ? "[sys]" : ""),
                 itemTag, itemBg);
    lavaDrawText((sdApp || isUsbDisk || isBootloader) ? 52 : 28, y, name, itemText,
                 itemBg);
  }

  lavaDrawText(8, 206, sdScanStatusText(), g_context.sdReady ? LAVA_GREEN : LAVA_RED,
                LAVA_BLACK);
  if (g_lastSdRun.status != SdAppLoaderStatus::Ok) {
    lavaDrawText(8, 214, sdAppLoaderStatusText(g_lastSdRun.status), LAVA_RED,
                  LAVA_BLACK);
  }
  lavaDrawText(8, 224, "A:Open UP/DN:Move LEFT/RIGHT:Tab", LAVA_GRAY, LAVA_BLACK);

  if (g_bootLoaderHintVisible) {
    lavaFillRect(24, 44, 272, 156, LAVA_DARK_BLUE);
    lavaFillRect(24, 44, 272, 20, LAVA_YELLOW);
    lavaDrawText(30, 50, "Boot Loader", LAVA_BLACK, LAVA_YELLOW);
    lavaDrawText(40, 82, "1. Hold ST", LAVA_WHITE, LAVA_DARK_BLUE);
    lavaDrawText(40, 100, "2. Press RESET", LAVA_WHITE, LAVA_DARK_BLUE);
    lavaDrawText(40, 118, "3. Release RESET into", LAVA_WHITE, LAVA_DARK_BLUE);
    lavaDrawText(58, 136, "download mode", LAVA_CYAN, LAVA_DARK_BLUE);
    lavaDrawText(40, 162, "RESET alone returns to", LAVA_GRAY, LAVA_DARK_BLUE);
    lavaDrawText(58, 180, "normal boot", LAVA_GRAY, LAVA_DARK_BLUE);
    lavaDrawText(86, 198, "A/B:Back", LAVA_YELLOW, LAVA_DARK_BLUE);
  }
  lavaPresent();
}

static void enterSelectedApp() {
  if (g_selectedApp >= totalLauncherItems()) {
    return;
  }

  if (g_selectedTab == SYSTEM_TAB_INDEX && g_selectedApp == USB_DISK_MENU_INDEX) {
    rebootToUsbDisk();
    return;
  }

  if (g_selectedTab == SYSTEM_TAB_INDEX && g_selectedApp == BOOTLOADER_MENU_INDEX) {
    showBootloaderInstructions();
    drawLauncher();
    return;
  }

  if (g_selectedTab != SYSTEM_TAB_INDEX) {
    const SdAppManifestSummary *app = selectedSdApp(g_selectedApp);
    if (app == nullptr) {
      return;
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

static void writeStartupLog() {
  launcherLogAppendf("startup chip=%s rev=%d cpu_mhz=%u", ESP.getChipModel(),
                     ESP.getChipRevision(), ESP.getCpuFreqMHz());
  launcherLogAppendf("startup flash=%u heap=%u", ESP.getFlashChipSize(), ESP.getFreeHeap());
  launcherLogAppendf("startup mac=%s", macAddress().c_str());
  launcherLogAppendf("startup sd_ready=%d tft_ready=%d", g_context.sdReady ? 1 : 0,
                     g_context.tftReady ? 1 : 0);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

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
  launcherTrace("[setup] initDisplay - BEFORE");
  initDisplay();
  launcherTrace("[setup] initDisplay - AFTER");

  launcherTrace("[setup] initSdCard");
  initSdCard();
  launcherLogBeginSession(g_context.sdReady);
  writeStartupLog();
  clampLauncherSelection();
  launcherTrace("[setup] drawLauncher - BEFORE");
  drawLauncher();
  launcherTrace("[setup] drawLauncher - AFTER");
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
