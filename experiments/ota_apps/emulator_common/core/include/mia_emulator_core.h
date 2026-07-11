#pragma once

#include <stddef.h>
#include <stdint.h>

#include "mia_runtime_target.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MIA_CORE_OK = 0,
    MIA_CORE_ERR_INVALID_ARGUMENT,
    MIA_CORE_ERR_CALLBACK,
} MiaCoreStatusCode;

typedef struct {
    MiaCoreStatusCode code;
    const char *message;
} MiaCoreStatus;

typedef uint32_t (*MiaCoreInputFn)(void *ctx);
typedef MiaCoreStatus (*MiaCoreVideoFn)(void *ctx, const uint16_t *pixels, size_t pixel_count);
typedef MiaCoreStatus (*MiaCoreAudioFn)(void *ctx, const int16_t *frames, size_t frame_count);
typedef MiaCoreStatus (*MiaCoreSaveFn)(void *ctx, const char *path, const uint8_t *data, size_t size);
typedef MiaCoreStatus (*MiaCoreExitFn)(void *ctx);

typedef struct {
    void *ctx;
    MiaCoreInputFn read_input;
    MiaCoreVideoFn submit_video;
    MiaCoreAudioFn submit_audio;
    MiaCoreSaveFn flush_save;
    MiaCoreExitFn clean_exit;
} MiaCoreHost;

typedef struct {
    const MiaRuntimeTarget *target;
    MiaCoreHost host;
    const char *rom_path;
    const char *save_path;
    uint32_t last_input;
    uint32_t frames_submitted;
    uint32_t audio_submitted;
    uint32_t saves_flushed;
    uint8_t exit_requested;
} MiaCoreAdapter;

MiaCoreStatus mia_core_ok(void);
MiaCoreStatus mia_core_error(MiaCoreStatusCode code, const char *message);
MiaCoreStatus mia_core_adapter_init(MiaCoreAdapter *adapter, const MiaRuntimeTarget *target, MiaCoreHost host);
MiaCoreStatus mia_core_adapter_select_rom(MiaCoreAdapter *adapter, const char *rom_path, const char *save_path);
MiaCoreStatus mia_core_adapter_submit_video(MiaCoreAdapter *adapter, const uint16_t *pixels, size_t pixel_count);
MiaCoreStatus mia_core_adapter_submit_audio(MiaCoreAdapter *adapter, const int16_t *frames, size_t frame_count);
MiaCoreStatus mia_core_adapter_poll_input(MiaCoreAdapter *adapter, uint32_t *out_input);
MiaCoreStatus mia_core_adapter_flush_save(MiaCoreAdapter *adapter, const uint8_t *data, size_t size);
MiaCoreStatus mia_core_adapter_request_exit(MiaCoreAdapter *adapter);

#ifdef __cplusplus
}
#endif
