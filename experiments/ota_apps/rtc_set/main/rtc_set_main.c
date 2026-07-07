#include "mia_host_abi.h"

#include <stdint.h>
#include <stdio.h>

typedef struct FieldSpec {
  const char *label;
  uint8_t min_value;
  uint8_t max_value;
} FieldSpec;

static const FieldSpec FIELDS[6] = {
    {"Year", 0, 99},   {"Month", 1, 12}, {"Day", 1, 31},
    {"Hour", 0, 23},   {"Minute", 0, 59}, {"Second", 0, 59},
};

static MiaHostDateTime rtc_now = {2000, 1, 1, 0, 0, 0, 6};
static MiaHostDateTime edit_time = {2000, 1, 1, 0, 0, 0, 6};
static uint8_t selected_field;
static const char *status_text = "B:Read  A:Save";

static uint8_t exit_pressed(void) {
  return mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
         mia_host_button_down(MIA_HOST_BUTTON_START);
}

static void clamp_edit_time(void) {
  if (edit_time.month < 1) edit_time.month = 1;
  if (edit_time.month > 12) edit_time.month = 12;
  uint8_t max_day = mia_host_rtc_days_in_month(edit_time.year, edit_time.month);
  if (edit_time.day < 1) edit_time.day = 1;
  if (edit_time.day > max_day) edit_time.day = max_day;
  edit_time.weekday = mia_host_rtc_day_of_week(&edit_time);
}

static void adjust_selected_field(int8_t delta) {
  int value = 0;
  switch (selected_field) {
    case 0:
      value = (int)edit_time.year + delta;
      if (value < 2000) value = 2099;
      if (value > 2099) value = 2000;
      edit_time.year = (uint16_t)value;
      break;
    case 1:
      value = (int)edit_time.month + delta;
      if (value < 1) value = 12;
      if (value > 12) value = 1;
      edit_time.month = (uint8_t)value;
      break;
    case 2:
      value = (int)edit_time.day + delta;
      if (value < 1) value = mia_host_rtc_days_in_month(edit_time.year, edit_time.month);
      if (value > mia_host_rtc_days_in_month(edit_time.year, edit_time.month)) value = 1;
      edit_time.day = (uint8_t)value;
      break;
    case 3:
      value = (int)edit_time.hour + delta;
      if (value < 0) value = 23;
      if (value > 23) value = 0;
      edit_time.hour = (uint8_t)value;
      break;
    case 4:
      value = (int)edit_time.minute + delta;
      if (value < 0) value = 59;
      if (value > 59) value = 0;
      edit_time.minute = (uint8_t)value;
      break;
    case 5:
      value = (int)edit_time.second + delta;
      if (value < 0) value = 59;
      if (value > 59) value = 0;
      edit_time.second = (uint8_t)value;
      break;
  }
  clamp_edit_time();
  status_text = "Edited";
}

static void load_rtc_into_editor(void) {
  if (mia_host_rtc_read(&rtc_now)) {
    edit_time = rtc_now;
    status_text = "RTC read OK";
  } else {
    status_text = "RTC read FAIL";
  }
}

static void draw_rtc_app(void) {
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 6, "RTC Set", MIA_HOST_BLACK, MIA_HOST_YELLOW);

  char line[48];
  snprintf(line, sizeof(line), "RTC  %04u-%02u-%02u %02u:%02u:%02u", rtc_now.year,
           rtc_now.month, rtc_now.day, rtc_now.hour, rtc_now.minute, rtc_now.second);
  mia_host_draw_text(8, 34, line, MIA_HOST_CYAN, MIA_HOST_BLACK);

  snprintf(line, sizeof(line), "EDIT %04u-%02u-%02u %02u:%02u:%02u", edit_time.year,
           edit_time.month, edit_time.day, edit_time.hour, edit_time.minute,
           edit_time.second);
  mia_host_draw_text(8, 50, line, MIA_HOST_WHITE, MIA_HOST_BLACK);

  for (uint8_t i = 0; i < 6; ++i) {
    int32_t y = 82 + i * 18;
    uint8_t bg = i == selected_field ? MIA_HOST_BLUE : MIA_HOST_BLACK;
    uint8_t fg = i == selected_field ? MIA_HOST_YELLOW : MIA_HOST_WHITE;
    int value = 0;
    switch (i) {
      case 0: value = edit_time.year; break;
      case 1: value = edit_time.month; break;
      case 2: value = edit_time.day; break;
      case 3: value = edit_time.hour; break;
      case 4: value = edit_time.minute; break;
      case 5: value = edit_time.second; break;
    }
    mia_host_fill_rect(8, y - 2, 180, 14, bg);
    if (i == 0) {
      snprintf(line, sizeof(line), "%s: %04d", FIELDS[i].label, value);
    } else {
      snprintf(line, sizeof(line), "%s: %02d", FIELDS[i].label, value);
    }
    mia_host_draw_text(12, y, line, fg, bg);
  }

  mia_host_draw_text(8, 198, status_text, MIA_HOST_GREEN, MIA_HOST_BLACK);
  mia_host_draw_text(8, 210, "UP/DN field  LT/RT value", MIA_HOST_GRAY,
                     MIA_HOST_BLACK);
  mia_host_draw_text(8, 224, "A:Save  B:Read  SEL+ST:Exit", MIA_HOST_GRAY,
                     MIA_HOST_BLACK);
  mia_host_present();
}

int rtc_set_main_impl(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  if (mia_host_abi_version() != 2) {
    return 1;
  }
  selected_field = 0;
  load_rtc_into_editor();
  draw_rtc_app();
  while (1) {
    mia_host_buttons_poll();
    if (exit_pressed()) {
      break;
    }
    uint8_t changed = 0;
    if (mia_host_button_pressed(MIA_HOST_BUTTON_UP) && selected_field > 0) {
      --selected_field;
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN) && selected_field + 1 < 6) {
      ++selected_field;
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_L)) {
      adjust_selected_field(-1);
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_R)) {
      adjust_selected_field(1);
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
      if (mia_host_rtc_write(&edit_time)) {
        load_rtc_into_editor();
        status_text = "RTC write OK";
      } else {
        status_text = "RTC write FAIL";
      }
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
      load_rtc_into_editor();
      changed = 1;
    }
    if (changed) {
      draw_rtc_app();
    }
    mia_host_delay_ms(20);
  }
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_present();
  return 0;
}
