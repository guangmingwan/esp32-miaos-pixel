#include "mia_host_abi.h"

#include <Arduino.h>
#include <SD.h>
#include <stdint.h>
#include <string.h>

#include "app.h"
#include "lcd_ili9342.h"
#include "lava_native_display.h"
#include "lava_text.h"
#include "mia_i18n.h"
#include "pins.h"
#include "system_settings.h"
#include "rtc_clock.h"

extern ButtonState g_allButtons[];
void updateAllButtons();

uint32_t mia_host_abi_version(void) { return 2; }

void mia_host_log(const char *message) {
  Serial.printf("[mia_host_abi] %s\n", message == nullptr ? "<null>" : message);
}

int32_t mia_host_screen_width(void) { return LAVA_SCREEN_W; }

int32_t mia_host_screen_height(void) { return LAVA_SCREEN_H; }

void mia_host_clear(uint8_t color) {
  if (lavaDisplayReady()) {
    lavaClear(color);
  }
}

void mia_host_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color) {
  if (!lavaDisplayReady()) {
    return;
  }
  if (w <= 0 || h <= 0 || x >= LAVA_SCREEN_W || y >= LAVA_SCREEN_H) {
    return;
  }
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > LAVA_SCREEN_W) {
    w = LAVA_SCREEN_W - x;
  }
  if (y + h > LAVA_SCREEN_H) {
    h = LAVA_SCREEN_H - y;
  }
  if (w <= 0 || h <= 0) {
    return;
  }
  lavaFillRect(static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(w),
               static_cast<int16_t>(h), color);
}

void mia_host_fill_screen_rgb565(uint16_t color) {
  if (lavaDisplayReady()) {
    Lcd.fillScreen(color);
  }
}

void mia_host_draw_text(int32_t x, int32_t y, const char *text, uint8_t fg,
                         uint8_t bg) {
  if (!lavaDisplayReady()) {
    return;
  }
  if (text == nullptr || x >= LAVA_SCREEN_W || y < -7 || y >= LAVA_SCREEN_H) {
    return;
  }
  if (x < -LAVA_SCREEN_W) {
    x = -LAVA_SCREEN_W;
  }
  lavaDrawText(static_cast<int16_t>(x), static_cast<int16_t>(y), text, fg, bg);
}

int32_t mia_host_text_height(void) { return lavaFontHeight(); }

int32_t mia_host_text_width(const char *text) { return lavaTextWidth(text); }

void mia_host_present(void) {
  if (lavaDisplayReady()) {
    lavaPresent();
  }
}

void mia_host_buttons_poll(void) { updateAllButtons(); }

uint8_t mia_host_button_down(uint8_t button) {
  if (button >= 14) {
    return 0;
  }
  return g_allButtons[button].down ? 1 : 0;
}

uint8_t mia_host_button_pressed(uint8_t button) {
  if (button >= 14) {
    return 0;
  }
  return g_allButtons[button].pressed ? 1 : 0;
}

uint8_t mia_host_button_released(uint8_t button) {
  if (button >= 14) {
    return 0;
  }
  return g_allButtons[button].released ? 1 : 0;
}

void mia_host_delay_ms(uint32_t ms) { delay(ms); }

uint32_t mia_host_millis(void) { return millis(); }

uint8_t mia_host_language(void) {
  return miaLanguage() == MiaLanguage::Chinese ? 1 : 0;
}

uint8_t mia_host_language_set(uint8_t language) {
  if (language > static_cast<uint8_t>(MiaLanguage::Chinese)) {
    return 0;
  }
  miaSetLanguage(static_cast<MiaLanguage>(language));
  return 1;
}

uint8_t mia_host_font_get(void) {
  return static_cast<uint8_t>(lavaFontFace());
}

uint8_t mia_host_font_set(uint8_t font) {
  if (font > static_cast<uint8_t>(LavaFontFace::DroidGbk12)) {
    return 0;
  }
  lavaSetFontFace(static_cast<LavaFontFace>(font));
  return 1;
}

uint8_t mia_host_font_count(void) {
  return static_cast<uint8_t>(LavaFontFace::DroidGbk12) + 1;
}

const char *mia_host_font_name(uint8_t font) {
  if (font >= mia_host_font_count()) {
    return "?";
  }
  return lavaFontName(static_cast<LavaFontFace>(font));
}

void mia_host_backlight_set(uint8_t enabled) {
  miaSystemSetBacklightEnabled(enabled != 0);
}

uint8_t mia_host_brightness_get(void) { return miaSystemBrightness(); }

