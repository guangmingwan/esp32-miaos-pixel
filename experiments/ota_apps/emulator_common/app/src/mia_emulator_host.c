#include "mia_emulator_runtime.h"
#include "mia_host_abi.h"

uint32_t mia_emulator_host_buttons(void) {
    mia_host_buttons_poll();
    uint32_t bits = 0;
    for (uint8_t key = MIA_HOST_BUTTON_BOOT; key <= MIA_HOST_BUTTON_RIGHT; ++key) {
        bits |= mia_host_button_down(key) ? (1u << key) : 0u;
    }
    return bits;
}

static uint32_t read_input(void *context) {
    MiaEmulatorRuntime *runtime = context;
    return mia_app_input_core_mask(&runtime->hardware_target, mia_emulator_host_buttons());
}

static int32_t present_rgb565(const uint16_t *pixels, uint32_t width, uint32_t height, uint32_t pitch, void *context) {
    (void)context;
    return mia_host_present_rgb565(pixels, width, height, pitch);
}

static MiaCoreStatus submit_video(void *context, const uint16_t *pixels, size_t count) {
    MiaEmulatorRuntime *runtime = context;
    MiaHardwareStatus status = mia_app_video_submit_to_host(&runtime->video, &runtime->hardware_target, pixels, count, present_rgb565, NULL);
    return status.code == MIA_HARDWARE_OK ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, status.message);
}

static int32_t write_audio(const int16_t *frames, size_t count, void *context) {
    (void)context;
    const int32_t written = mia_host_audio_write_pcm16(frames, (uint32_t)count, 2);
    return written < 0 ? written : (int32_t)count;
}

static MiaCoreStatus submit_audio(void *context, const int16_t *frames, size_t count) {
    MiaEmulatorRuntime *runtime = context;
    MiaHardwareStatus status = mia_app_audio_deliver(&runtime->audio, frames, count, runtime->audio_drain, 512u, write_audio, NULL);
    return status.code == MIA_HARDWARE_OK ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, status.message);
}

static MiaCoreStatus flush_save(void *context, const char *path, const uint8_t *data, size_t size) {
    (void)path;
    MiaEmulatorRuntime *runtime = context;
    MiaStorageStatus status = mia_app_save_flush(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, MIA_STORAGE_FLUSH_CORE_REQUEST, data, size, NULL);
    return status.code == MIA_STORAGE_OK ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, status.message);
}

static MiaCoreStatus clean_exit(void *context) {
    MiaEmulatorRuntime *runtime = context;
    MiaCoreStatus status = mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CLEAN_EXIT, true);
    if (status.code == MIA_CORE_OK) mia_host_audio_close();
    return status;
}

MiaCoreHost mia_emulator_make_host(MiaEmulatorRuntime *runtime) {
    return (MiaCoreHost){runtime, read_input, submit_video, submit_audio, flush_save, clean_exit};
}
