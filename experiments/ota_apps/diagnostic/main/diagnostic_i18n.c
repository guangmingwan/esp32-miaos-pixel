#include "diagnostic_i18n.h"

#include "mia_host_abi.h"

static const DiagnosticText DG_EN = {
    .title = "ESP32-S3 Diagnostic",
    .tft_ok = "TFT OK",
    .sd_ok = "SD OK",
    .ota_ok = "OTA OK",
    .rtc_ok = "RTC OK",
    .btn = "BTN",
    .exit_hint = "Yellow:down Green:seen  SEL+ST:Exit",
    .boot_note = "BOOT and ST mirror GPIO0",
};

static const DiagnosticText DG_ZH = {
    .title = "ESP32-S3 诊断",
    .tft_ok = "屏正常",
    .sd_ok = "SD正常",
    .ota_ok = "OTA正常",
    .rtc_ok = "RTC正常",
    .btn = "按键",
    .exit_hint = "黄色:按下 绿色:已检测 SEL+ST:退出",
    .boot_note = "BOOT 和 ST 映射 GPIO0",
};

const DiagnosticText *diagnostic_text(void) {
  return mia_host_language() == 1 ? &DG_ZH : &DG_EN;
}
