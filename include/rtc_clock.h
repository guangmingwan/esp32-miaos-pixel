#pragma once

#include <Arduino.h>

struct RtcDateTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t weekday;
};

bool rtcReadDateTime(RtcDateTime &dateTime);
bool rtcWriteDateTime(const RtcDateTime &dateTime);
void rtcClampDateTime(RtcDateTime &dateTime);
uint8_t rtcDaysInMonth(uint16_t year, uint8_t month);
uint8_t rtcDayOfWeek(const RtcDateTime &dateTime);
const char *rtcWeekdayShortName(uint8_t weekday);
