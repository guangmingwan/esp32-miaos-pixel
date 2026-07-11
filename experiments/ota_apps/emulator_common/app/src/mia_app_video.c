#include "mia_app_video.h"

MiaHardwareStatus mia_app_video_submit(MiaAppVideoSink *sink, const MiaHardwareTarget *target, const uint16_t *pixels, size_t pixel_count, MiaDisplayRect *out_rect) {
    if (sink == NULL || target == NULL || pixels == NULL) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid app video submit");
    }
    const size_t expected = (size_t)target->geometry.width * target->geometry.height;
    if (pixel_count != expected) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "video frame size does not match target");
    }
    const MiaRgb565Surface surface = {target->geometry.width, target->geometry.height, target->geometry.width, pixels};
    return mia_display_render_rgb565(&surface, sink->scale_mode, sink->display_pixels, sink->display_pixel_count, out_rect);
}

MiaHardwareStatus mia_app_video_submit_to_host(MiaAppVideoSink *sink,
                                               const MiaHardwareTarget *target,
                                               const uint16_t *pixels, size_t pixel_count,
                                               MiaAppVideoPresentFn present, void *context) {
    if (present == NULL) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "missing video presenter");
    }
    MiaDisplayRect rect;
    const MiaHardwareStatus status =
        mia_app_video_submit(sink, target, pixels, pixel_count, &rect);
    if (status.code != MIA_HARDWARE_OK) {
        return status;
    }
    if (present(sink->display_pixels, MIA_DISPLAY_WIDTH, MIA_DISPLAY_HEIGHT,
                MIA_DISPLAY_WIDTH * sizeof(uint16_t), context) != 0) {
        return mia_hardware_error(MIA_HARDWARE_ERR_IO, "video presentation failed");
    }
    return mia_hardware_ok();
}
