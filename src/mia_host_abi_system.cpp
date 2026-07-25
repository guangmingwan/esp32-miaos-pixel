#include "mia_host_abi.h"

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

#include "app.h"
#include "pins.h"

extern AppContext g_context;

uint8_t mia_host_get_system_info(MiaHostSystemInfo *info) {
  if (info == nullptr) {
    return 0;
  }

  memset(info, 0, sizeof(MiaHostSystemInfo));
  strncpy(info->chip_model, ESP.getChipModel(), sizeof(info->chip_model) - 1);
  info->chip_revision = static_cast<uint8_t>(ESP.getChipRevision());
  info->cpu_mhz = static_cast<uint16_t>(ESP.getCpuFreqMHz());
  info->free_heap_kb = ESP.getFreeHeap() / 1024;
  info->flash_mb = ESP.getFlashChipSize() / 1024 / 1024;
  info->tft_ready = g_context.tftReady ? 1 : 0;
  info->sd_ready = g_context.sdReady ? 1 : 0;
  return 1;
}

uint8_t mia_host_read_battery(MiaHostBatteryInfo *info) {
  if (info == nullptr) {
    return 0;
  }

  const int32_t raw = analogRead(VBAT_ADC_PIN);
  const float volts = raw * 3.3f / 4095.0f * VBAT_DIVIDER * VBAT_ADC_CALIBRATION;
  info->raw = raw;
  info->millivolts = static_cast<uint32_t>(volts * 1000.0f);
  return 1;
}
