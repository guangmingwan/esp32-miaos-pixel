#include "system_settings.h"

#include <Arduino.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "launcher_log.h"
#include "pins.h"

namespace {

constexpr char NVS_NAMESPACE[] = "mia-system";
constexpr char BRIGHTNESS_KEY[] = "bright";
constexpr char VOLUME_KEY[] = "volume";
constexpr char KEY_BEEP_KEY[] = "keybeep";
constexpr uint8_t DEFAULT_BRIGHTNESS = 80;
constexpr uint8_t DEFAULT_VOLUME = 70;
constexpr uint8_t BACKLIGHT_CHANNEL = 7;
constexpr uint32_t BACKLIGHT_FREQUENCY_HZ = 5000;
constexpr uint8_t BACKLIGHT_RESOLUTION_BITS = 8;
constexpr uint32_t BACKLIGHT_MAX_DUTY = 255;

uint8_t g_brightness = DEFAULT_BRIGHTNESS;
uint8_t g_volume = DEFAULT_VOLUME;
bool g_keyBeep = true;
bool g_backlightEnabled = true;
bool g_backlightReady = false;

esp_err_t openStore(nvs_open_mode_t mode, nvs_handle_t *handle) {
  esp_err_t err = nvs_open(NVS_NAMESPACE, mode, handle);
  if (err != ESP_ERR_NVS_NOT_INITIALIZED) return err;
  err = nvs_flash_init();
  return err == ESP_OK ? nvs_open(NVS_NAMESPACE, mode, handle) : err;
}

uint8_t readValue(const char *key, uint8_t fallback, uint8_t maximum) {
  nvs_handle_t handle;
  if (openStore(NVS_READONLY, &handle) != ESP_OK) return fallback;
  uint8_t value = fallback;
  const esp_err_t err = nvs_get_u8(handle, key, &value);
  nvs_close(handle);
  return err == ESP_OK && value <= maximum ? value : fallback;
}

bool writeValue(const char *key, uint8_t value) {
  nvs_handle_t handle;
  if (openStore(NVS_READWRITE, &handle) != ESP_OK) return false;
  esp_err_t err = nvs_set_u8(handle, key, value);
  if (err == ESP_OK) err = nvs_commit(handle);
  nvs_close(handle);
  return err == ESP_OK;
}

void applyBacklight() {
  if (TFT_BL_PIN < 0) return;
  if (!g_backlightReady) {
    ledcSetup(BACKLIGHT_CHANNEL, BACKLIGHT_FREQUENCY_HZ, BACKLIGHT_RESOLUTION_BITS);
    ledcAttachPin(TFT_BL_PIN, BACKLIGHT_CHANNEL);
    g_backlightReady = true;
  }
  const uint32_t level = g_backlightEnabled ? g_brightness : 0;
  const uint32_t duty = BACKLIGHT_MAX_DUTY - level * BACKLIGHT_MAX_DUTY / 100;
  ledcWrite(BACKLIGHT_CHANNEL, duty);
}

}  // namespace

void miaSystemSettingsInit(bool skipPersisted) {
  if (!skipPersisted) {
    g_brightness = readValue(BRIGHTNESS_KEY, DEFAULT_BRIGHTNESS, 100);
    if (g_brightness < 10) g_brightness = DEFAULT_BRIGHTNESS;
    g_volume = readValue(VOLUME_KEY, DEFAULT_VOLUME, 100);
    g_keyBeep = readValue(KEY_BEEP_KEY, 1, 1) != 0;
  }
  launcherTracef("[system-settings] brightness=%u volume=%u key-beep=%u safe=%u",
                 g_brightness, g_volume, g_keyBeep ? 1 : 0, skipPersisted ? 1 : 0);
  applyBacklight();
}

uint8_t miaSystemBrightness(void) { return g_brightness; }

bool miaSystemSetBrightness(uint8_t brightness) {
  if (brightness < 10 || brightness > 100 || !writeValue(BRIGHTNESS_KEY, brightness)) {
    return false;
  }
  g_brightness = brightness;
  applyBacklight();
  return true;
}

void miaSystemSetBacklightEnabled(bool enabled) {
  g_backlightEnabled = enabled;
  applyBacklight();
}

uint8_t miaSystemVolume(void) { return g_volume; }

bool miaSystemSetVolume(uint8_t volume) {
  if (volume > 100 || !writeValue(VOLUME_KEY, volume)) return false;
  g_volume = volume;
  return true;
}

bool miaSystemKeyBeep(void) { return g_keyBeep; }

bool miaSystemSetKeyBeep(bool enabled) {
  if (!writeValue(KEY_BEEP_KEY, enabled ? 1 : 0)) return false;
  g_keyBeep = enabled;
  if (!enabled) digitalWrite(BEEP_PIN, LOW);
  return true;
}
