#pragma once

#include "mia_hardware_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    MiaAudioQueue queue;
} MiaAppAudioSink;

typedef int32_t (*MiaAppAudioWriteFn)(const int16_t *frames, size_t frame_count, void *context);

MiaHardwareStatus mia_app_audio_init(MiaAppAudioSink *sink, int16_t *storage, size_t capacity_frames);
MiaHardwareStatus mia_app_audio_submit(MiaAppAudioSink *sink, const int16_t *frames, size_t frame_count, MiaAudioTransfer *out_transfer);
MiaHardwareStatus mia_app_audio_drain(MiaAppAudioSink *sink, int16_t *out_frames, size_t frame_count, MiaAudioTransfer *out_transfer);
MiaHardwareStatus mia_app_audio_deliver(MiaAppAudioSink *sink, const int16_t *frames, size_t frame_count, int16_t *scratch, size_t scratch_capacity, MiaAppAudioWriteFn write_frames, void *context);

#ifdef __cplusplus
}
#endif
