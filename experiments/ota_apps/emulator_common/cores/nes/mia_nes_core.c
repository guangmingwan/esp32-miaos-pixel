#include "mia_emulator_runtime.h"
#include "mia_nes_contract.h"
#include "mia_host_abi.h"
#include "display_host.h"
#include "nofrendo.h"

#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdlib.h>

#define NES_FRAME_PIXELS (256u * 240u)

static nes_t *core;
static uint16_t palette[256];
static uint16_t *video_frames[2];
static QueueHandle_t free_video_frames;
static QueueHandle_t ready_video_frames;

static void display_task(void *argument) {
    (void)argument;
    for (;;) {
        uint8_t index = 0;
        if (xQueueReceive(ready_video_frames, &index, portMAX_DELAY) != pdTRUE) continue;
        (void)display_host_present_rgb565_region(
            video_frames[index], 32, 0, 256, 240, 256u * sizeof(uint16_t));
        (void)xQueueSend(free_video_frames, &index, portMAX_DELAY);
    }
}

static MiaCoreStatus start_display_task(void) {
    for (uint8_t index = 0; index < 2; ++index) {
        video_frames[index] = heap_caps_malloc(
            NES_FRAME_PIXELS * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (video_frames[index] == NULL) {
            return mia_core_error(MIA_CORE_ERR_CALLBACK, "NES video buffer allocation failed");
        }
    }
    free_video_frames = xQueueCreate(2, sizeof(uint8_t));
    ready_video_frames = xQueueCreate(1, sizeof(uint8_t));
    if (free_video_frames == NULL || ready_video_frames == NULL) {
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "NES video queue allocation failed");
    }
    for (uint8_t index = 0; index < 2; ++index) {
        (void)xQueueSend(free_video_frames, &index, 0);
    }
    if (xTaskCreatePinnedToCore(display_task, "nes_display", 4096, NULL, 4, NULL, 0) != pdPASS) {
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "NES display task allocation failed");
    }
    return mia_core_ok();
}

static void blit(uint8_t *pixels) {
    if (pixels == NULL) return;
    uint8_t frame_index = 0;
    if (xQueueReceive(free_video_frames, &frame_index, 0) != pdTRUE) return;
    uint16_t *frame = video_frames[frame_index];
    for (size_t y = 0; y < 240u; ++y) {
        for (size_t x = 0; x < 256u; ++x) frame[y * 256u + x] = palette[pixels[y * NES_SCREEN_PITCH + x + NES_SCREEN_OVERDRAW]];
    }
    if (xQueueSend(ready_video_frames, &frame_index, 0) != pdTRUE) {
        (void)xQueueSend(free_video_frames, &frame_index, 0);
    }
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
    display_host_fill_screen_rgb565(0);
    status = start_display_task();
    if (status.code != MIA_CORE_OK) return status;
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
        const uint32_t frame_started = mia_host_millis();
        const uint32_t host = mia_emulator_host_buttons();
        if (mia_app_input_exit_requested(&runtime->input, host, mia_host_millis())) return mia_core_ok();
        int pad = 0;
        if (host & (1u << MIA_HOST_BUTTON_A)) pad |= NES_PAD_A;
        if (host & (1u << MIA_HOST_BUTTON_B)) pad |= NES_PAD_B;
        if (host & (1u << MIA_HOST_BUTTON_SELECT)) pad |= NES_PAD_SELECT;
        if (host & (1u << MIA_HOST_BUTTON_START)) pad |= NES_PAD_START;
        if (host & (1u << MIA_HOST_BUTTON_UP)) pad |= NES_PAD_UP;
        if (host & (1u << MIA_HOST_BUTTON_DOWN)) pad |= NES_PAD_DOWN;
        if (host & (1u << MIA_HOST_BUTTON_LEFT)) pad |= NES_PAD_LEFT;
        if (host & (1u << MIA_HOST_BUTTON_RIGHT)) pad |= NES_PAD_RIGHT;
        input_update(0, pad);
        nes_emulate((frames & 1u) == 0u);
        MiaCoreStatus status = mia_core_adapter_submit_audio(
            &runtime->adapter, core->apu->buffer, (size_t)core->apu->samples_per_frame);
        if (status.code != MIA_CORE_OK) return status;
        if (++frames % 60u == 0u) {
            status = mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CORE_REQUEST, false);
            if (status.code != MIA_CORE_OK) return status;
        }
        const uint32_t frame_period = core->refresh_rate == 50 ? 20u : 16u;
        const uint32_t elapsed = mia_host_millis() - frame_started;
        if (elapsed < frame_period) mia_host_delay_ms(frame_period - elapsed);
    }
}
