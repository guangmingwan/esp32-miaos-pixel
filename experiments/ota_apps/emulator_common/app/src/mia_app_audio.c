#include "mia_app_audio.h"

MiaHardwareStatus mia_app_audio_init(MiaAppAudioSink *sink, int16_t *storage, size_t capacity_frames) {
    if (sink == NULL) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid app audio sink");
    }
    return mia_audio_queue_init(&sink->queue, storage, capacity_frames);
}

MiaHardwareStatus mia_app_audio_submit(MiaAppAudioSink *sink, const int16_t *frames, size_t frame_count, MiaAudioTransfer *out_transfer) {
    if (sink == NULL) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid app audio submit");
    }
    return mia_audio_queue_push(&sink->queue, frames, frame_count, out_transfer);
}

MiaHardwareStatus mia_app_audio_drain(MiaAppAudioSink *sink, int16_t *out_frames, size_t frame_count, MiaAudioTransfer *out_transfer) {
    if (sink == NULL) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid app audio drain");
    }
    return mia_audio_queue_pop(&sink->queue, out_frames, frame_count, out_transfer);
}

MiaHardwareStatus mia_app_audio_deliver(MiaAppAudioSink *sink, const int16_t *frames, size_t frame_count, int16_t *scratch, size_t scratch_capacity, MiaAppAudioWriteFn write_frames, void *context) {
    if (sink == NULL || frames == NULL || scratch == NULL || scratch_capacity == 0 || write_frames == NULL) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid app audio delivery");
    }
    MiaAudioTransfer pushed;
    MiaHardwareStatus push_status = mia_app_audio_submit(sink, frames, frame_count, &pushed);
    size_t remaining = pushed.accepted_frames;
    while (remaining > 0) {
        const size_t chunk = remaining < scratch_capacity ? remaining : scratch_capacity;
        MiaAudioTransfer drained;
        MiaHardwareStatus status = mia_app_audio_drain(sink, scratch, chunk, &drained);
        if (status.code != MIA_HARDWARE_OK || drained.accepted_frames != chunk) {
            return mia_hardware_error(MIA_HARDWARE_ERR_IO, "audio queue drain failed");
        }
        if (write_frames(scratch, chunk, context) != (int32_t)chunk) {
            return mia_hardware_error(MIA_HARDWARE_ERR_IO, "audio write failed");
        }
        remaining -= chunk;
    }
    return push_status;
}
