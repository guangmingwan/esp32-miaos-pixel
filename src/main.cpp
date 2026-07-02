#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
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
#include "apps/calculator_app.h"
#include "apps/diagnostic_app.h"
#include "apps/flashlight_app.h"
#include "apps/ftp_server_app.h"
#include "apps/minesweeper_app.h"
#include "apps/rtc_app.h"
#include "apps/screen_test_app.h"
#include "apps/sd_browser_app.h"
#include "apps/timer_app.h"
#include "apps/wifi_files_app.h"
#include "apps/wifi_scan_app.h"
#include "lcd_ili9342.h"
#include "lava_native_display.h"
#include "pins.h"
#include "sd_app_loader.h"

SPIClass tftSpi(FSPI);
SPIClass sdSpi(HSPI);

static AppContext g_context = {};
ButtonState g_allButtons[ALL_BUTTON_COUNT] = {};
static bool g_allButtonLast[ALL_BUTTON_COUNT] = {};
static uint8_t g_hc165State = 0xFF;
static uint32_t g_lastLauncherRenderMs = 0;
static uint8_t g_selectedApp = 0;
static const LauncherApp *g_activeApp = nullptr;
static SdAppLoaderResult g_sdScan = {SdAppLoaderStatus::SdUnavailable, 0};
static SdAppManifestSummary g_sdApps[4] = {};
static SdAppLoaderResult g_lastSdRun = {SdAppLoaderStatus::Ok, 0};
static constexpr uint8_t BUTTON_INDEX_START = 1;
static constexpr uint8_t BUTTON_INDEX_SELECT = 5;
static constexpr uint8_t BUTTON_INDEX_A = 6;
static constexpr uint8_t BUTTON_INDEX_B = 7;
static constexpr uint8_t BUTTON_INDEX_UP = 10;
static constexpr uint8_t BUTTON_INDEX_DOWN = 11;
static constexpr uint8_t BUTTON_INDEX_L = 3;
static constexpr uint8_t BUTTON_INDEX_R = 4;

static const LauncherApp *const BUILTIN_APPS[] = {
    &diagnosticApp(),
    &screenTestApp(),
    &rtcApp(),
    &calculatorApp(),
    &flashlightApp(),
    &timerApp(),
    &sdBrowserApp(),
    &wifiFilesApp(),
    &ftpServerApp(),
    &wifiScanApp(),
    &minesweeperApp(),
    &aboutApp(),
};
static constexpr uint8_t BUILTIN_APP_COUNT = sizeof(BUILTIN_APPS) / sizeof(BUILTIN_APPS[0]);

static constexpr uint8_t USB_DISK_MENU_INDEX = BUILTIN_APP_COUNT;
static constexpr uint8_t BOOTLOADER_MENU_INDEX = BUILTIN_APP_COUNT + 1;
static constexpr uint8_t TOTAL_LAUNCHER_ITEMS_FIXED = BUILTIN_APP_COUNT + 2;

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
    Serial.println("[usb-disk] ota_1 partition NOT FOUND");
    delay(1000);
    return;
  }
  Serial.printf("[usb-disk] ota_1 at 0x%08x\n", (unsigned)ota1->address);

  esp_err_t err = forceOtaBoot(ota1);
  Serial.printf("[usb-disk] force_boot => %s\n", esp_err_to_name(err));
  if (err != ESP_OK) {
    delay(2000);
    return;
  }

  Serial.println("[usb-disk] rebooting now");
  Serial.flush();
  delay(100);
  ESP.restart();
}

