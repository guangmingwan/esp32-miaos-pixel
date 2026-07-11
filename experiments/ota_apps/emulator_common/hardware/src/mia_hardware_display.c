#include "mia_hardware_display.h"

#include <string.h>

static bool geometry_valid(MiaHardwareGeometry source) {
    return source.width > 0 && source.height > 0;
}

static uint16_t ceil_div_u16(uint16_t numerator, uint16_t denominator) {
    return (uint16_t)((numerator + denominator - 1u) / denominator);
}

MiaHardwareStatus mia_display_fit_rect(MiaHardwareGeometry source, MiaDisplayScaleMode mode, MiaDisplayRect *out_rect) {
    if (out_rect == NULL || !geometry_valid(source)) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid display geometry");
    }
    uint16_t scale = 0;
    switch (mode) {
    case MIA_DISPLAY_SCALE_FIT:
        scale = (uint16_t)((MIA_DISPLAY_WIDTH / source.width) < (MIA_DISPLAY_HEIGHT / source.height) ? (MIA_DISPLAY_WIDTH / source.width) : (MIA_DISPLAY_HEIGHT / source.height));
        if (scale == 0) {
            return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "source too large for integer fit");
        }
        out_rect->width = (uint16_t)(source.width * scale);
        out_rect->height = (uint16_t)(source.height * scale);
        out_rect->x = (uint16_t)((MIA_DISPLAY_WIDTH - out_rect->width) / 2u);
        out_rect->y = (uint16_t)((MIA_DISPLAY_HEIGHT - out_rect->height) / 2u);
        out_rect->scale = scale;
        return mia_hardware_ok();
    case MIA_DISPLAY_SCALE_CROP:
        scale = ceil_div_u16(MIA_DISPLAY_WIDTH, source.width);
        if (ceil_div_u16(MIA_DISPLAY_HEIGHT, source.height) > scale) {
            scale = ceil_div_u16(MIA_DISPLAY_HEIGHT, source.height);
        }
        out_rect->x = 0;
        out_rect->y = 0;
        out_rect->width = MIA_DISPLAY_WIDTH;
        out_rect->height = MIA_DISPLAY_HEIGHT;
        out_rect->scale = scale;
        return mia_hardware_ok();
    case MIA_DISPLAY_SCALE_STRETCH:
        out_rect->x = 0;
        out_rect->y = 0;
        out_rect->width = MIA_DISPLAY_WIDTH;
        out_rect->height = MIA_DISPLAY_HEIGHT;
        out_rect->scale = 1;
        return mia_hardware_ok();
    }
    return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "unknown scale mode");
}

MiaHardwareStatus mia_display_plan_buffers(MiaHardwareGeometry source, MiaDisplayBufferPolicy policy, MiaDisplayBufferDecision *out_decision) {
    if (out_decision == NULL || !geometry_valid(source)) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid buffer policy");
    }
    if (policy.mode != MIA_DISPLAY_BUFFER_SINGLE && policy.mode != MIA_DISPLAY_BUFFER_DOUBLE) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid buffer count");
    }
    if (policy.memory != MIA_DISPLAY_MEMORY_INTERNAL && policy.memory != MIA_DISPLAY_MEMORY_PSRAM) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid memory policy");
    }
    out_decision->buffer_count = policy.mode == MIA_DISPLAY_BUFFER_DOUBLE ? 2u : 1u;
    out_decision->bytes_per_buffer = (size_t)MIA_DISPLAY_PIXELS * sizeof(uint16_t);
    out_decision->memory = policy.memory;
    return mia_hardware_ok();
}

static MiaHardwareStatus validate_output(uint16_t *out_pixels, size_t out_pixel_count) {
    if (out_pixels == NULL) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "missing output buffer");
    }
    if (out_pixel_count < MIA_DISPLAY_PIXELS) {
        return mia_hardware_error(MIA_HARDWARE_ERR_BUFFER_TOO_SMALL, "output buffer too small");
    }
    return mia_hardware_ok();
}

static void clear_output(uint16_t *out_pixels) {
    memset(out_pixels, 0, (size_t)MIA_DISPLAY_PIXELS * sizeof(uint16_t));
}

static bool fit_source(const MiaDisplayRect *rect, uint16_t dx, uint16_t dy, uint16_t *sx, uint16_t *sy) {
    if (dx < rect->x || dy < rect->y || dx >= (uint16_t)(rect->x + rect->width) || dy >= (uint16_t)(rect->y + rect->height)) {
        return false;
    }
    *sx = (uint16_t)((dx - rect->x) / rect->scale);
    *sy = (uint16_t)((dy - rect->y) / rect->scale);
    return true;
}

static bool crop_source(MiaHardwareGeometry source, uint16_t scale, uint16_t dx, uint16_t dy, uint16_t *sx, uint16_t *sy) {
    const uint16_t scaled_width = (uint16_t)(source.width * scale);
    const uint16_t scaled_height = (uint16_t)(source.height * scale);
    const uint16_t crop_x = (uint16_t)((scaled_width - MIA_DISPLAY_WIDTH) / 2u);
    const uint16_t crop_y = (uint16_t)((scaled_height - MIA_DISPLAY_HEIGHT) / 2u);
    *sx = (uint16_t)((dx + crop_x) / scale);
    *sy = (uint16_t)((dy + crop_y) / scale);
    return *sx < source.width && *sy < source.height;
}

