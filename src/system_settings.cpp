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
constexpr char IDLE_TIMEOUT_KEY[] = "idle";
constexpr uint8_t DEFAULT_BRIGHTNESS = 80;
constexpr uint8_t DEFAULT_VOLUME = 70;
constexpr uint8_t DEFAULT_IDLE_TIMEOUT_MINUTES = 0;
constexpr uint8_t BACKLIGHT_CHANNEL = 7;
constexpr uint32_t BACKLIGHT_FREQUENCY_HZ = 5000;
constexpr uint8_t BACKLIGHT_RESOLUTION_BITS = 8;
constexpr uint32_t BACKLIGHT_MAX_DUTY = 255;

uint8_t g_brightness = DEFAULT_BRIGHTNESS;
uint8_t g_volume = DEFAULT_VOLUME;
uint8_t g_idleTimeoutMinutes = DEFAULT_IDLE_TIMEOUT_MINUTES;
bool g_keyBeep = true;
bool g_backlightEnabled = true;
bool g_backlightReady = false;
bool g_idleDimmed = false;
bool g_idleOff = false;
uint32_t g_lastActivityMs = 0;
bool g_idleClockReady = false;

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
  uint8_t effectiveBrightness = g_brightness;
  if (g_idleDimmed) {
    effectiveBrightness = g_brightness / 4;
    if (effectiveBrightness < 10) effectiveBrightness = 10;
  }
  const uint32_t level = g_backlightEnabled && !g_idleOff ? effectiveBrightness : 0;
  const uint32_t duty = BACKLIGHT_MAX_DUTY - level * BACKLIGHT_MAX_DUTY / 100;
  ledcWrite(BACKLIGHT_CHANNEL, duty);
}

}  // namespace

void miaSystemSettingsInit(bool skipPersisted) {
  if (!skipPersisted) {
    g_brightness = readValue(BRIGHTNESS_KEY, DEFAULT_BRIGHTNESS, 100);
    if (g_brightness < 10) g_brightness = DEFAULT_BRIGHTNESS;
    g_volume = readValue(VOLUME_KEY, DEFAULT_VOLUME, 100);
    g_idleTimeoutMinutes = readValue(IDLE_TIMEOUT_KEY, DEFAULT_IDLE_TIMEOUT_MINUTES, 30);
    if (g_idleTimeoutMinutes != 0 && g_idleTimeoutMinutes != 1 &&
        g_idleTimeoutMinutes != 5 && g_idleTimeoutMinutes != 10 &&
        g_idleTimeoutMinutes != 30) {
      g_idleTimeoutMinutes = DEFAULT_IDLE_TIMEOUT_MINUTES;
    }
    g_keyBeep = readValue(KEY_BEEP_KEY, 1, 1) != 0;
  }
  g_idleDimmed = false;
  g_idleOff = false;
  g_lastActivityMs = millis();
  g_idleClockReady = true;
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
  g_lastActivityMs = millis();
  g_idleDimmed = false;
  g_idleOff = false;
  applyBacklight();
  return true;
}

void miaSystemSetBacklightEnabled(bool enabled) {
  g_backlightEnabled = enabled;
  applyBacklight();
}

uint8_t miaSystemIdleTimeoutMinutes(void) { return g_idleTimeoutMinutes; }

bool miaSystemSetIdleTimeoutMinutes(uint8_t minutes) {
  if (minutes != 0 && minutes != 1 && minutes != 5 && minutes != 10 && minutes != 30) {
    return false;
  }
  if (!writeValue(IDLE_TIMEOUT_KEY, minutes)) return false;
  g_idleTimeoutMinutes = minutes;
  g_idleDimmed = false;
  g_idleOff = false;
  g_lastActivityMs = millis();
  g_idleClockReady = true;
  applyBacklight();
  return true;
}

void miaSystemIdleTick(uint32_t nowMs, bool userActivity) {
  if (!g_idleClockReady) {
    g_lastActivityMs = nowMs;
    g_idleClockReady = true;
  }
  if (userActivity) {
    g_lastActivityMs = nowMs;
    if (g_idleDimmed || g_idleOff) {
      g_idleDimmed = false;
      g_idleOff = false;
      applyBacklight();
    }
    return;
  }
  if (g_idleTimeoutMinutes == 0) return;

  const uint32_t dimAfterMs = static_cast<uint32_t>(g_idleTimeoutMinutes) * 60UL * 1000UL;
  const uint32_t idleMs = nowMs - g_lastActivityMs;
  if (idleMs >= dimAfterMs * 2U) {
    if (!g_idleOff) {
      g_idleOff = true;
      g_idleDimmed = true;
      applyBacklight();
    }
  } else if (idleMs >= dimAfterMs && !g_idleDimmed) {
    g_idleDimmed = true;
    applyBacklight();
  }
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
