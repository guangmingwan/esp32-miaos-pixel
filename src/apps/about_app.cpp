#include "apps/about_app.h"

#include <Arduino.h>
#include <esp_chip_info.h>

#include "lava_native_display.h"

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

static constexpr uint8_t ABOUT_PAGE_COUNT = 5;
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
  lavaDrawText(4, 222, "UP/DN page  SEL+ST Exit", LAVA_GRAY, LAVA_BLACK);
}

static void drawChipPage() {
  char line[40];
  esp_chip_info_t chipInfo;
  esp_chip_info(&chipInfo);
  drawHeader("Chip");
  lavaDrawText(8, 34, "MiaOS 0.1", LAVA_CYAN, LAVA_BLACK);
  snprintf(line, sizeof(line), "Model %s", ESP.getChipModel());
  lavaDrawText(8, 54, line, LAVA_WHITE, LAVA_BLACK);
  snprintf(line, sizeof(line), "Rev %u  Cores %u", ESP.getChipRevision(), ESP.getChipCores());
  lavaDrawText(8, 74, line, LAVA_WHITE, LAVA_BLACK);
  snprintf(line, sizeof(line), "CPU %luMHz", static_cast<unsigned long>(ESP.getCpuFreqMHz()));
  lavaDrawText(8, 94, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), "WiFi b/g/n %s",
           featureFlagName(chipInfo.features, CHIP_FEATURE_WIFI_BGN));
  lavaDrawText(8, 114, line, LAVA_YELLOW, LAVA_BLACK);
  snprintf(line, sizeof(line), "BT %s BLE %s",
           featureFlagName(chipInfo.features, CHIP_FEATURE_BT),
           featureFlagName(chipInfo.features, CHIP_FEATURE_BLE));
  lavaDrawText(8, 134, line, LAVA_YELLOW, LAVA_BLACK);
}

static void drawMemoryPage() {
  char line[40];
  drawHeader("Memory");
  snprintf(line, sizeof(line), "SRAM total %luK",
           static_cast<unsigned long>(ESP.getHeapSize() / 1024));
  lavaDrawText(8, 38, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), "SRAM free  %luK",
           static_cast<unsigned long>(ESP.getFreeHeap() / 1024));
  lavaDrawText(8, 58, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), "SRAM min   %luK",
           static_cast<unsigned long>(ESP.getMinFreeHeap() / 1024));
  lavaDrawText(8, 78, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), "Max block  %luK",
           static_cast<unsigned long>(ESP.getMaxAllocHeap() / 1024));
  lavaDrawText(8, 98, line, LAVA_GREEN, LAVA_BLACK);
  lavaDrawText(8, 132, "SRAM is volatile RAM", LAVA_GRAY, LAVA_BLACK);
}

static void drawPsramPage() {
  char line[40];
  drawHeader("PSRAM");
  const uint32_t psramSize = ESP.getPsramSize();
  if (psramSize == 0) {
    lavaDrawText(8, 44, "PSRAM not found", LAVA_RED, LAVA_BLACK);
    lavaDrawText(8, 66, "External RAM chip", LAVA_GRAY, LAVA_BLACK);
    lavaDrawText(8, 84, "not mounted", LAVA_GRAY, LAVA_BLACK);
    return;
  }
  snprintf(line, sizeof(line), "Total %luK", static_cast<unsigned long>(psramSize / 1024));
  lavaDrawText(8, 38, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), "Free  %luK",
           static_cast<unsigned long>(ESP.getFreePsram() / 1024));
  lavaDrawText(8, 58, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), "Min   %luK",
           static_cast<unsigned long>(ESP.getMinFreePsram() / 1024));
  lavaDrawText(8, 78, line, LAVA_GREEN, LAVA_BLACK);
  snprintf(line, sizeof(line), "Max block %luK",
           static_cast<unsigned long>(ESP.getMaxAllocPsram() / 1024));
  lavaDrawText(8, 98, line, LAVA_GREEN, LAVA_BLACK);
  lavaDrawText(8, 132, "External volatile RAM", LAVA_GRAY, LAVA_BLACK);
}

static void drawFlashPage() {
  char line[40];
  drawHeader("NOR Flash");
  snprintf(line, sizeof(line), "Size %luMB",
           static_cast<unsigned long>(ESP.getFlashChipSize() / 1024 / 1024));
  lavaDrawText(8, 38, line, LAVA_YELLOW, LAVA_BLACK);
  snprintf(line, sizeof(line), "Speed %luMHz",
           static_cast<unsigned long>(ESP.getFlashChipSpeed() / 1000000));
  lavaDrawText(8, 58, line, LAVA_YELLOW, LAVA_BLACK);
  snprintf(line, sizeof(line), "Mode %s", flashModeName(ESP.getFlashChipMode()));
  lavaDrawText(8, 78, line, LAVA_YELLOW, LAVA_BLACK);
  lavaDrawText(8, 104, "Sketch info unavailable", LAVA_GRAY, LAVA_BLACK);
  lavaDrawText(8, 124, "OTA space unavailable", LAVA_GRAY, LAVA_BLACK);
  lavaDrawText(8, 146, "Non-volatile storage", LAVA_GRAY, LAVA_BLACK);
}

static void drawBuildPage() {
  char line[40];
  drawHeader("System");
  lavaDrawText(8, 34, "Author wanguangmign", LAVA_WHITE, LAVA_BLACK);
  lavaDrawText(8, 54, "Contributor WaitForWind", LAVA_WHITE, LAVA_BLACK);
  snprintf(line, sizeof(line), "SDK %.21s", ESP.getSdkVersion());
  lavaDrawText(8, 84, line, LAVA_CYAN, LAVA_BLACK);
  snprintf(line, sizeof(line), "MAC %04lX%08lX", static_cast<unsigned long>(ESP.getEfuseMac() >> 32),
           static_cast<unsigned long>(ESP.getEfuseMac()));
  lavaDrawText(8, 104, line, LAVA_CYAN, LAVA_BLACK);
  lavaDrawText(8, 138, "ESP32-D0WD board", LAVA_GRAY, LAVA_BLACK);
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
