#include "mia_emulator_runtime.h"
#include "mia_snes_contract.h"
#include "mia_host_abi.h"
#include "snes9x.h"

#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <stdlib.h>

static MiaEmulatorRuntime *active_runtime;
static int16_t *mix_buffer;
static MiaSnesPacing pacing;

static uint32_t snes_pad(uint32_t value) {
    uint32_t pad = 0;
    if (value & MIA_APP_CORE_INPUT_A) pad |= SNES_A_MASK;
    if (value & MIA_APP_CORE_INPUT_B) pad |= SNES_B_MASK;
    if (value & MIA_APP_CORE_INPUT_SELECT) pad |= SNES_SELECT_MASK;
    if (value & MIA_APP_CORE_INPUT_START) pad |= SNES_START_MASK;
    if (value & MIA_APP_CORE_INPUT_UP) pad |= SNES_UP_MASK;
    if (value & MIA_APP_CORE_INPUT_DOWN) pad |= SNES_DOWN_MASK;
    if (value & MIA_APP_CORE_INPUT_LEFT) pad |= SNES_LEFT_MASK;
    if (value & MIA_APP_CORE_INPUT_RIGHT) pad |= SNES_RIGHT_MASK;
    return pad;
}

bool S9xInitDisplay(void) {
    GFX.Pitch = SNES_WIDTH * 2;
    GFX.ZPitch = SNES_WIDTH;
    GFX.Screen = heap_caps_malloc((size_t)GFX.Pitch * SNES_HEIGHT_EXTENDED, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    GFX.SubScreen = heap_caps_malloc((size_t)GFX.Pitch * SNES_HEIGHT_EXTENDED, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    GFX.ZBuffer = heap_caps_malloc((size_t)GFX.ZPitch * SNES_HEIGHT_EXTENDED, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    GFX.SubZBuffer = heap_caps_malloc((size_t)GFX.ZPitch * SNES_HEIGHT_EXTENDED, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    return GFX.Screen != NULL && GFX.SubScreen != NULL && GFX.ZBuffer != NULL && GFX.SubZBuffer != NULL;
}

void S9xDeinitDisplay(void) {
    free(GFX.Screen);
    free(GFX.SubScreen);
    free(GFX.ZBuffer);
    free(GFX.SubZBuffer);
}

uint32_t S9xReadJoypad(int32_t port) {
    if (port != 0 || active_runtime == NULL) return 0;
    uint32_t value = 0;
    (void)mia_core_adapter_poll_input(&active_runtime->adapter, &value);
    return snes_pad(value);
}

bool S9xReadMousePosition(int32_t which, int32_t *x, int32_t *y, uint32_t *buttons) {
    (void)which; (void)x; (void)y; (void)buttons;
    return false;
}

bool S9xReadSuperScopePosition(int32_t *x, int32_t *y, uint32_t *buttons) {
    (void)x; (void)y; (void)buttons;
    return false;
}

bool JustifierOffscreen(void) { return true; }
void JustifierButtons(uint32_t *buttons) { (void)buttons; }
void S9xMessage(int type, int number, const char *message) { (void)type; (void)number; if (message != NULL) mia_host_log(message); }
void S9xExit(void) {}

static size_t sram_size(void) {
    if (Memory.SRAMSize == 0) return 0;
    size_t size = (size_t)1u << (Memory.SRAMSize + 3u);
    return size > SRAM_SIZE ? SRAM_SIZE : size;
}

static MiaCoreStatus load_sram(MiaEmulatorRuntime *runtime) {
    const size_t capacity = sram_size();
    if (capacity == 0) return mia_core_ok();
    size_t size = 0;
    MiaStorageStatus status = mia_app_save_load(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, Memory.SRAM, capacity, &size);
    if (status.code == MIA_STORAGE_ERR_MISSING_REQUIRED_FILE) return mia_core_ok();
    return status.code == MIA_STORAGE_OK && size <= capacity ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, "SNES SRAM load failed");
}

MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime) {
    if (!mia_snes_extension_supported(runtime->selection.rom_path)) return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "unsupported SNES file");
    active_runtime = runtime;
    Settings.CyclesPercentage = 100;
    Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
    Settings.FrameTimePAL = 20000;
    Settings.FrameTimeNTSC = 16667;
    Settings.ControllerOption = SNES_JOYPAD;
    Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;
    Settings.SoundPlaybackRate = MIA_EMULATOR_SAMPLE_RATE;
    Settings.SoundInputRate = MIA_EMULATOR_SAMPLE_RATE;
    Settings.DisableSoundEcho = false;
    Settings.InterpolatedSound = true;
    mix_buffer = heap_caps_malloc(2048u * 2u * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (mix_buffer == NULL || !S9xInitDisplay() || !S9xInitMemory() || !S9xInitAPU() || !S9xInitSound(0, 0) || !S9xInitGFX()) return mia_core_error(MIA_CORE_ERR_CALLBACK, "SNES core init failed");
    if (!LoadROM(runtime->selection.rom_path)) return mia_core_error(MIA_CORE_ERR_CALLBACK, "SNES ROM load failed");
    S9xSetPlaybackRate(Settings.SoundPlaybackRate);
    return load_sram(runtime);
}

MiaCoreStatus mia_emulator_core_flush(MiaEmulatorRuntime *runtime, MiaStorageFlushReason reason, bool force) {
    (void)force;
    const size_t size = sram_size();
    if (size == 0) return mia_core_ok();
    MiaStorageStatus status = mia_app_save_flush(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, reason, Memory.SRAM, size, NULL);
    return status.code == MIA_STORAGE_OK ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, status.message);
}

MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime) {
    unsigned save_counter = 0;
    for (;;) {
        const uint32_t started = (uint32_t)esp_timer_get_time();
        const uint32_t host = mia_emulator_host_buttons();
        if (mia_app_input_exit_requested(&runtime->input, host, mia_host_millis())) return mia_core_ok();
        IPPU.RenderThisFrame = true;
        S9xMainLoop();
        MiaCoreStatus status = mia_core_adapter_submit_video(&runtime->adapter, (const uint16_t *)GFX.Screen, SNES_WIDTH * SNES_HEIGHT);
        if (status.code != MIA_CORE_OK) return status;
        S9xMixSamples(mix_buffer, 2048u * 2u);
        status = mia_core_adapter_submit_audio(&runtime->adapter, mix_buffer, 2048u);
        if (status.code != MIA_CORE_OK) return status;
        const uint32_t budget = Memory.ROMFramesPerSecond > 55 ? 16667u : 20000u;
        const uint32_t elapsed = (uint32_t)esp_timer_get_time() - started;
        mia_snes_pacing_record(&pacing, elapsed, budget, 1);
        if (++save_counter % 60u == 0u) {
            status = mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CORE_REQUEST, false);
            if (status.code != MIA_CORE_OK) return status;
        }
        if (elapsed < budget) mia_host_delay_ms((budget - elapsed) / 1000u);
    }
}
