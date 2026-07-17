#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t droid_gbk_draw_text(uint8_t *pixels, int32_t surface_width,
                            int32_t surface_height, int32_t x, int32_t y,
                            const char *text, uint8_t fg, uint8_t bg);

#ifdef __cplusplus
}
#endif
