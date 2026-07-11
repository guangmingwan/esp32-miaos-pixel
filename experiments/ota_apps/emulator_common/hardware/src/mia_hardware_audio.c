#include "mia_hardware_audio.h"

#include <string.h>

#define MIA_AUDIO_MIN_RATE 8000u
#define MIA_AUDIO_MAX_RATE 48000u

static void reset_transfer(MiaAudioTransfer *transfer) {
    if (transfer != NULL) {
        transfer->accepted_frames = 0;
        transfer->dropped_frames = 0;
    }
}

MiaHardwareStatus mia_audio_negotiate(const MiaHardwareTarget *target, MiaAudioConfig *out_config) {
    if (target == NULL || out_config == NULL) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid audio target");
    }
    if (target->sample_rate_hz < MIA_AUDIO_MIN_RATE || target->sample_rate_hz > MIA_AUDIO_MAX_RATE) {
        return mia_hardware_error(MIA_HARDWARE_ERR_UNSUPPORTED_RATE, "unsupported sample rate");
    }
    out_config->sample_rate_hz = target->sample_rate_hz;
    out_config->channels = 2;
    out_config->bits_per_sample = 16;
    return mia_hardware_ok();
}

MiaHardwareStatus mia_audio_queue_init(MiaAudioQueue *queue, int16_t *stereo_frame_storage, size_t capacity_frames) {
    if (queue == NULL || stereo_frame_storage == NULL || capacity_frames == 0) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid audio queue");
    }
    queue->frames = stereo_frame_storage;
    queue->capacity_frames = capacity_frames;
    queue->read_index = 0;
    queue->write_index = 0;
    queue->count_frames = 0;
    memset(stereo_frame_storage, 0, capacity_frames * 2u * sizeof(int16_t));
    return mia_hardware_ok();
}

MiaHardwareStatus mia_audio_queue_push(MiaAudioQueue *queue, const int16_t *stereo_frames, size_t frame_count, MiaAudioTransfer *out_transfer) {
    reset_transfer(out_transfer);
    if (queue == NULL || queue->frames == NULL || stereo_frames == NULL) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid audio push");
    }
    for (size_t i = 0; i < frame_count; ++i) {
        if (queue->count_frames == queue->capacity_frames) {
            if (out_transfer != NULL) {
                out_transfer->dropped_frames = frame_count - i;
            }
            return mia_hardware_error(MIA_HARDWARE_ERR_QUEUE_FULL, "audio queue full");
        }
        const size_t dst = queue->write_index * 2u;
        const size_t src = i * 2u;
        queue->frames[dst] = stereo_frames[src];
        queue->frames[dst + 1u] = stereo_frames[src + 1u];
        queue->write_index = (queue->write_index + 1u) % queue->capacity_frames;
        queue->count_frames++;
        if (out_transfer != NULL) {
            out_transfer->accepted_frames++;
        }
    }
    return mia_hardware_ok();
}

MiaHardwareStatus mia_audio_queue_pop(MiaAudioQueue *queue, int16_t *out_stereo_frames, size_t frame_count, MiaAudioTransfer *out_transfer) {
    reset_transfer(out_transfer);
    if (queue == NULL || queue->frames == NULL || out_stereo_frames == NULL) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid audio pop");
    }
    for (size_t i = 0; i < frame_count; ++i) {
        const size_t dst = i * 2u;
        if (queue->count_frames == 0) {
            out_stereo_frames[dst] = 0;
            out_stereo_frames[dst + 1u] = 0;
            if (out_transfer != NULL) {
                out_transfer->dropped_frames++;
            }
            continue;
        }
        const size_t src = queue->read_index * 2u;
        out_stereo_frames[dst] = queue->frames[src];
        out_stereo_frames[dst + 1u] = queue->frames[src + 1u];
        queue->read_index = (queue->read_index + 1u) % queue->capacity_frames;
        queue->count_frames--;
        if (out_transfer != NULL) {
            out_transfer->accepted_frames++;
        }
    }
    if (out_transfer != NULL && out_transfer->dropped_frames > 0) {
        return mia_hardware_error(MIA_HARDWARE_ERR_QUEUE_EMPTY, "audio queue underflow");
    }
    return mia_hardware_ok();
}
