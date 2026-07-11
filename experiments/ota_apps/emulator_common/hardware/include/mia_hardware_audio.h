#pragma once

#include <stddef.h>
#include <stdint.h>

#include "mia_hardware_status.h"
#include "mia_hardware_target.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t sample_rate_hz;
    uint8_t channels;
    uint8_t bits_per_sample;
} MiaAudioConfig;

typedef struct {
    int16_t *frames;
    size_t capacity_frames;
    size_t read_index;
    size_t write_index;
    size_t count_frames;
} MiaAudioQueue;

typedef struct {
    size_t accepted_frames;
    size_t dropped_frames;
} MiaAudioTransfer;

MiaHardwareStatus mia_audio_negotiate(const MiaHardwareTarget *target, MiaAudioConfig *out_config);
MiaHardwareStatus mia_audio_queue_init(MiaAudioQueue *queue, int16_t *stereo_frame_storage, size_t capacity_frames);
MiaHardwareStatus mia_audio_queue_push(MiaAudioQueue *queue, const int16_t *stereo_frames, size_t frame_count, MiaAudioTransfer *out_transfer);
MiaHardwareStatus mia_audio_queue_pop(MiaAudioQueue *queue, int16_t *out_stereo_frames, size_t frame_count, MiaAudioTransfer *out_transfer);

#ifdef __cplusplus
}
#endif