static void rebootToBootloader() {
  Serial.println("[boot-loader] entering ROM download mode");
  if (g_context.tftReady) {
    constexpr uint8_t kBlack = 0;
    constexpr uint8_t kWhite = 1;
    constexpr uint8_t kBlue = 2;
    constexpr uint8_t kYellow = 5;
    constexpr uint8_t kCyan = 6;
    constexpr uint8_t kGray = 7;
    constexpr uint8_t kDarkBlue = 8;
    lavaClear(kDarkBlue);
    lavaFillRect(0, 0, LAVA_SCREEN_W, 28, kYellow);
    lavaDrawText(8, 10, "Boot Loader", kBlack, kYellow);
    lavaFillRect(18, 58, 284, 96, kBlue);
    lavaDrawText(34, 78, "Entering download mode", kWhite, kBlue);
    lavaDrawText(34, 98, "USB will reconnect soon", kCyan, kBlue);
    lavaDrawText(34, 126, "Use PlatformIO upload now", kYellow, kBlue);
    lavaDrawText(18, 214, "If stuck, press RESET or replug USB", kGray, kDarkBlue);
    lavaPresent();
  }
  Serial.flush();
  delay(1500);
  REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
  ESP.restart();
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
    Serial.printf("[sd] begin trial %u Hz\n", static_cast<unsigned>(hz));
    sdSpi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (SD.begin(SD_CS_PIN, sdSpi, hz)) {
      if (SD.cardType() == CARD_NONE) {
        Serial.printf("[sd] no card detected at %u Hz\n", static_cast<unsigned>(hz));
        SD.end();
        delay(50);
        continue;
      }
      g_context.sdReady = true;
      Serial.printf("[sd] mounted at %u Hz\n", static_cast<unsigned>(hz));
      break;
    }
    Serial.printf("[sd] mount failed at %u Hz\n", static_cast<unsigned>(hz));
    SD.end();
    delay(50);
  }
#else
  g_context.sdReady = false;
#endif
  g_sdScan = scanSdApps(g_sdApps, sizeof(g_sdApps) / sizeof(g_sdApps[0]),
                        g_context.sdReady);
}

