#pragma once

#include <stdint.h>

typedef struct {
    const char *title;
    const char *language;
    const char *font;
    const char *date_time;
    const char *brightness;
    const char *volume;
    const char *key_beep;
    const char *enabled;
    const char *disabled;
    const char *english;
    const char *chinese;
    const char *saved;
    const char *save_failed;
    const char *restart_hint;
    const char *rtc_read_failed;
    const char *main_controls;
    const char *main_exit;
    const char *date_title;
    const char *year;
    const char *month;
    const char *day;
    const char *hour;
    const char *minute;
    const char *second;
    const char *date_controls;
    const char *date_save;
} SettingsText;

const SettingsText *settings_text(uint8_t language);
