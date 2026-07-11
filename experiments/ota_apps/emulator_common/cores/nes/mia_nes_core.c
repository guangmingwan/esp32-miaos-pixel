#include "mia_emulator_runtime.h"
#include "mia_nes_contract.h"
#include "mia_host_abi.h"
#include "nofrendo.h"

#include <stdio.h>
#include <stdlib.h>

static nes_t *core;
static uint16_t palette[256];
static uint16_t frame[256u * 240u];

static void blit(uint8_t *pixels) {
    if (pixels == NULL) return;
    for (size_t y = 0; y < 240u; ++y) {
        for (size_t x = 0; x < 256u; ++x) frame[y * 256u + x] = palette[pixels[y * NES_SCREEN_PITCH + x + NES_SCREEN_OVERDRAW]];
    }
    (void)mia_core_adapter_submit_video(&mia_emulator_runtime.adapter, frame, 256u * 240u);
}

static MiaCoreStatus preflight(const char *path) {
    if (mia_nes_validate_extension(path) != MIA_NES_OK) return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "unsupported NES file");
    FILE *file = fopen(path, "rb");
    if (file == NULL) return mia_core_error(MIA_CORE_ERR_CALLBACK, "NES ROM open failed");
    uint8_t header[16];
    const size_t read = fread(header, 1, sizeof(header), file);
    fclose(file);
    FILE *bios = fopen("/sd/bios/fds_bios.bin", "rb");
    int bios_valid = 0;
    if (bios != NULL) {
        bios_valid = fseek(bios, 0, SEEK_END) == 0 && ftell(bios) == 8192;
        fclose(bios);
    }
    const MiaNesResult result = mia_nes_validate_image(header, read, bios_valid);
    if (result == MIA_NES_UNSUPPORTED_MAPPER) return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "unsupported NES mapper");
    if (result == MIA_NES_FDS_BIOS_REQUIRED) return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "valid 8192-byte FDS BIOS required");
    return result == MIA_NES_OK ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "invalid NES header");
}

static MiaCoreStatus load_sram(MiaEmulatorRuntime *runtime) {
    if (!core->cart->battery || core->cart->prg_ram_banks < 1) return mia_core_ok();
    const size_t capacity = (size_t)core->cart->prg_ram_banks * ROM_PRG_BANK_SIZE;
    size_t size = 0;
    MiaStorageStatus status = mia_app_save_load(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, core->cart->prg_ram, capacity, &size);
    if (status.code == MIA_STORAGE_ERR_MISSING_REQUIRED_FILE) return mia_core_ok();
    return status.code == MIA_STORAGE_OK && size == capacity ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, "NES SRAM load failed");
}

MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime) {
    MiaCoreStatus status = preflight(runtime->selection.rom_path);
    if (status.code != MIA_CORE_OK) return status;
    core = nes_init(SYS_DETECT, MIA_EMULATOR_SAMPLE_RATE, true, "/sd/bios/fds_bios.bin");
    if (core == NULL) return mia_core_error(MIA_CORE_ERR_CALLBACK, "Nofrendo init failed");
    const int loaded = nes_loadfile(runtime->selection.rom_path);
    if (loaded != 0) return mia_core_error(MIA_CORE_ERR_CALLBACK, loaded == -2 ? "unsupported NES mapper" : "Nofrendo ROM load failed");
    uint16_t *built = nofrendo_buildpalette(NES_PALETTE_PVM, 16);
    if (built == NULL) return mia_core_error(MIA_CORE_ERR_CALLBACK, "NES palette allocation failed");
    for (size_t i = 0; i < 256u; ++i) palette[i] = built[i];
    free(built);
    nes_setvidbuf(malloc(NES_SCREEN_PITCH * NES_SCREEN_HEIGHT));
    core->blit_func = blit;
    return load_sram(runtime);
}

MiaCoreStatus mia_emulator_core_flush(MiaEmulatorRuntime *runtime, MiaStorageFlushReason reason, bool force) {
    (void)force;
    if (core == NULL || !core->cart->battery || core->cart->prg_ram_banks < 1) return mia_core_ok();
    const size_t size = (size_t)core->cart->prg_ram_banks * ROM_PRG_BANK_SIZE;
    MiaStorageStatus status = mia_app_save_flush(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, reason, core->cart->prg_ram, size, NULL);
    return status.code == MIA_STORAGE_OK ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, status.message);
}

MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime) {
    unsigned frames = 0;
    for (;;) {
        uint32_t value = 0;
        MiaCoreStatus status = mia_core_adapter_poll_input(&runtime->adapter, &value);
        if (status.code != MIA_CORE_OK) return status;
        const uint32_t host = mia_emulator_host_buttons();
        if (mia_app_input_exit_requested(&runtime->input, host, mia_host_millis())) return mia_core_ok();
        int pad = 0;
        if (value & MIA_APP_CORE_INPUT_A) pad |= NES_PAD_A;
        if (value & MIA_APP_CORE_INPUT_B) pad |= NES_PAD_B;
        if (value & MIA_APP_CORE_INPUT_SELECT) pad |= NES_PAD_SELECT;
        if (value & MIA_APP_CORE_INPUT_START) pad |= NES_PAD_START;
        if (value & MIA_APP_CORE_INPUT_UP) pad |= NES_PAD_UP;
        if (value & MIA_APP_CORE_INPUT_DOWN) pad |= NES_PAD_DOWN;
        if (value & MIA_APP_CORE_INPUT_LEFT) pad |= NES_PAD_LEFT;
        if (value & MIA_APP_CORE_INPUT_RIGHT) pad |= NES_PAD_RIGHT;
        input_update(0, pad);
        nes_emulate(true);
        status = mia_core_adapter_submit_audio(&runtime->adapter, core->apu->buffer, (size_t)core->apu->samples_per_frame);
        if (status.code != MIA_CORE_OK) return status;
        if (++frames % 60u == 0u) {
            status = mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CORE_REQUEST, false);
            if (status.code != MIA_CORE_OK) return status;
        }
        mia_host_delay_ms(core->refresh_rate == 50 ? 20 : 16);
    }
}
