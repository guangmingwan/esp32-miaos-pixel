#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t mia_host_abi_version(void);
void mia_host_log(const char *message);
int32_t mia_host_screen_width(void);
int32_t mia_host_screen_height(void);
void mia_host_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color);
void mia_host_draw_text(int32_t x, int32_t y, const char *text, uint8_t fg,
                        uint8_t bg);
void mia_host_present(void);

#ifdef __cplusplus
}
#endif
