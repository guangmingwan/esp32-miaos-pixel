#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t mia_host_abi_version(void);
void mia_host_log(const char *message);
int32_t mia_host_screen_width(void);
int32_t mia_host_screen_height(void);
void mia_host_clear(uint8_t color);
void mia_host_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color);
void mia_host_draw_text(int32_t x, int32_t y, const char *text, uint8_t fg,
                        uint8_t bg);
void mia_host_present(void);
uint8_t mia_host_button_down(uint8_t button);
void mia_host_delay_ms(uint32_t ms);
uint32_t mia_host_millis(void);

typedef struct MiaHostDateTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t weekday;
} MiaHostDateTime;

uint8_t mia_host_rtc_read(MiaHostDateTime *date_time);
uint8_t mia_host_rtc_write(const MiaHostDateTime *date_time);
uint8_t mia_host_rtc_days_in_month(uint16_t year, uint8_t month);
uint8_t mia_host_rtc_day_of_week(const MiaHostDateTime *date_time);

typedef struct MiaHostDirEntry {
  char name[64];
  uint8_t is_dir;
  uint32_t size;
} MiaHostDirEntry;

int32_t mia_host_sd_list_dir(const char *path, MiaHostDirEntry *entries,
                             uint32_t capacity);

enum MiaHostPalette {
  MIA_HOST_BLACK = 0,
  MIA_HOST_WHITE = 1,
  MIA_HOST_BLUE = 2,
  MIA_HOST_GREEN = 3,
  MIA_HOST_RED = 4,
  MIA_HOST_YELLOW = 5,
  MIA_HOST_CYAN = 6,
  MIA_HOST_GRAY = 7,
  MIA_HOST_DARK_BLUE = 8,
};

enum MiaHostButton {
  MIA_HOST_BUTTON_BOOT = 0,
  MIA_HOST_BUTTON_START = 1,
  MIA_HOST_BUTTON_M = 2,
  MIA_HOST_BUTTON_L = 3,
  MIA_HOST_BUTTON_R = 4,
  MIA_HOST_BUTTON_SELECT = 5,
  MIA_HOST_BUTTON_A = 6,
  MIA_HOST_BUTTON_B = 7,
  MIA_HOST_BUTTON_X = 8,
  MIA_HOST_BUTTON_Y = 9,
  MIA_HOST_BUTTON_UP = 10,
  MIA_HOST_BUTTON_DOWN = 11,
  MIA_HOST_BUTTON_LEFT = 12,
  MIA_HOST_BUTTON_RIGHT = 13,
};

#ifdef __cplusplus
}
#endif
