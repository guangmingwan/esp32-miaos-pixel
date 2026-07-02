#include "apps/rtc_app.h"

#include <Arduino.h>
#include <Wire.h>

#include "lava_native_display.h"

namespace {

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

constexpr uint8_t RTC_I2C_ADDR = 0x51;
constexpr uint8_t RTC_TIME_REG = 0x02;
constexpr uint8_t FIELD_COUNT = 6;

struct RtcDateTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

struct FieldSpec {
  const char *label;
  uint8_t minValue;
  uint8_t maxValue;
};

constexpr FieldSpec FIELDS[FIELD_COUNT] = {
    {"Year", 0, 99},
    {"Month", 1, 12},
    {"Day", 1, 31},
    {"Hour", 0, 23},
    {"Minute", 0, 59},
    {"Second", 0, 59},
};

RtcDateTime g_rtcNow = {2000, 1, 1, 0, 0, 0};
RtcDateTime g_editTime = {2000, 1, 1, 0, 0, 0};
uint8_t g_selectedField = 0;
const char *g_status = "B:Read  A:Save";

uint8_t toBcd(uint8_t value) { return static_cast<uint8_t>(((value / 10) << 4) | (value % 10)); }

uint8_t fromBcd(uint8_t value) { return static_cast<uint8_t>(((value >> 4) * 10) + (value & 0x0F)); }

bool isLeapYear(uint16_t year) {
  if (year % 400 == 0) {
    return true;
  }
  if (year % 100 == 0) {
    return false;
  }
  return (year % 4) == 0;
}

uint8_t daysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2 && isLeapYear(year)) {
    return 29;
  }
  return days[month - 1];
}

uint8_t dayOfWeek(const RtcDateTime &dateTime) {
  int year = dateTime.year;
  int month = dateTime.month;
  if (month < 3) {
    month += 12;
    --year;
  }
  const int k = year % 100;
  const int j = year / 100;
  const int h = (dateTime.day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
  return static_cast<uint8_t>((h + 6) % 7);
}

void clampDateTime(RtcDateTime &dateTime) {
  if (dateTime.month < 1) {
    dateTime.month = 1;
  }
  if (dateTime.month > 12) {
    dateTime.month = 12;
  }
  const uint8_t maxDay = daysInMonth(dateTime.year, dateTime.month);
  if (dateTime.day < 1) {
    dateTime.day = 1;
  }
  if (dateTime.day > maxDay) {
    dateTime.day = maxDay;
  }
}

bool readRtc(RtcDateTime &dateTime) {
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(RTC_TIME_REG);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(static_cast<int>(RTC_I2C_ADDR), 7) != 7) {
    return false;
  }

  const uint8_t seconds = Wire.read();
  const uint8_t minutes = Wire.read();
  const uint8_t hours = Wire.read();
  const uint8_t days = Wire.read();
  const uint8_t weekdays = Wire.read();
  const uint8_t months = Wire.read();
  const uint8_t years = Wire.read();
  (void)weekdays;

  dateTime.second = fromBcd(seconds & 0x7F);
  dateTime.minute = fromBcd(minutes & 0x7F);
  dateTime.hour = fromBcd(hours & 0x3F);
  dateTime.day = fromBcd(days & 0x3F);
  dateTime.month = fromBcd(months & 0x1F);
  dateTime.year = static_cast<uint16_t>(2000 + fromBcd(years));
  clampDateTime(dateTime);
  return true;
}

bool writeRtc(const RtcDateTime &dateTime) {
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(RTC_TIME_REG);
  Wire.write(toBcd(dateTime.second));
  Wire.write(toBcd(dateTime.minute));
  Wire.write(toBcd(dateTime.hour));
  Wire.write(toBcd(dateTime.day));
  Wire.write(toBcd(dayOfWeek(dateTime)) & 0x07);
  Wire.write(toBcd(dateTime.month) & 0x1F);
  Wire.write(toBcd(static_cast<uint8_t>(dateTime.year % 100)));
  return Wire.endTransmission() == 0;
}