uint8_t mia_host_brightness_set(uint8_t brightness) {
  return miaSystemSetBrightness(brightness) ? 1 : 0;
}

uint8_t mia_host_idle_timeout_get(void) { return miaSystemIdleTimeoutMinutes(); }

uint8_t mia_host_idle_timeout_set(uint8_t minutes) {
  return miaSystemSetIdleTimeoutMinutes(minutes) ? 1 : 0;
}

uint8_t mia_host_volume_get(void) { return miaSystemVolume(); }

uint8_t mia_host_volume_set(uint8_t volume) {
  return miaSystemSetVolume(volume) ? 1 : 0;
}

uint8_t mia_host_key_beep_get(void) { return miaSystemKeyBeep() ? 1 : 0; }

uint8_t mia_host_key_beep_set(uint8_t enabled) {
  return miaSystemSetKeyBeep(enabled != 0) ? 1 : 0;
}

static void copyRtcToHost(const RtcDateTime &src, MiaHostDateTime *dest) {
  dest->year = src.year;
  dest->month = src.month;
  dest->day = src.day;
  dest->hour = src.hour;
  dest->minute = src.minute;
  dest->second = src.second;
  dest->weekday = src.weekday;
}

static RtcDateTime copyRtcFromHost(const MiaHostDateTime *src) {
  RtcDateTime dest = {src->year, src->month, src->day, src->hour,
                      src->minute, src->second, src->weekday};
  return dest;
}

uint8_t mia_host_rtc_read(MiaHostDateTime *date_time) {
  if (date_time == nullptr) {
    return 0;
  }
  RtcDateTime value = {2000, 1, 1, 0, 0, 0, 6};
  if (!rtcReadDateTime(value)) {
    return 0;
  }
  copyRtcToHost(value, date_time);
  return 1;
}

uint8_t mia_host_rtc_write(const MiaHostDateTime *date_time) {
  if (date_time == nullptr) {
    return 0;
  }
  RtcDateTime value = copyRtcFromHost(date_time);
  rtcClampDateTime(value);
  value.weekday = rtcDayOfWeek(value);
  return rtcWriteDateTime(value) ? 1 : 0;
}

uint8_t mia_host_rtc_days_in_month(uint16_t year, uint8_t month) {
  if (month < 1 || month > 12) {
    return 31;
  }
  return rtcDaysInMonth(year, month);
}

uint8_t mia_host_rtc_day_of_week(const MiaHostDateTime *date_time) {
  if (date_time == nullptr) {
    return 0;
  }
  RtcDateTime value = copyRtcFromHost(date_time);
  rtcClampDateTime(value);
  return rtcDayOfWeek(value);
}

static const char *hostBaseName(const char *path) {
  const char *slash = strrchr(path, '/');
  return slash == nullptr ? path : slash + 1;
}

static void copyDirEntry(MiaHostDirEntry &dest, File &entry) {
  memset(&dest, 0, sizeof(dest));
  strncpy(dest.name, hostBaseName(entry.name()), sizeof(dest.name) - 1);
  dest.is_dir = entry.isDirectory() ? 1 : 0;
  dest.size = entry.isDirectory() ? 0 : static_cast<uint32_t>(entry.size());
}

int32_t mia_host_sd_list_dir(const char *path, MiaHostDirEntry *entries,
                             uint32_t capacity) {
  Serial.printf("[mia_host_abi] sd_list_dir path='%s' cap=%u core=%d\n",
                path == nullptr ? "<null>" : path,
                static_cast<unsigned>(capacity), static_cast<int>(xPortGetCoreID()));
  if (path == nullptr || entries == nullptr || capacity == 0) {
    return -1;
  }

  Serial.println("[mia_host_abi] sd_list_dir: SD.open");
  File directory = SD.open(path);
  Serial.printf("[mia_host_abi] sd_list_dir: SD.open => %d\n", directory ? 1 : 0);
  if (!directory) {
    return -2;
  }
  if (!directory.isDirectory()) {
    directory.close();
    return -3;
  }

  uint32_t count = 0;
  for (uint8_t pass = 0; pass < 2 && count < capacity; ++pass) {
    directory.rewindDirectory();
    while (count < capacity) {
      File entry = directory.openNextFile();
      if (!entry) {
        break;
      }
      const bool wantDirectory = pass == 0;
      if (entry.isDirectory() == wantDirectory) {
        copyDirEntry(entries[count], entry);
        ++count;
      }
      entry.close();
    }
  }
  directory.close();
  Serial.printf("[mia_host_abi] sd_list_dir done count=%u\n",
                static_cast<unsigned>(count));
  return static_cast<int32_t>(count);
}
