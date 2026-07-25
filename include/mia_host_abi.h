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
void mia_host_fill_screen_rgb565(uint16_t color);
void mia_host_draw_text(int32_t x, int32_t y, const char *text, uint8_t fg,
                         uint8_t bg);
int32_t mia_host_text_height(void);
int32_t mia_host_text_width(const char *text);
static inline int32_t mia_host_text_y_centered(int32_t area_y, int32_t area_height) {
  const int32_t offset = (area_height - mia_host_text_height()) / 2;
  return area_y + (offset > 0 ? offset : 0);
}
void mia_host_present(void);
typedef enum MiaHostResult {
  MIA_HOST_RESULT_OK = 0,
  MIA_HOST_RESULT_INVALID_ARGUMENT = -1,
  MIA_HOST_RESULT_NOT_READY = -2,
  MIA_HOST_RESULT_IO = -3,
} MiaHostResult;
int32_t mia_host_present_rgb565(const uint16_t *pixels, uint32_t width,
                                uint32_t height, uint32_t pitch_bytes);
void mia_host_buttons_poll(void);
uint8_t mia_host_button_down(uint8_t button);
uint8_t mia_host_button_pressed(uint8_t button);
uint8_t mia_host_button_released(uint8_t button);
void mia_host_delay_ms(uint32_t ms);
uint32_t mia_host_millis(void);
uint8_t mia_host_language(void);
uint8_t mia_host_language_set(uint8_t language);
uint8_t mia_host_font_get(void);
uint8_t mia_host_font_set(uint8_t font);
uint8_t mia_host_font_count(void);
const char *mia_host_font_name(uint8_t font);
void mia_host_backlight_set(uint8_t enabled);
uint8_t mia_host_brightness_get(void);
uint8_t mia_host_brightness_set(uint8_t brightness);
uint8_t mia_host_volume_get(void);
uint8_t mia_host_volume_set(uint8_t volume);
uint8_t mia_host_key_beep_get(void);
uint8_t mia_host_key_beep_set(uint8_t enabled);

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
#ifndef MIA_HOST_DIRENT_NAME_SIZE
#define MIA_HOST_DIRENT_NAME_SIZE 64
#endif
  char name[MIA_HOST_DIRENT_NAME_SIZE];
  uint8_t is_dir;
  uint32_t size;
} MiaHostDirEntry;

int32_t mia_host_sd_list_dir(const char *path, MiaHostDirEntry *entries,
                             uint32_t capacity);
int32_t mia_host_sd_remove(const char *path);

typedef struct MiaHostSystemInfo {
  char chip_model[24];
  uint8_t chip_revision;
  uint16_t cpu_mhz;
  uint32_t free_heap_kb;
  uint32_t flash_mb;
  uint8_t tft_ready;
  uint8_t sd_ready;
} MiaHostSystemInfo;

typedef struct MiaHostBatteryInfo {
  int32_t raw;
  uint32_t millivolts;
} MiaHostBatteryInfo;

uint8_t mia_host_get_system_info(MiaHostSystemInfo *info);
uint8_t mia_host_read_battery(MiaHostBatteryInfo *info);

typedef struct MiaHostWifiNetwork {
  char ssid[33];
  int32_t rssi;
} MiaHostWifiNetwork;

int32_t mia_host_wifi_scan(MiaHostWifiNetwork *networks, uint32_t capacity);
void mia_host_wifi_off(void);

typedef struct MiaHostWifiFilesStatus {
  uint8_t running;
  uint8_t ap_mode;
  char status[32];
  char ssid[32];
  char ip[16];
} MiaHostWifiFilesStatus;

uint8_t mia_host_wifi_files_start(void);
void mia_host_wifi_files_poll(void);
void mia_host_wifi_files_stop(void);
uint8_t mia_host_wifi_files_get_status(MiaHostWifiFilesStatus *status);

typedef struct MiaHostFtpStatus {
  uint8_t running;
  char status[32];
  char ssid[32];
  char ip[16];
  char user[16];
  char pass[16];
} MiaHostFtpStatus;

uint8_t mia_host_ftp_start(void);
void mia_host_ftp_poll(void);
void mia_host_ftp_stop(void);
uint8_t mia_host_ftp_get_status(MiaHostFtpStatus *status);

typedef struct MiaHostAudioStatus {
  uint8_t open;
  uint32_t sample_rate;
  uint8_t channels;
  uint8_t bits_per_sample;
  int32_t last_error;
} MiaHostAudioStatus;

uint8_t mia_host_audio_open(uint32_t sample_rate, uint8_t channels,
                            uint8_t bits_per_sample);
int32_t mia_host_audio_write_pcm16(const int16_t *samples, uint32_t frame_count,
                                   uint8_t channels);
void mia_host_audio_stop(void);
void mia_host_audio_close(void);
uint8_t mia_host_audio_get_status(MiaHostAudioStatus *status);

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
