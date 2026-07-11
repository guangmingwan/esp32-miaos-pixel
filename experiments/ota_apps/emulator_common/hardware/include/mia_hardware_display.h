#pragma once

#include <stddef.h>
#include <stdint.h>

#include "mia_hardware_status.h"
#include "mia_hardware_target.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIA_DISPLAY_WIDTH 320u
#define MIA_DISPLAY_HEIGHT 240u
#define MIA_DISPLAY_PIXELS (MIA_DISPLAY_WIDTH * MIA_DISPLAY_HEIGHT)

typedef enum {
    MIA_DISPLAY_SCALE_FIT = 1,
    MIA_DISPLAY_SCALE_CROP,
    MIA_DISPLAY_SCALE_STRETCH,
} MiaDisplayScaleMode;

typedef enum {
    MIA_DISPLAY_BUFFER_SINGLE = 1,
    MIA_DISPLAY_BUFFER_DOUBLE,
} MiaDisplayBufferMode;

typedef enum {
    MIA_DISPLAY_MEMORY_INTERNAL = 1,
    MIA_DISPLAY_MEMORY_PSRAM,
} MiaDisplayMemoryPolicy;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint16_t scale;
} MiaDisplayRect;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    const uint8_t *pixels;
    const uint16_t *palette_rgb565;
    size_t palette_count;
} MiaPalettedSurface;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    const uint16_t *pixels;
} MiaRgb565Surface;

typedef struct {
    MiaDisplayBufferMode mode;
    MiaDisplayMemoryPolicy memory;
} MiaDisplayBufferPolicy;

typedef struct {
    uint8_t buffer_count;
    size_t bytes_per_buffer;
    MiaDisplayMemoryPolicy memory;
} MiaDisplayBufferDecision;

MiaHardwareStatus mia_display_fit_rect(MiaHardwareGeometry source, MiaDisplayScaleMode mode, MiaDisplayRect *out_rect);
MiaHardwareStatus mia_display_plan_buffers(MiaHardwareGeometry source, MiaDisplayBufferPolicy policy, MiaDisplayBufferDecision *out_decision);
MiaHardwareStatus mia_display_render_paletted(const MiaPalettedSurface *surface, MiaDisplayScaleMode mode, uint16_t *out_pixels, size_t out_pixel_count, MiaDisplayRect *out_rect);
MiaHardwareStatus mia_display_render_rgb565(const MiaRgb565Surface *surface, MiaDisplayScaleMode mode, uint16_t *out_pixels, size_t out_pixel_count, MiaDisplayRect *out_rect);

#ifdef __cplusplus
}
#endif
