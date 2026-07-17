#include "diagnostic_i18n.h"

#include "mia_host_abi.h"

static const DiagnosticText DG_EN = {
    .title = "ESP32-S3 Diagnostic",
    .tft_ok = "TFT OK",
    .sd_ok = "SD OK",
    .ota_ok = "OTA OK",
    .btn = "BTN",
    .exit_hint = "Hold SEL+ST to exit",
    .boot_note = "BOOT and ST mirror GPIO0",
};

static const DiagnosticText DG_ZH = {
    .title = "ESP32-S3 诊断",
    .tft_ok = "屏正常",
    .sd_ok = "SD正常",
    .ota_ok = "OTA正常",
    .btn = "按键",
    .exit_hint = "按住 SEL+ST 退出",
    .boot_note = "BOOT 和 ST 映射 GPIO0",
};

const DiagnosticText *diagnostic_text(void) {
  return mia_host_language() == 1 ? &DG_ZH : &DG_EN;
}
