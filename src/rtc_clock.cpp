#include "rtc_clock.h"

#include <Wire.h>

namespace {

constexpr uint8_t RTC_I2C_ADDR = 0x51;
constexpr uint8_t RTC_TIME_REG = 0x02;

uint8_t toBcd(uint8_t value) {
  return static_cast<uint8_t>(((value / 10) << 4) | (value % 10));
}

uint8_t fromBcd(uint8_t value) {
  return static_cast<uint8_t>(((value >> 4) * 10) + (value & 0x0F));
}

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

}

void rtcClampDateTime(RtcDateTime &dateTime) {
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

uint8_t rtcDaysInMonth(uint16_t year, uint8_t month) { return daysInMonth(year, month); }

uint8_t rtcDayOfWeek(const RtcDateTime &dateTime) {
  int year = dateTime.year;
  int month = dateTime.month;
  if (month < 3) {
    month += 12;
    --year;
  }
  const int k = year % 100;
  const int j = year / 100;
  const int h =
      (dateTime.day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
  return static_cast<uint8_t>((h + 6) % 7);
}

const char *rtcWeekdayShortName(uint8_t weekday) {
  static const char *const kWeekdays[] = {"Sun", "Mon", "Tue", "Wed",
                                          "Thu", "Fri", "Sat"};
  return weekday < 7 ? kWeekdays[weekday] : "---";
}

bool rtcReadDateTime(RtcDateTime &dateTime) {
  Wire.setTimeOut(50);
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

  dateTime.second = fromBcd(seconds & 0x7F);
  dateTime.minute = fromBcd(minutes & 0x7F);
  dateTime.hour = fromBcd(hours & 0x3F);
  dateTime.day = fromBcd(days & 0x3F);
  dateTime.month = fromBcd(months & 0x1F);
  dateTime.year = static_cast<uint16_t>(2000 + fromBcd(years));
  rtcClampDateTime(dateTime);
  dateTime.weekday = weekdays & 0x07;
  if (dateTime.weekday > 6) {
    dateTime.weekday = rtcDayOfWeek(dateTime);
  }
  return true;
}

bool rtcWriteDateTime(const RtcDateTime &dateTime) {
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(RTC_TIME_REG);
  Wire.write(toBcd(dateTime.second));
  Wire.write(toBcd(dateTime.minute));
  Wire.write(toBcd(dateTime.hour));
  Wire.write(toBcd(dateTime.day));
  Wire.write(toBcd(rtcDayOfWeek(dateTime)) & 0x07);
  Wire.write(toBcd(dateTime.month) & 0x1F);
  Wire.write(toBcd(static_cast<uint8_t>(dateTime.year % 100)));
  return Wire.endTransmission() == 0;
}
