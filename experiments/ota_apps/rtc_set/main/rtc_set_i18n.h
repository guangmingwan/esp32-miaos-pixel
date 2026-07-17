#pragma once

typedef struct {
  const char *title;
  const char *label_year;
  const char *label_month;
  const char *label_day;
  const char *label_hour;
  const char *label_minute;
  const char *label_second;
  const char *status_edited;
  const char *status_read_ok;
  const char *status_read_fail;
  const char *status_write_ok;
  const char *status_write_fail;
  const char *controls_field;
  const char *controls_save;
} RtcSetText;

const RtcSetText *rtc_set_text(void);