static bool map_source(MiaHardwareGeometry source, MiaDisplayScaleMode mode, const MiaDisplayRect *rect,
                       uint16_t dx, uint16_t dy, uint16_t *sx, uint16_t *sy) {
    switch (mode) {
    case MIA_DISPLAY_SCALE_FIT:
        return fit_source(rect, dx, dy, sx, sy);
    case MIA_DISPLAY_SCALE_CROP:
        return crop_source(source, rect->scale, dx, dy, sx, sy);
    case MIA_DISPLAY_SCALE_STRETCH:
        *sx = (uint16_t)(((uint32_t)dx * source.width) / MIA_DISPLAY_WIDTH);
        *sy = (uint16_t)(((uint32_t)dy * source.height) / MIA_DISPLAY_HEIGHT);
        return true;
    }
    return false;
}

MiaHardwareStatus mia_display_render_paletted(const MiaPalettedSurface *surface, MiaDisplayScaleMode mode, uint16_t *out_pixels, size_t out_pixel_count, MiaDisplayRect *out_rect) {
    MiaHardwareStatus status = validate_output(out_pixels, out_pixel_count);
    if (!mia_hardware_status_ok(status)) {
        return status;
    }
    if (surface == NULL || surface->pixels == NULL || surface->palette_rgb565 == NULL || surface->stride < surface->width) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid paletted surface");
    }
    MiaDisplayRect rect = {0};
    status = mia_display_fit_rect((MiaHardwareGeometry){surface->width, surface->height}, mode, &rect);
    if (!mia_hardware_status_ok(status)) {
        return status;
    }
    clear_output(out_pixels);
    for (uint16_t dy = 0; dy < MIA_DISPLAY_HEIGHT; ++dy) {
        for (uint16_t dx = 0; dx < MIA_DISPLAY_WIDTH; ++dx) {
            uint16_t sx = 0;
            uint16_t sy = 0;
            const bool visible = map_source((MiaHardwareGeometry){surface->width, surface->height}, mode, &rect, dx, dy, &sx, &sy);
            if (visible) {
                const uint8_t index = surface->pixels[(size_t)sy * surface->stride + sx];
                if (index >= surface->palette_count) {
                    return mia_hardware_error(MIA_HARDWARE_ERR_PALETTE_INDEX, "palette index out of range");
                }
                out_pixels[(size_t)dy * MIA_DISPLAY_WIDTH + dx] = surface->palette_rgb565[index];
            }
        }
    }
    if (out_rect != NULL) {
        *out_rect = rect;
    }
    return mia_hardware_ok();
}

MiaHardwareStatus mia_display_render_rgb565(const MiaRgb565Surface *surface, MiaDisplayScaleMode mode, uint16_t *out_pixels, size_t out_pixel_count, MiaDisplayRect *out_rect) {
    MiaHardwareStatus status = validate_output(out_pixels, out_pixel_count);
    if (!mia_hardware_status_ok(status)) {
        return status;
    }
    if (surface == NULL || surface->pixels == NULL || surface->stride < surface->width) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid rgb565 surface");
    }
    MiaDisplayRect rect = {0};
    status = mia_display_fit_rect((MiaHardwareGeometry){surface->width, surface->height}, mode, &rect);
    if (!mia_hardware_status_ok(status)) {
        return status;
    }
    if (mode == MIA_DISPLAY_SCALE_STRETCH) {
        uint16_t source_x[MIA_DISPLAY_WIDTH];
        for (uint16_t dx = 0; dx < MIA_DISPLAY_WIDTH; ++dx) {
            source_x[dx] = (uint16_t)(((uint32_t)dx * surface->width) / MIA_DISPLAY_WIDTH);
        }
        for (uint16_t dy = 0; dy < MIA_DISPLAY_HEIGHT; ++dy) {
            const uint16_t sy = (uint16_t)(((uint32_t)dy * surface->height) / MIA_DISPLAY_HEIGHT);
            const uint16_t *source_row = surface->pixels + (size_t)sy * surface->stride;
            uint16_t *output_row = out_pixels + (size_t)dy * MIA_DISPLAY_WIDTH;
            for (uint16_t dx = 0; dx < MIA_DISPLAY_WIDTH; ++dx) output_row[dx] = source_row[source_x[dx]];
        }
        if (out_rect != NULL) *out_rect = rect;
        return mia_hardware_ok();
    }
    clear_output(out_pixels);
    for (uint16_t dy = 0; dy < MIA_DISPLAY_HEIGHT; ++dy) {
        for (uint16_t dx = 0; dx < MIA_DISPLAY_WIDTH; ++dx) {
            uint16_t sx = 0;
            uint16_t sy = 0;
            const bool visible = map_source((MiaHardwareGeometry){surface->width, surface->height}, mode, &rect, dx, dy, &sx, &sy);
            if (visible) {
                out_pixels[(size_t)dy * MIA_DISPLAY_WIDTH + dx] = surface->pixels[(size_t)sy * surface->stride + sx];
            }
        }
    }
    if (out_rect != NULL) {
        *out_rect = rect;
    }
    return mia_hardware_ok();
}
