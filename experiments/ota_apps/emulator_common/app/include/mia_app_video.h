#pragma once

#include <stddef.h>
#include <stdint.h>

#include "mia_hardware_display.h"
#include "mia_hardware_target.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t *display_pixels;
    size_t display_pixel_count;
    MiaDisplayScaleMode scale_mode;
} MiaAppVideoSink;

typedef int32_t (*MiaAppVideoPresentFn)(const uint16_t *pixels, uint32_t width, uint32_t height,
                                       uint32_t pitch_bytes, void *context);

MiaHardwareStatus mia_app_video_submit(MiaAppVideoSink *sink, const MiaHardwareTarget *target, const uint16_t *pixels, size_t pixel_count, MiaDisplayRect *out_rect);
MiaHardwareStatus mia_app_video_submit_to_host(MiaAppVideoSink *sink,
                                               const MiaHardwareTarget *target,
                                               const uint16_t *pixels, size_t pixel_count,
                                               MiaAppVideoPresentFn present, void *context);

#ifdef __cplusplus
}
#endif
