#include "apps/about_app.h"

#include <Arduino.h>
#include <esp_chip_info.h>
#include <esp_heap_caps.h>
#include <sdkconfig.h>

#include "lava_native_display.h"
#include "mia_i18n.h"
#include "pins.h"

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

static constexpr uint8_t ABOUT_PAGE_COUNT = 6;
static uint8_t g_aboutPage = 0;

static void drawMiaCatIcon(int16_t x, int16_t y) {
  static const char *const kCatRows[] = {
      "##....##",
      "###..###",
      "########",
      "##c##c##",
      "########",
      "#r####r#",
      "##.##.##",
      ".######.",
      "..#..#..",
  };

  constexpr int16_t block = 2;
  for (uint8_t row = 0; row < sizeof(kCatRows) / sizeof(kCatRows[0]); ++row) {
    for (uint8_t col = 0; kCatRows[row][col] != '\0'; ++col) {
      uint8_t color = LAVA_YELLOW;
      switch (kCatRows[row][col]) {
        case '#':
          color = LAVA_BLACK;
          break;
        case 'c':
          color = LAVA_CYAN;
          break;
        case 'r':
          color = LAVA_RED;
          break;
        default:
          continue;
      }
      lavaFillRect(x + col * block, y + row * block, block, block, color);
    }
  }
}

static const char *flashModeName(FlashMode_t mode) {
  switch (mode) {
  case FM_QIO:
    return "QIO";
  case FM_QOUT:
    return "QOUT";
  case FM_DIO:
    return "DIO";
  case FM_DOUT:
    return "DOUT";
  case FM_FAST_READ:
    return "FAST";
  case FM_SLOW_READ:
    return "SLOW";
  case FM_UNKNOWN:
  default:
    return "UNKNOWN";
  }
}

static const char *featureFlagName(uint32_t features, uint32_t flag) {
  return (features & flag) != 0 ? "YES" : "NO";
}

static void drawHeader(const char *title) {
  char line[24];
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  snprintf(line, sizeof(line), "%s %u/%u", title, g_aboutPage + 1, ABOUT_PAGE_COUNT);
  lavaDrawText(4, 6, line, LAVA_BLACK, LAVA_YELLOW);
  drawMiaCatIcon(LAVA_SCREEN_W - 22, 1);
}

static void drawFooter() {
  lavaDrawText(4, 222, miaTr("UP/DN page  SEL+ST Exit"), LAVA_GRAY, LAVA_BLACK);
}

static void drawChipPage() {
  char line[40];
  esp_chip_info_t chipInfo;
  esp_chip_info(&chipInfo);
  drawHeader(miaTr("Chip"));
  lavaDrawText(8, 34, miaTr("MiaOS 0.1"), LAVA_CYAN, LAVA_BLACK);
  snprintf(line, sizeof(line), "Model %s", ESP.getChipModel());
  lavaDrawText(8, 54, line, LAVA_WHITE, LAVA_BLACK);
  snprintf(line, sizeof(line), "Rev %u  Cores %u", ESP.getChipRevision(), ESP.getChipCores());
  lavaDrawText(8, 74, line, LAVA_WHITE, LAVA_BLACK);
  snprintf(line, sizeof(line), "CPU %luMHz", static_cast<unsigned long>(ESP.getCpuFreqMHz()));
  lavaDrawText(8, 94, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), miaTr("WiFi b/g/n %s"),
           featureFlagName(chipInfo.features, CHIP_FEATURE_WIFI_BGN));
  lavaDrawText(8, 114, line, LAVA_YELLOW, LAVA_BLACK);
  snprintf(line, sizeof(line), miaTr("BT %s BLE %s"),
           featureFlagName(chipInfo.features, CHIP_FEATURE_BT),
           featureFlagName(chipInfo.features, CHIP_FEATURE_BLE));
  lavaDrawText(8, 134, line, LAVA_YELLOW, LAVA_BLACK);
}

static void drawMemoryPage() {
  char line[40];
  drawHeader(miaTr("Memory"));
  snprintf(line, sizeof(line), miaTr("SRAM total %luK"),
           static_cast<unsigned long>(ESP.getHeapSize() / 1024));
  lavaDrawText(8, 38, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), miaTr("SRAM free  %luK"),
           static_cast<unsigned long>(ESP.getFreeHeap() / 1024));
  lavaDrawText(8, 58, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), miaTr("SRAM min   %luK"),
           static_cast<unsigned long>(ESP.getMinFreeHeap() / 1024));
  lavaDrawText(8, 78, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), miaTr("Max block  %luK"),
           static_cast<unsigned long>(ESP.getMaxAllocHeap() / 1024));
  lavaDrawText(8, 98, line, LAVA_GREEN, LAVA_BLACK);
  lavaDrawText(8, 132, miaTr("SRAM is volatile RAM"), LAVA_GRAY, LAVA_BLACK);
}