void adjustSelectedField(int8_t delta) {
  int value = 0;
  switch (g_selectedField) {
    case 0:
      value = static_cast<int>(g_editTime.year) + delta;
      if (value < 2000) {
        value = 2099;
      }
      if (value > 2099) {
        value = 2000;
      }
      g_editTime.year = static_cast<uint16_t>(value);
      break;
    case 1:
      value = static_cast<int>(g_editTime.month) + delta;
      if (value < 1) {
        value = 12;
      }
      if (value > 12) {
        value = 1;
      }
      g_editTime.month = static_cast<uint8_t>(value);
      break;
    case 2:
      value = static_cast<int>(g_editTime.day) + delta;
      if (value < 1) {
        value = daysInMonth(g_editTime.year, g_editTime.month);
      }
      if (value > daysInMonth(g_editTime.year, g_editTime.month)) {
        value = 1;
      }
      g_editTime.day = static_cast<uint8_t>(value);
      break;
    case 3:
      value = static_cast<int>(g_editTime.hour) + delta;
      if (value < 0) {
        value = 23;
      }
      if (value > 23) {
        value = 0;
      }
      g_editTime.hour = static_cast<uint8_t>(value);
      break;
    case 4:
      value = static_cast<int>(g_editTime.minute) + delta;
      if (value < 0) {
        value = 59;
      }
      if (value > 59) {
        value = 0;
      }
      g_editTime.minute = static_cast<uint8_t>(value);
      break;
    case 5:
      value = static_cast<int>(g_editTime.second) + delta;
      if (value < 0) {
        value = 59;
      }
      if (value > 59) {
        value = 0;
      }
      g_editTime.second = static_cast<uint8_t>(value);
      break;
  }
  clampDateTime(g_editTime);
  g_status = "Edited";
}

void loadRtcIntoEditor() {
  if (readRtc(g_rtcNow)) {
    g_editTime = g_rtcNow;
    g_status = "RTC read OK";
  } else {
    g_status = "RTC read FAIL";
  }
}

void drawRtcApp(AppContext &context) {
  if (!context.tftReady) {
    return;
  }

  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(4, 6, "RTC Set", LAVA_BLACK, LAVA_YELLOW);

  char line[48];
  snprintf(line, sizeof(line), "RTC  %04u-%02u-%02u %02u:%02u:%02u",
           g_rtcNow.year, g_rtcNow.month, g_rtcNow.day,
           g_rtcNow.hour, g_rtcNow.minute, g_rtcNow.second);
  lavaDrawText(8, 34, line, LAVA_CYAN, LAVA_BLACK);

  snprintf(line, sizeof(line), "EDIT %04u-%02u-%02u %02u:%02u:%02u",
           g_editTime.year, g_editTime.month, g_editTime.day,
           g_editTime.hour, g_editTime.minute, g_editTime.second);
  lavaDrawText(8, 50, line, LAVA_WHITE, LAVA_BLACK);

  for (uint8_t i = 0; i < FIELD_COUNT; ++i) {
    const int16_t y = 82 + i * 18;
    const bool selected = i == g_selectedField;
    const uint8_t bg = selected ? LAVA_BLUE : LAVA_BLACK;
    const uint8_t fg = selected ? LAVA_YELLOW : LAVA_WHITE;
    int value = 0;
    switch (i) {
      case 0: value = g_editTime.year; break;
      case 1: value = g_editTime.month; break;
      case 2: value = g_editTime.day; break;
      case 3: value = g_editTime.hour; break;
      case 4: value = g_editTime.minute; break;
      case 5: value = g_editTime.second; break;
    }
    lavaFillRect(8, y - 2, 180, 14, bg);
    if (i == 0) {
      snprintf(line, sizeof(line), "%s: %04d", FIELDS[i].label, value);
    } else {
      snprintf(line, sizeof(line), "%s: %02d", FIELDS[i].label, value);
    }
    lavaDrawText(12, y, line, fg, bg);
  }

  lavaDrawText(8, 198, g_status, LAVA_GREEN, LAVA_BLACK);
  lavaDrawText(8, 210, "UP/DN field  LT/RT value", LAVA_GRAY, LAVA_BLACK);
  lavaDrawText(8, 224, "A:Save  B:Read  SEL+ST:Exit", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

void rtcBegin(AppContext &context) {
  g_selectedField = 0;
  loadRtcIntoEditor();
  drawRtcApp(context);
}

void rtcTick(AppContext &context, uint32_t nowMs) {
  (void)nowMs;
  bool changed = false;
  if (context.buttons[2].pressed && g_selectedField > 0) {
    --g_selectedField;
    changed = true;
  }
  if (context.buttons[3].pressed && g_selectedField + 1 < FIELD_COUNT) {
    ++g_selectedField;
    changed = true;
  }
  if (context.buttons[4].pressed) {
    adjustSelectedField(-1);
    changed = true;
  }
  if (context.buttons[5].pressed) {
    adjustSelectedField(1);
    changed = true;
  }
  if (context.buttons[0].pressed) {
    if (writeRtc(g_editTime)) {
      loadRtcIntoEditor();
      g_status = "RTC write OK";
    } else {
      g_status = "RTC write FAIL";
    }
    changed = true;
  }
  if (context.buttons[1].pressed) {
    loadRtcIntoEditor();
    changed = true;
  }

  if (changed) {
    drawRtcApp(context);
  }
}

void rtcEnd(AppContext &context) {
  (void)context;
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

}

const LauncherApp &rtcApp() {
  static const LauncherApp app = {"RTC Set", rtcBegin, rtcTick, rtcEnd};
  return app;
}
