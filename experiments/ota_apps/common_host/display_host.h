#pragma once

#include <stdint.h>
#include <stddef.h>

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
int32_t display_host_present_rgb565(const uint16_t *pixels, uint32_t width,
                                    uint32_t height, uint32_t pitch_bytes);
int32_t display_host_present_rgb565_region(const uint16_t *pixels, int32_t x, int32_t y,
                                           uint32_t width, uint32_t height,
                                           uint32_t pitch_bytes);
void display_host_backlight_set(uint8_t enabled);

#ifdef MIA_DISPLAY_HOST_NATIVE_TEST
typedef int32_t (*DisplayHostTestChunkWriter)(uint32_t y, uint32_t rows,
                                              const uint16_t *wire_pixels,
                                              size_t pixel_count, void *context);
int32_t display_host_test_transport_rgb565(const uint16_t *pixels, uint32_t width,
                                           uint32_t height, uint32_t pitch_bytes,
                                           uint8_t ready, uint16_t *staging,
                                           uint32_t staging_rows,
                                           DisplayHostTestChunkWriter writer, void *context);
#endif

#ifdef __cplusplus
}
#endif
