#include "mia_host_abi.h"
#include "settings_i18n.h"

#include <stdint.h>
#include <stdio.h>

#define MAIN_ROWS 7
#define MAIN_ROW_HEIGHT 22
#define MAIN_ROW_START 23
#define RTC_FIELDS 6

typedef enum {
    SCREEN_MAIN,
    SCREEN_DATE_TIME,
} SettingsScreen;

typedef enum {
    STATUS_NONE,
    STATUS_SAVED,
    STATUS_FAILED,
    STATUS_RESTART,
    STATUS_RTC_FAILED,
} StatusKind;

static SettingsScreen screen;
static StatusKind status_kind;
static uint8_t selected_row;
static uint8_t selected_field;
static uint8_t ui_language;
static uint8_t language;
static uint8_t font;
static uint8_t brightness;
static uint8_t volume;
static uint8_t idle_timeout;
static uint8_t key_beep;
static MiaHostDateTime rtc_value = {2000, 1, 1, 0, 0, 0, 6};

static uint8_t exit_pressed(void) {
    return mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
           mia_host_button_down(MIA_HOST_BUTTON_START);
}

static const char *status_text(const SettingsText *text) {
    switch (status_kind) {
        case STATUS_SAVED: return text->saved;
        case STATUS_FAILED: return text->save_failed;
        case STATUS_RESTART: return text->restart_hint;
        case STATUS_RTC_FAILED: return text->rtc_read_failed;
        default: return "";
    }
}

static void draw_header(const char *title) {
    mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
    mia_host_draw_text(4, mia_host_text_y_centered(0, 20), title,
                       MIA_HOST_BLACK, MIA_HOST_YELLOW);
}

static void draw_main_row(uint8_t row, int32_t y, const char *label,
                          const char *value) {
    uint8_t selected = row == selected_row;
    uint8_t bg = selected ? MIA_HOST_BLUE : MIA_HOST_BLACK;
    uint8_t fg = selected ? MIA_HOST_YELLOW : MIA_HOST_WHITE;
    mia_host_fill_rect(6, y, 308, MAIN_ROW_HEIGHT - 1, bg);
    if (selected) mia_host_fill_rect(6, y, 3, MAIN_ROW_HEIGHT - 1, MIA_HOST_YELLOW);
    mia_host_draw_text(14, mia_host_text_y_centered(y, MAIN_ROW_HEIGHT), label, fg, bg);
    mia_host_draw_text(150, mia_host_text_y_centered(y, MAIN_ROW_HEIGHT), value, fg, bg);
}

