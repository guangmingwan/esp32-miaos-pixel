#include "mia_emulator_core.h"

#include <string.h>

MiaCoreStatus mia_core_ok(void) {
    return (MiaCoreStatus){MIA_CORE_OK, "ok"};
}

MiaCoreStatus mia_core_error(MiaCoreStatusCode code, const char *message) {
    return (MiaCoreStatus){code, message};
}

MiaCoreStatus mia_core_adapter_init(MiaCoreAdapter *adapter, const MiaRuntimeTarget *target, MiaCoreHost host) {
    if (adapter == NULL || target == NULL) {
        return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "invalid adapter init");
    }
    if (host.read_input == NULL || host.submit_video == NULL || host.submit_audio == NULL || host.flush_save == NULL || host.clean_exit == NULL) {
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "missing core host callback");
    }
    memset(adapter, 0, sizeof(*adapter));
    adapter->target = target;
    adapter->host = host;
    return mia_core_ok();
}

MiaCoreStatus mia_core_adapter_select_rom(MiaCoreAdapter *adapter, const char *rom_path, const char *save_path) {
    if (adapter == NULL || rom_path == NULL || save_path == NULL) {
        return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "invalid rom selection");
    }
    adapter->rom_path = rom_path;
    adapter->save_path = save_path;
    return mia_core_ok();
}

MiaCoreStatus mia_core_adapter_submit_video(MiaCoreAdapter *adapter, const uint16_t *pixels, size_t pixel_count) {
    if (adapter == NULL || pixels == NULL) {
        return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "invalid video frame");
    }
    const MiaCoreStatus status = adapter->host.submit_video(adapter->host.ctx, pixels, pixel_count);
    if (status.code != MIA_CORE_OK) {
        return status;
    }
    adapter->frames_submitted += 1;
    return mia_core_ok();
}

MiaCoreStatus mia_core_adapter_submit_audio(MiaCoreAdapter *adapter, const int16_t *frames, size_t frame_count) {
    if (adapter == NULL || frames == NULL) {
        return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "invalid audio frame");
    }
    const MiaCoreStatus status = adapter->host.submit_audio(adapter->host.ctx, frames, frame_count);
    if (status.code != MIA_CORE_OK) {
        return status;
    }
    adapter->audio_submitted += frame_count;
    return mia_core_ok();
}

MiaCoreStatus mia_core_adapter_poll_input(MiaCoreAdapter *adapter, uint32_t *out_input) {
    if (adapter == NULL || out_input == NULL) {
        return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "invalid input poll");
    }
    adapter->last_input = adapter->host.read_input(adapter->host.ctx);
    *out_input = adapter->last_input;
    return mia_core_ok();
}

MiaCoreStatus mia_core_adapter_flush_save(MiaCoreAdapter *adapter, const uint8_t *data, size_t size) {
    if (adapter == NULL || adapter->save_path == NULL || data == NULL) {
        return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "invalid save flush");
    }
    const MiaCoreStatus status = adapter->host.flush_save(adapter->host.ctx, adapter->save_path, data, size);
    if (status.code != MIA_CORE_OK) {
        return status;
    }
    adapter->saves_flushed += 1;
    return mia_core_ok();
}

MiaCoreStatus mia_core_adapter_request_exit(MiaCoreAdapter *adapter) {
    if (adapter == NULL) {
        return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "invalid exit request");
    }
    const MiaCoreStatus status = adapter->host.clean_exit(adapter->host.ctx);
    if (status.code != MIA_CORE_OK) {
        return status;
    }
    adapter->exit_requested = 1;
    return mia_core_ok();
}