static void updateAllButtons() {
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

static uint8_t totalLauncherItems() {
  return TOTAL_LAUNCHER_ITEMS_FIXED + min<uint8_t>(g_sdScan.appCount,
                                           sizeof(g_sdApps) / sizeof(g_sdApps[0]));
}

static void drawLauncher() {
  if (!g_context.tftReady) {
    return;
  }

  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(6, 6, "MiaOS Launcher", LAVA_BLACK, LAVA_YELLOW);
  lavaDrawText(266, 6, "QVGA", LAVA_BLUE, LAVA_YELLOW);

  lavaDrawText(8, 30, "Apps", LAVA_CYAN, LAVA_BLACK);
  const uint8_t maxVisibleApps = 9;
  uint8_t firstApp = 0;
  const uint8_t itemCount = totalLauncherItems();
  if (itemCount > maxVisibleApps && g_selectedApp >= maxVisibleApps) {
    firstApp = g_selectedApp - maxVisibleApps + 1;
  }
  const uint8_t visibleEnd = min<uint8_t>(itemCount, firstApp + maxVisibleApps);
  for (uint8_t i = firstApp; i < visibleEnd; ++i) {
    const int16_t y = 46 + (i - firstApp) * 18;
    const bool selected = i == g_selectedApp;
    const bool sdApp = i >= TOTAL_LAUNCHER_ITEMS_FIXED;
    const bool isUsbDisk = i == USB_DISK_MENU_INDEX;
    const bool isBootloader = i == BOOTLOADER_MENU_INDEX;
    const char *name;
    if (isUsbDisk) {
      name = "USB Disk";
    } else if (isBootloader) {
      name = "Boot Loader";
    } else if (sdApp) {
      name = g_sdApps[i - TOTAL_LAUNCHER_ITEMS_FIXED].name;
    } else {
      name = BUILTIN_APPS[i]->name;
    }
    lavaFillRect(6, y - 3, 308, 15, selected ? LAVA_BLUE : LAVA_BLACK);
    lavaDrawText(12, y, selected ? ">" : " ", LAVA_YELLOW,
                 selected ? LAVA_BLUE : LAVA_BLACK);
    lavaDrawText(28, y, sdApp ? "[sd]" : ((isUsbDisk || isBootloader) ? ">>" : ""), LAVA_GREEN,
                 selected ? LAVA_BLUE : LAVA_BLACK);
    lavaDrawText(sdApp ? 52 : ((isUsbDisk || isBootloader) ? 52 : 28), y, name, LAVA_WHITE,
                 selected ? LAVA_BLUE : LAVA_BLACK);
  }

  lavaDrawText(8, 206, sdScanStatusText(), g_context.sdReady ? LAVA_GREEN : LAVA_RED,
                LAVA_BLACK);
  if (g_lastSdRun.status != SdAppLoaderStatus::Ok) {
    lavaDrawText(8, 218, sdAppLoaderStatusText(g_lastSdRun.status), LAVA_RED,
                  LAVA_BLACK);
  }
  lavaDrawText(8, 222, "A:Open  UP/DN:Move", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

static void enterSelectedApp() {
  if (g_selectedApp >= totalLauncherItems()) {
    return;
  }

  if (g_selectedApp == USB_DISK_MENU_INDEX) {
    rebootToUsbDisk();
    return;
  }

  if (g_selectedApp == BOOTLOADER_MENU_INDEX) {
    rebootToBootloader();
    return;
  }

  if (g_selectedApp >= TOTAL_LAUNCHER_ITEMS_FIXED) {
    const SdAppManifestSummary &app =
        g_sdApps[g_selectedApp - TOTAL_LAUNCHER_ITEMS_FIXED];
    g_lastSdRun = runSdAppByPath(app.path, g_context.sdReady);
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
  Serial.println();
  Serial.println("ESP32-S3 Retro-Pixel launcher (ILI9342 320x240)");
  Serial.printf("Chip: %s rev%d, CPU %uMHz\n", ESP.getChipModel(),
                ESP.getChipRevision(), ESP.getCpuFreqMHz());
  Serial.printf("Flash: %u bytes, heap: %u bytes\n", ESP.getFlashChipSize(),
                ESP.getFreeHeap());
  Serial.printf("MAC: %s\n", macAddress().c_str());
  Serial.printf("TFT: ILI9342 320x240 SCK=%d MOSI=%d CS=%d DC=%d RST=%d BL=%d\n",
                TFT_SCK_PIN, TFT_MOSI_PIN, TFT_CS_PIN, TFT_DC_PIN,
                TFT_RST_PIN, TFT_BL_PIN);
  Serial.printf("SD: SCK=%d MOSI=%d MISO=%d CS=%d\n", SD_SCK_PIN, SD_MOSI_PIN,
                SD_MISO_PIN, SD_CS_PIN);
  Serial.printf("Keys: BOOT=%d M=%d L=%d R=%d SEL=%d ST=%d\n", KEY_BOOT_PIN,
                KEY_M_PIN, KEY_L_PIN, KEY_R_PIN, KEY_SELECT_PIN, KEY_START_PIN);
  Serial.printf("HC165: PL=%d CLK=%d DAT=%d\n", HC165_PL_PIN, HC165_CLK_PIN,
                HC165_DAT_PIN);
  Serial.printf("Audio: I2S WS=%d BCK=%d DAT=%d AMP=%d\n", I2S_WS_PIN,
                I2S_BCK_PIN, I2S_DATA_PIN, AMP_CTRL_PIN);
  Serial.printf("Misc: VBAT=%d BEEP=%d I2C SCL=%d SDA=%d\n", VBAT_ADC_PIN,
                BEEP_PIN, I2C_SCL_PIN, I2C_SDA_PIN);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("[setup] configurePins");
  configurePins();
  Serial.println("[setup] Wire.begin");
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("[setup] updateAllButtons");
  updateAllButtons();

  Serial.println("[setup] printStartupInfo");
  printStartupInfo();
  Serial.println("[setup] initDisplay - BEFORE");
  initDisplay();
  Serial.println("[setup] initDisplay - AFTER");

  Serial.println("[setup] initSdCard");
  initSdCard();
  Serial.println("[setup] drawLauncher - BEFORE");
  drawLauncher();
  Serial.println("[setup] drawLauncher - AFTER");
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