static void drawPsramPage() {
  char line[40];
  drawHeader("PSRAM");
#if defined(CONFIG_SPIRAM_BOOT_INIT) && CONFIG_SPIRAM_BOOT_INIT
  const bool psramConfigured = true;
#else
  const bool psramConfigured = false;
#endif
  const uint32_t psramSize = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  if (!psramConfigured) {
    lavaDrawText(8, 49, miaTr("PSRAM config disabled"), LAVA_RED, LAVA_BLACK);
    lavaDrawText(8, 71, miaTr("Build has no external"), LAVA_GRAY, LAVA_BLACK);
    lavaDrawText(8, 89, miaTr("RAM support enabled"), LAVA_GRAY, LAVA_BLACK);
    return;
  }
  if (psramSize == 0) {
    lavaDrawText(8, 49, miaTr("PSRAM init failed"), LAVA_RED, LAVA_BLACK);
    lavaDrawText(8, 71, miaTr("Configured in build"), LAVA_GRAY, LAVA_BLACK);
    lavaDrawText(8, 89, miaTr("but absent at runtime"), LAVA_GRAY, LAVA_BLACK);
    return;
  }
  lavaDrawText(8, 25, miaTr("Configured YES"), LAVA_CYAN, LAVA_BLACK);
  snprintf(line, sizeof(line), "Total %luK", static_cast<unsigned long>(psramSize / 1024));
  lavaDrawText(8, 43, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), "Free  %luK",
           static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
  lavaDrawText(8, 63, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), "Min   %luK",
           static_cast<unsigned long>(heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM) / 1024));
  lavaDrawText(8, 83, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), "Max block %luK",
           static_cast<unsigned long>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) / 1024));
  lavaDrawText(8, 103, line, LAVA_GREEN, LAVA_BLACK);
  lavaDrawText(8, 137, miaTr("External volatile RAM"), LAVA_GRAY, LAVA_BLACK);
}

static void drawFlashPage() {
  char line[40];
  drawHeader(miaTr("NOR Flash"));
  snprintf(line, sizeof(line), "Size %luMB",
           static_cast<unsigned long>(ESP.getFlashChipSize() / 1024 / 1024));
  lavaDrawText(8, 38, line, LAVA_YELLOW, LAVA_BLACK);
  snprintf(line, sizeof(line), "Speed %luMHz",
           static_cast<unsigned long>(ESP.getFlashChipSpeed() / 1000000));
  lavaDrawText(8, 58, line, LAVA_YELLOW, LAVA_BLACK);
  snprintf(line, sizeof(line), "Mode %s", flashModeName(ESP.getFlashChipMode()));
  lavaDrawText(8, 78, line, LAVA_YELLOW, LAVA_BLACK);
  lavaDrawText(8, 104, miaTr("Sketch info unavailable"), LAVA_GRAY, LAVA_BLACK);
  lavaDrawText(8, 124, miaTr("OTA space unavailable"), LAVA_GRAY, LAVA_BLACK);
  lavaDrawText(8, 146, miaTr("Non-volatile storage"), LAVA_GRAY, LAVA_BLACK);
}

static void drawBuildPage() {
  char line[40];
  drawHeader(miaTr("System"));
  lavaDrawText(8, 34, miaTr("Author wanguangmign"), LAVA_WHITE, LAVA_BLACK);
  lavaDrawText(8, 54, miaTr("Contributor WaitForWind"), LAVA_WHITE, LAVA_BLACK);
  snprintf(line, sizeof(line), "SDK %.21s", ESP.getSdkVersion());
  lavaDrawText(8, 84, line, LAVA_CYAN, LAVA_BLACK);
  snprintf(line, sizeof(line), "MAC %04lX%08lX", static_cast<unsigned long>(ESP.getEfuseMac() >> 32),
           static_cast<unsigned long>(ESP.getEfuseMac()));
  lavaDrawText(8, 104, line, LAVA_CYAN, LAVA_BLACK);
  lavaDrawText(8, 138, miaTr("ESP32-D0WD board"), LAVA_GRAY, LAVA_BLACK);
}

static void drawAudioPage() {
  char line[40];
  drawHeader(miaTr("Audio"));
  lavaDrawText(8, 34, miaTr("NS4168 I2S amp"), LAVA_CYAN, LAVA_BLACK);
  snprintf(line, sizeof(line), "WS GPIO %d", I2S_WS_PIN);
  lavaDrawText(8, 54, line, LAVA_WHITE, LAVA_BLACK);
  snprintf(line, sizeof(line), "BCK GPIO %d", I2S_BCK_PIN);
  lavaDrawText(8, 74, line, LAVA_WHITE, LAVA_BLACK);
  snprintf(line, sizeof(line), "DATA GPIO %d", I2S_DATA_PIN);
  lavaDrawText(8, 94, line, LAVA_WHITE, LAVA_BLACK);
  snprintf(line, sizeof(line), "AMP EN GPIO %d", AMP_CTRL_PIN);
  lavaDrawText(8, 114, line, LAVA_GREEN, LAVA_BLACK);
  lavaDrawText(8, 148, miaTr("Single speaker output"), LAVA_GRAY, LAVA_BLACK);
  lavaDrawText(8, 168, miaTr("CTRL pin is active high"), LAVA_GRAY, LAVA_BLACK);
}

static void drawAbout(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  lavaClear(LAVA_BLACK);
  switch (g_aboutPage) {
  case 0:
    drawChipPage();
    break;
  case 1:
    drawMemoryPage();
    break;
  case 2:
    drawPsramPage();
    break;
  case 3:
    drawFlashPage();
    break;
  case 4:
    drawAudioPage();
    break;
  case 5:
  default:
    drawBuildPage();
    break;
  }
  drawFooter();
  lavaPresent();
}

static void aboutBegin(AppContext &context) {
  g_aboutPage = 0;
  drawAbout(context);
}

static void aboutTick(AppContext &context, uint32_t nowMs) {
  (void)nowMs;
  if (context.buttons[2].pressed && g_aboutPage > 0) {
    --g_aboutPage;
    drawAbout(context);
  }
  if (context.buttons[3].pressed && g_aboutPage + 1 < ABOUT_PAGE_COUNT) {
    ++g_aboutPage;
    drawAbout(context);
  }
}

static void aboutEnd(AppContext &context) {
  (void)context;
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

const LauncherApp &aboutApp() {
  static const LauncherApp app = {"About", aboutBegin, aboutTick, aboutEnd};
  return app;
}
