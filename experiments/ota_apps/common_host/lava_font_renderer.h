#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t lava_font_renderer_draw_text(
    uint8_t *pixels, int32_t surface_width, int32_t surface_height, int32_t x,
    int32_t y, const char *text, uint8_t fg, uint8_t bg, uint8_t font_face);
int32_t lava_font_renderer_text_height(uint8_t font_face);

#ifdef __cplusplus
}
#endif
