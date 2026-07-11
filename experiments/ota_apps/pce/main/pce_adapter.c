#include "mia_emulator_runtime.h"
#include "mia_host_abi.h"
#include "pce-go.h"
#include "psg.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static uint8_t indexed[XBUF_WIDTH * XBUF_HEIGHT];
static uint16_t output[256 * 224];
static uint16_t palette[256];
static bool exit_requested;

uint8_t *osd_gfx_framebuffer(int width, int height) {
    (void)width;
    (void)height;
    return indexed;
}

void osd_input_read(uint8_t joypads[8]) {
    uint32_t input = 0;
    (void)mia_core_adapter_poll_input(&mia_emulator_runtime.adapter, &input);
    uint8_t buttons = 0;
    if (input & MIA_APP_CORE_INPUT_A) buttons |= JOY_A;
    if (input & MIA_APP_CORE_INPUT_B) buttons |= JOY_B;
    if (input & MIA_APP_CORE_INPUT_UP) buttons |= JOY_UP;
    if (input & MIA_APP_CORE_INPUT_DOWN) buttons |= JOY_DOWN;
    if (input & MIA_APP_CORE_INPUT_LEFT) buttons |= JOY_LEFT;
    if (input & MIA_APP_CORE_INPUT_RIGHT) buttons |= JOY_RIGHT;
    if (input & MIA_APP_CORE_INPUT_START) buttons |= JOY_RUN;
    if (input & MIA_APP_CORE_INPUT_SELECT) buttons |= JOY_SELECT;
    joypads[0] = buttons;
    if (mia_app_input_exit_requested(&mia_emulator_runtime.input, mia_emulator_host_buttons(), mia_host_millis())) {
        exit_requested = true;
        ShutdownPCE();
    }
}

void osd_vsync(void) {
    for (size_t y = 0; y < 224; ++y) {
        for (size_t x = 0; x < 256; ++x) output[y * 256 + x] = palette[indexed[(y + 9) * XBUF_WIDTH + x + 48]];
    }
    (void)mia_core_adapter_submit_video(&mia_emulator_runtime.adapter, output, 256u * 224u);
    int16_t audio[368 * 2];
    psg_update(audio, 368, 0xff);
    (void)mia_core_adapter_submit_audio(&mia_emulator_runtime.adapter, audio, 368);
}

MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime) {
    if (InitPCE(22050, true) != 0) return mia_core_error(MIA_CORE_ERR_CALLBACK, "PCE core init failed");
    if (LoadFile(runtime->selection.rom_path) != 0) return mia_core_error(MIA_CORE_ERR_CALLBACK, "PCE ROM corrupt or unsupported");
    uint16_t *source = PalettePCE(16);
    if (source == NULL) return mia_core_error(MIA_CORE_ERR_CALLBACK, "PCE palette allocation failed");
    memcpy(palette, source, sizeof(palette));
    free(source);
    ResetPCE(true);
    return mia_core_ok();
}

MiaCoreStatus mia_emulator_core_flush(MiaEmulatorRuntime *runtime, MiaStorageFlushReason reason, bool force) {
    (void)force;
    if (SaveState(runtime->selection.save_name) != 0) return mia_core_error(MIA_CORE_ERR_CALLBACK, "PCE save failed");
    return reason == MIA_STORAGE_FLUSH_CLEAN_EXIT ? mia_core_ok() : mia_core_ok();
}

MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime) {
    exit_requested = false;
    RunPCE();
    if (!exit_requested) return mia_core_error(MIA_CORE_ERR_CALLBACK, "PCE core stopped unexpectedly");
    return mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CLEAN_EXIT, true);
}
