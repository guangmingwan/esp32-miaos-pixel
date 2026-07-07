#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int display_host_init(void);
int display_host_ready(void);
int32_t display_host_width(void);
int32_t display_host_height(void);
void display_host_clear(uint8_t color);
void display_host_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color);
void display_host_fill_screen_rgb565(uint16_t color);
void display_host_draw_text(int32_t x, int32_t y, const char *text, uint8_t fg, uint8_t bg);
void display_host_present(void);
void display_host_backlight_set(uint8_t enabled);

#ifdef __cplusplus
}
#endif
