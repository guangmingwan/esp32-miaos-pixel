#include "mia_emulator_runtime.h"
#include "gw_system.h"
#include "mia_host_abi.h"

#include <stdio.h>
#include <stdlib.h>

unsigned char *ROM_DATA;
unsigned int ROM_DATA_LENGTH;
static uint32_t gw_input;

unsigned int gw_get_buttons(void) { return gw_input; }

static MiaCoreStatus read_rom(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GW ROM open failed");
    ROM_DATA = malloc(400000u);
    if (ROM_DATA == NULL) { fclose(file); return mia_core_error(MIA_CORE_ERR_CALLBACK, "GW ROM allocation failed"); }
    ROM_DATA_LENGTH = (unsigned int)fread(ROM_DATA, 1, 400000u, file);
    fclose(file);
    return ROM_DATA_LENGTH > 0 ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, "GW ROM read failed");
}

static MiaCoreStatus load_save(MiaEmulatorRuntime *runtime) {
    gw_state_t state;
    size_t size = 0;
    MiaStorageStatus status = mia_app_save_load(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, (uint8_t *)&state, sizeof(state), &size);
    if (status.code == MIA_STORAGE_ERR_MISSING_REQUIRED_FILE) return mia_core_ok();
    return status.code == MIA_STORAGE_OK && size == sizeof(state) && gw_state_load(&state) ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, "GW state load failed");
}

MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime) {
    MiaCoreStatus status = read_rom(runtime->selection.rom_path);
    if (status.code != MIA_CORE_OK) return status;
    if (!gw_system_romload() || !gw_system_config()) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GW core init failed");
    gw_system_sound_init();
    gw_system_start();
    gw_system_reset();
    return load_save(runtime);
}

MiaCoreStatus mia_emulator_core_flush(MiaEmulatorRuntime *runtime, MiaStorageFlushReason reason, bool force) {
    (void)force;
    gw_state_t state;
    if (!gw_state_save(&state)) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GW state export failed");
    MiaStorageStatus status = mia_app_save_flush(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, reason, (const uint8_t *)&state, sizeof(state), NULL);
    return status.code == MIA_STORAGE_OK ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, status.message);
}

MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime) {
    static uint16_t *framebuffer;
    static int16_t audio[GW_AUDIO_BUFFER_LENGTH * 2];
    if (framebuffer == NULL) framebuffer = malloc(320u * 240u * sizeof(uint16_t));
    if (framebuffer == NULL) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GW framebuffer allocation failed");
    unsigned frames = 0;
    for (;;) {
        uint32_t input = 0;
        MiaCoreStatus status = mia_core_adapter_poll_input(&runtime->adapter, &input);
        if (status.code != MIA_CORE_OK) return status;
        if (mia_app_input_exit_requested(&runtime->input, mia_emulator_host_buttons(), mia_host_millis())) return mia_core_ok();
        gw_input = mia_app_input_gw_mask(input);
        (void)gw_system_run(GW_SYSTEM_CYCLES);
        gw_system_blit(framebuffer);
        status = mia_core_adapter_submit_video(&runtime->adapter, framebuffer, 320u * 240u);
        if (status.code != MIA_CORE_OK) return status;
        for (size_t index = 0; index < GW_AUDIO_BUFFER_LENGTH; ++index) {
            const int16_t sample = (int16_t)((int)gw_audio_buffer[index] << 7);
            audio[index * 2u] = sample;
            audio[index * 2u + 1u] = sample;
        }
        status = mia_core_adapter_submit_audio(&runtime->adapter, audio, GW_AUDIO_BUFFER_LENGTH);
        if (status.code != MIA_CORE_OK) return status;
        gw_audio_buffer_copied = true;
        if (++frames % 128u == 0u) {
            status = mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CORE_REQUEST, false);
            if (status.code != MIA_CORE_OK) return status;
        }
        mia_host_delay_ms(8);
    }
}