static void draw_main(void) {
    const SettingsText *text = settings_text(ui_language);
    char date[32];
    char brightness_text[8];
    char volume_text[8];
    char idle_text[12];
    snprintf(date, sizeof(date), "%04u-%02u-%02u %02u:%02u",
             rtc_value.year, rtc_value.month, rtc_value.day, rtc_value.hour,
             rtc_value.minute);
    mia_host_clear(MIA_HOST_BLACK);
    draw_header(text->title);
    snprintf(brightness_text, sizeof(brightness_text), "%u%%", brightness);
    snprintf(volume_text, sizeof(volume_text), "%u%%", volume);
    if (idle_timeout == 0) {
        snprintf(idle_text, sizeof(idle_text), "%s", text->never);
    } else {
        snprintf(idle_text, sizeof(idle_text), "%u%s", idle_timeout, text->minutes_suffix);
    }
    draw_main_row(0, MAIN_ROW_START, text->language,
                  language == 1 ? text->chinese : text->english);
    draw_main_row(1, MAIN_ROW_START + 1 * MAIN_ROW_HEIGHT, text->font, mia_host_font_name(font));
    draw_main_row(2, MAIN_ROW_START + 2 * MAIN_ROW_HEIGHT, text->brightness, brightness_text);
    draw_main_row(3, MAIN_ROW_START + 3 * MAIN_ROW_HEIGHT, text->volume, volume_text);
    draw_main_row(4, MAIN_ROW_START + 4 * MAIN_ROW_HEIGHT, text->key_beep,
                  key_beep ? text->enabled : text->disabled);
    draw_main_row(5, MAIN_ROW_START + 5 * MAIN_ROW_HEIGHT, text->idle_timeout, idle_text);
    draw_main_row(6, MAIN_ROW_START + 6 * MAIN_ROW_HEIGHT, text->date_time, date);
    mia_host_draw_text(8, 186, status_text(text),
                       status_kind == STATUS_FAILED || status_kind == STATUS_RTC_FAILED
                           ? MIA_HOST_RED : MIA_HOST_GREEN,
                       MIA_HOST_BLACK);
    mia_host_draw_text(8, 204, text->main_controls, MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_draw_text(8, 222, text->main_exit, MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_present();
}

static const char *field_label(uint8_t field, const SettingsText *text) {
    static const char *empty = "";
    switch (field) {
        case 0: return text->year;
        case 1: return text->month;
        case 2: return text->day;
        case 3: return text->hour;
        case 4: return text->minute;
        case 5: return text->second;
        default: return empty;
    }
}

static int field_value(uint8_t field) {
    switch (field) {
        case 0: return rtc_value.year;
        case 1: return rtc_value.month;
        case 2: return rtc_value.day;
        case 3: return rtc_value.hour;
        case 4: return rtc_value.minute;
        default: return rtc_value.second;
    }
}

static void draw_date_time(void) {
    const SettingsText *text = settings_text(ui_language);
    char line[40];
    mia_host_clear(MIA_HOST_BLACK);
    draw_header(text->date_title);
    for (uint8_t field = 0; field < RTC_FIELDS; ++field) {
        int32_t y = 34 + field * 25;
        uint8_t selected = field == selected_field;
        uint8_t bg = selected ? MIA_HOST_BLUE : MIA_HOST_BLACK;
        uint8_t fg = selected ? MIA_HOST_YELLOW : MIA_HOST_WHITE;
        mia_host_fill_rect(8, y, 220, 21, bg);
        if (field == 0) {
            snprintf(line, sizeof(line), "%s: %04d", field_label(field, text),
                     field_value(field));
        } else {
            snprintf(line, sizeof(line), "%s: %02d", field_label(field, text),
                     field_value(field));
        }
        mia_host_draw_text(14, mia_host_text_y_centered(y, 21), line, fg, bg);
    }
    mia_host_draw_text(8, 186, status_text(text),
                       status_kind == STATUS_FAILED ? MIA_HOST_RED : MIA_HOST_GREEN,
                       MIA_HOST_BLACK);
    mia_host_draw_text(8, 204, text->date_controls, MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_draw_text(8, 222, text->date_save, MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_present();
}

static void clamp_date(void) {
    if (rtc_value.month < 1) rtc_value.month = 1;
    if (rtc_value.month > 12) rtc_value.month = 12;
    uint8_t days = mia_host_rtc_days_in_month(rtc_value.year, rtc_value.month);
    if (rtc_value.day < 1) rtc_value.day = 1;
    if (rtc_value.day > days) rtc_value.day = days;
    rtc_value.weekday = mia_host_rtc_day_of_week(&rtc_value);
}

static void adjust_date(int8_t delta) {
    int value;
    switch (selected_field) {
        case 0:
            value = (int)rtc_value.year + delta;
            if (value < 2000) value = 2099;
            if (value > 2099) value = 2000;
            rtc_value.year = (uint16_t)value;
            break;
        case 1:
            value = (int)rtc_value.month + delta;
            if (value < 1) value = 12;
            if (value > 12) value = 1;
            rtc_value.month = (uint8_t)value;
            break;
        case 2:
            value = (int)rtc_value.day + delta;
            if (value < 1) value = mia_host_rtc_days_in_month(rtc_value.year, rtc_value.month);
            if (value > mia_host_rtc_days_in_month(rtc_value.year, rtc_value.month)) value = 1;
            rtc_value.day = (uint8_t)value;
            break;
        case 3:
            value = (int)rtc_value.hour + delta;
            if (value < 0) value = 23;
            if (value > 23) value = 0;
            rtc_value.hour = (uint8_t)value;
            break;
        case 4:
            value = (int)rtc_value.minute + delta;
            if (value < 0) value = 59;
            if (value > 59) value = 0;
            rtc_value.minute = (uint8_t)value;
            break;
        default:
            value = (int)rtc_value.second + delta;
            if (value < 0) value = 59;
            if (value > 59) value = 0;
            rtc_value.second = (uint8_t)value;
            break;
    }
    clamp_date();
    status_kind = STATUS_NONE;
}

static void change_main_value(int8_t delta) {
    if (selected_row == 0) {
        language = language == 0 ? 1 : 0;
        if (mia_host_language_set(language)) {
            ui_language = language;
            status_kind = STATUS_SAVED;
        } else {
            status_kind = STATUS_FAILED;
        }
    } else if (selected_row == 1) {
        uint8_t count = mia_host_font_count();
        int next = (int)font + delta;
        if (next < 0) next = count - 1;
        if (next >= count) next = 0;
        font = (uint8_t)next;
        status_kind = mia_host_font_set(font) ? STATUS_SAVED : STATUS_FAILED;
    } else if (selected_row == 2) {
        int next = (int)brightness + delta;
        if (next < 10) next = 100;
        if (next > 100) next = 10;
        brightness = (uint8_t)next;
        status_kind = mia_host_brightness_set(brightness) ? STATUS_SAVED : STATUS_FAILED;
    } else if (selected_row == 3) {
        int next = (int)volume + delta;
        if (next < 0) next = 100;
        if (next > 100) next = 0;
        volume = (uint8_t)next;
        status_kind = mia_host_volume_set(volume) ? STATUS_SAVED : STATUS_FAILED;
    } else if (selected_row == 4) {
        key_beep = key_beep == 0 ? 1 : 0;
        status_kind = mia_host_key_beep_set(key_beep) ? STATUS_SAVED : STATUS_FAILED;
    } else if (selected_row == 5) {
        static const uint8_t values[] = {0, 1, 5, 10, 30};
        int index = 0;
        for (int i = 0; i < (int)(sizeof(values) / sizeof(values[0])); ++i) {
            if (values[i] == idle_timeout) index = i;
        }
        index += delta;
        if (index < 0) index = (int)(sizeof(values) / sizeof(values[0])) - 1;
        if (index >= (int)(sizeof(values) / sizeof(values[0]))) index = 0;
        idle_timeout = values[index];
        status_kind = mia_host_idle_timeout_set(idle_timeout) ? STATUS_SAVED : STATUS_FAILED;
    } else {
        screen = SCREEN_DATE_TIME;
        selected_field = 0;
        status_kind = STATUS_NONE;
    }
}

int settings_main_impl(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    if (mia_host_abi_version() != 2) return 1;

    screen = SCREEN_MAIN;
    selected_row = 0;
    selected_field = 0;
    ui_language = mia_host_language();
    language = ui_language;
    font = mia_host_font_get();
    brightness = mia_host_brightness_get();
    volume = mia_host_volume_get();
    idle_timeout = mia_host_idle_timeout_get();
    key_beep = mia_host_key_beep_get();
    status_kind = mia_host_rtc_read(&rtc_value) ? STATUS_NONE : STATUS_RTC_FAILED;
    draw_main();

    while (mia_host_button_down(MIA_HOST_BUTTON_UP) ||
           mia_host_button_down(MIA_HOST_BUTTON_DOWN) ||
           mia_host_button_down(MIA_HOST_BUTTON_LEFT) ||
           mia_host_button_down(MIA_HOST_BUTTON_RIGHT) ||
           mia_host_button_down(MIA_HOST_BUTTON_A) ||
           mia_host_button_down(MIA_HOST_BUTTON_B)) {
        mia_host_buttons_poll();
        mia_host_delay_ms(10);
    }

    while (1) {
        mia_host_buttons_poll();
        if (exit_pressed()) break;
        uint8_t changed = 0;
        if (screen == SCREEN_MAIN) {
            if (mia_host_button_pressed(MIA_HOST_BUTTON_UP) && selected_row > 0) {
                --selected_row;
                changed = 1;
            }
            if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN) && selected_row + 1 < MAIN_ROWS) {
                ++selected_row;
                changed = 1;
            }
            if (mia_host_button_pressed(MIA_HOST_BUTTON_LEFT)) {
                change_main_value(-1);
                changed = 1;
            }
            if (mia_host_button_pressed(MIA_HOST_BUTTON_RIGHT) ||
                mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
                change_main_value(1);
                changed = 1;
            }
        } else {
            if (mia_host_button_pressed(MIA_HOST_BUTTON_UP) && selected_field > 0) {
                --selected_field;
                changed = 1;
            }
            if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN) && selected_field + 1 < RTC_FIELDS) {
                ++selected_field;
                changed = 1;
            }
            if (mia_host_button_pressed(MIA_HOST_BUTTON_LEFT)) {
                adjust_date(-1);
                changed = 1;
            }
            if (mia_host_button_pressed(MIA_HOST_BUTTON_RIGHT)) {
                adjust_date(1);
                changed = 1;
            }
            if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
                status_kind = mia_host_rtc_write(&rtc_value) ? STATUS_SAVED : STATUS_FAILED;
                changed = 1;
            }
            if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
                screen = SCREEN_MAIN;
                if (!mia_host_rtc_read(&rtc_value)) status_kind = STATUS_RTC_FAILED;
                changed = 1;
            }
        }
        if (changed) {
            if (screen == SCREEN_MAIN) draw_main();
            else draw_date_time();
        }
        mia_host_delay_ms(20);
    }

    mia_host_clear(MIA_HOST_BLACK);
    mia_host_present();
    return 0;
}
