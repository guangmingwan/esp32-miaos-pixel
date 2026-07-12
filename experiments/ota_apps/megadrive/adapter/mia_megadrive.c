#include "megadrive_policy.h"
#include "mia_emulator_runtime.h"
#include "mia_host_abi.h"
#include "gwenesis.h"

#include <esp_heap_caps.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_AUDIO_SAMPLES 1056u
#define MAX_FRAME_PIXELS (320u * 240u)

extern unsigned char *VRAM;
int system_clock, scan_line;
int16_t gwenesis_sn76489_buffer[MAX_AUDIO_SAMPLES];
int sn76489_index, sn76489_clock;
int16_t gwenesis_ym2612_buffer[MAX_AUDIO_SAMPLES];
int ym2612_index, ym2612_clock;
static uint16_t *indexed_frame, *rgb_frame;
static unsigned char *rom_data;
extern unsigned char gwenesis_vdp_regs[0x20];
extern unsigned short gwenesis_vdp_status, CRAM565[256];
extern unsigned int screen_width, screen_height;
extern int hint_pending;
extern void m68k_update_irq(unsigned int level);
extern void m68k_set_irq(unsigned int level);

void gwenesis_io_get_buttons(void) {
    // Input is synchronized by update_pad() before the core runs each frame.
}

static MiaCoreStatus failure(const char *message) {
    return mia_core_error(MIA_CORE_ERR_CALLBACK, message);
}

static uint32_t input_to_policy(uint32_t bits) {
    uint32_t result = 0;
    static const struct { uint8_t host; uint32_t policy; } map[] = {
        {MIA_HOST_BUTTON_UP, MIA_MD_HOST_UP}, {MIA_HOST_BUTTON_DOWN, MIA_MD_HOST_DOWN},
        {MIA_HOST_BUTTON_LEFT, MIA_MD_HOST_LEFT}, {MIA_HOST_BUTTON_RIGHT, MIA_MD_HOST_RIGHT},
        {MIA_HOST_BUTTON_A, MIA_MD_HOST_A}, {MIA_HOST_BUTTON_B, MIA_MD_HOST_B},
        {MIA_HOST_BUTTON_X, MIA_MD_HOST_X}, {MIA_HOST_BUTTON_START, MIA_MD_HOST_START}};
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); ++i)
        if ((bits & (1u << map[i].host)) != 0u) result |= map[i].policy;
    return result;
}

static void update_pad(uint8_t mask) {
    for (int button = MIA_MD_PAD_UP; button <= MIA_MD_PAD_START; ++button) {
        if ((mask & (1u << button)) != 0u) gwenesis_io_pad_press_button(0, button);
        else gwenesis_io_pad_release_button(0, button);
    }
}

static MiaCoreStatus load_save(MiaEmulatorRuntime *runtime) {
    const size_t capacity = gwenesis_bus_sram_size();
    if (capacity == 0u) return mia_core_ok();
    size_t loaded = 0;
    MiaStorageStatus status = mia_app_save_load(&runtime->storage, &runtime->storage_target,
        runtime->selection.save_name, gwenesis_bus_sram_data(), capacity, &loaded);
    if (status.code == MIA_STORAGE_ERR_MISSING_REQUIRED_FILE) return mia_core_ok();
    if (status.code != MIA_STORAGE_OK || loaded != capacity) return failure("malformed Megadrive SRAM save");
    return mia_core_ok();
}

MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime) {
    FILE *file = fopen(runtime->selection.rom_path, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0) return failure("Megadrive ROM open failed");
    const long length = ftell(file);
    if (length < 0x200 || length > MAX_ROM_SIZE || fseek(file, 0, SEEK_SET) != 0) { fclose(file); return failure("Megadrive ROM size invalid"); }
    rom_data = heap_caps_malloc((size_t)length, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    VRAM = heap_caps_malloc(VRAM_MAX_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    indexed_frame = heap_caps_calloc(MAX_FRAME_PIXELS, sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    rgb_frame = heap_caps_malloc(MAX_FRAME_PIXELS * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (rom_data == NULL || VRAM == NULL || indexed_frame == NULL || rgb_frame == NULL) { fclose(file); return failure("Megadrive memory allocation failed"); }
    if (fread(rom_data, 1, (size_t)length, file) != (size_t)length) { fclose(file); return failure("Megadrive ROM read failed"); }
    fclose(file);
    const MiaMegadriveSram sram = mia_megadrive_sram_parse(rom_data, (size_t)length);
    gwenesis_bus_configure_sram(sram.start, sram.present ? sram.size : 0u);
    load_cartridge(rom_data, (size_t)length); power_on(); reset_emulation();
    return load_save(runtime);
}

MiaCoreStatus mia_emulator_core_flush(MiaEmulatorRuntime *runtime, MiaStorageFlushReason reason, bool force) {
    const size_t size = gwenesis_bus_sram_size();
    if (size == 0u || !mia_megadrive_save_should_flush(gwenesis_bus_sram_dirty() != 0, force)) return mia_core_ok();
    MiaStorageStatus status = mia_app_save_flush(&runtime->storage, &runtime->storage_target,
        runtime->selection.save_name, reason, gwenesis_bus_sram_data(), size, NULL);
    if (status.code == MIA_STORAGE_OK) gwenesis_bus_sram_mark_clean();
    return status.code == MIA_STORAGE_OK ? mia_core_ok() : failure(status.message);
}

static void run_scanlines(bool draw, MiaMegadriveTiming timing) {
    int hint_counter = gwenesis_vdp_regs[10];
    system_clock = zclk = ym2612_clock = sn76489_clock = 0;
    ym2612_index = sn76489_index = scan_line = 0;
    while (scan_line < timing.lines) {
        const int target = system_clock + VDP_CYCLES_PER_LINE;
        m68k_run(target); z80_run(target);
        if (draw && scan_line < (int)screen_height) gwenesis_vdp_render_line(scan_line);
        if ((scan_line == 0) || (scan_line > (int)screen_height)) hint_counter = gwenesis_vdp_regs[10];
        if (--hint_counter < 0) { if (REG0_LINE_INTERRUPT && scan_line <= (int)screen_height) { hint_pending = 1; m68k_update_irq(4); } hint_counter = gwenesis_vdp_regs[10]; }
        if (++scan_line == (int)screen_height) { if (REG1_VBLANK_INTERRUPT) { gwenesis_vdp_status |= STATUS_VIRQPENDING; m68k_set_irq(6); } z80_irq_line(1); }
        if (scan_line == (int)screen_height + 1) z80_irq_line(0);
        system_clock = target;
    }
    gwenesis_SN76489_run(system_clock); ym2612_run(system_clock); m68k.cycles -= system_clock;
}

static MiaCoreStatus submit_frame(MiaEmulatorRuntime *runtime) {
    const size_t count = screen_width * screen_height;
    for (size_t i = 0; i < count; ++i) rgb_frame[i] = CRAM565[indexed_frame[i] & 0xffu];
    runtime->hardware_target.geometry.width = (uint16_t)screen_width;
    runtime->hardware_target.geometry.height = (uint16_t)screen_height;
    return mia_core_adapter_submit_video(&runtime->adapter, rgb_frame, count);
}

static MiaCoreStatus submit_audio(MiaEmulatorRuntime *runtime, uint16_t frames) {
    int16_t mixed[MAX_AUDIO_SAMPLES];
    if ((size_t)frames * 2u > MAX_AUDIO_SAMPLES) return failure("Megadrive audio frame overflow");
    for (size_t i = 0; i < frames; ++i) {
        const int32_t psg = i < (size_t)sn76489_index ? gwenesis_sn76489_buffer[i] : 0;
        for (size_t channel = 0; channel < 2; ++channel) {
            int32_t value = gwenesis_ym2612_buffer[i * 2u + channel] + psg;
            if (value > INT16_MAX) value = INT16_MAX;
            if (value < INT16_MIN) value = INT16_MIN;
            mixed[i * 2u + channel] = (int16_t)value;
        }
    }
    return mia_core_adapter_submit_audio(&runtime->adapter, mixed, frames);
}

MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime) {
    uint8_t old_pad = 0xff; uint32_t frames = 0;
    for (;;) {
        const uint32_t host = mia_emulator_host_buttons();
        if (mia_app_input_exit_requested(&runtime->input, host, mia_host_millis())) return mia_core_ok();
        const uint8_t pad = mia_megadrive_pad_mask(input_to_policy(host));
        if (pad != old_pad) { update_pad(pad); old_pad = pad; }
        const bool pal = REG1_PAL != 0; const MiaMegadriveTiming timing = mia_megadrive_timing(pal);
        const MiaMegadriveGeometry geometry = mia_megadrive_geometry(REG12_MODE_H40 != 0, pal);
        screen_width = geometry.width; screen_height = geometry.height;
        gwenesis_vdp_set_buffer(indexed_frame); gwenesis_vdp_render_config(); run_scanlines(true, timing);
        MiaCoreStatus status = submit_frame(runtime); if (status.code != MIA_CORE_OK) return status;
        status = submit_audio(runtime, timing.audio_frames); if (status.code != MIA_CORE_OK) return status;
        if (++frames % timing.refresh_hz == 0u) { status = mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CORE_REQUEST, false); if (status.code != MIA_CORE_OK) return status; }
        mia_host_delay_ms(1);
    }
}
