#include "gba_policy.h"
#include "mia_emulator_runtime.h"
#include "mia_host_abi.h"

#include "common.h"

#include <esp_heap_caps.h>
#include <mbedtls/md5.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const GBA_BIOS_PATH = "/bios/gba_bios.bin";
#define AUDIO_FRAMES (GBA_SOUND_FREQUENCY / 60u + 1u)

u32 idle_loop_target_pc = 0xFFFFFFFFu;
u32 translation_gate_target_pc[MAX_TRANSLATION_GATES];
u32 translation_gate_targets = 0;
boot_mode selected_boot_mode = boot_game;
u32 skip_next_frame = 0;
int sprite_limit = 1;
gbsp_memory_t *gbsp_memory = NULL;

extern u32 gamepak_buffer_count;
extern void mia_gbsp_execute_arm(u32 cycles);

void netpacket_poll_receive(void) {}
void netpacket_send(uint16_t, const void *, size_t) {}
void set_fastforward_override(bool) {}

static int16_t input_callback(unsigned, unsigned, unsigned, unsigned id) {
    const uint32_t bits = mia_emulator_host_buttons();
    uint16_t mask = 0;
    static const struct { uint8_t host; uint8_t retro; } buttons[] = {
        {MIA_HOST_BUTTON_A, RETRO_DEVICE_ID_JOYPAD_A}, {MIA_HOST_BUTTON_B, RETRO_DEVICE_ID_JOYPAD_B},
        {MIA_HOST_BUTTON_L, RETRO_DEVICE_ID_JOYPAD_L}, {MIA_HOST_BUTTON_R, RETRO_DEVICE_ID_JOYPAD_R},
        {MIA_HOST_BUTTON_START, RETRO_DEVICE_ID_JOYPAD_START}, {MIA_HOST_BUTTON_SELECT, RETRO_DEVICE_ID_JOYPAD_SELECT},
        {MIA_HOST_BUTTON_UP, RETRO_DEVICE_ID_JOYPAD_UP}, {MIA_HOST_BUTTON_DOWN, RETRO_DEVICE_ID_JOYPAD_DOWN},
        {MIA_HOST_BUTTON_LEFT, RETRO_DEVICE_ID_JOYPAD_LEFT}, {MIA_HOST_BUTTON_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT},
    };
    for (size_t index = 0; index < sizeof(buttons) / sizeof(buttons[0]); ++index) {
        if ((bits & (1u << buttons[index].host)) != 0u) mask |= (uint16_t)(1u << buttons[index].retro);
    }
    return id == RETRO_DEVICE_ID_JOYPAD_MASK ? (int16_t)mask : (int16_t)((mask >> id) & 1u);
}

static bool validate_bios(void) {
    FILE *file = fopen(GBA_BIOS_PATH, "rb");
    if (file == NULL) return false;
    mbedtls_md5_context context;
    mbedtls_md5_init(&context);
    mbedtls_md5_starts(&context);
    uint8_t chunk[512];
    size_t size = 0;
    for (size_t count = fread(chunk, 1, sizeof(chunk), file); count != 0; count = fread(chunk, 1, sizeof(chunk), file)) {
        mbedtls_md5_update(&context, chunk, count);
        size += count;
    }
    fclose(file);
    uint8_t digest[16];
    mbedtls_md5_finish(&context, digest);
    mbedtls_md5_free(&context);
    return mia_gba_bios_metadata_valid(size, digest);
}

static MiaCoreStatus load_native_save(MiaEmulatorRuntime *runtime) {
    size_t size = 0;
    MiaStorageStatus status = mia_app_save_load(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, gamepak_backup, sizeof(gamepak_backup), &size);
    if (status.code == MIA_STORAGE_ERR_MISSING_REQUIRED_FILE) return mia_core_ok();
    const bool valid = size == 512u || size == 8u * 1024u || size == 32u * 1024u || size == 64u * 1024u || size == 128u * 1024u;
    return status.code == MIA_STORAGE_OK && valid ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, "malformed GBA save");
}

MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime) {
    struct stat rom_stat;
    if (stat(runtime->selection.rom_path, &rom_stat) != 0 || !mia_gba_rom_size_valid((size_t)rom_stat.st_size)) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GBA ROM exceeds 32 MiB or has an invalid header size");
    if (!validate_bios() || load_bios((char *)GBA_BIOS_PATH) != 0) return mia_core_error(MIA_CORE_ERR_CALLBACK, "canonical GBA BIOS missing or corrupt");
    gbsp_memory = heap_caps_calloc(1, sizeof(*gbsp_memory), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    gba_screen_pixels = heap_caps_malloc(GBA_SCREEN_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (gbsp_memory == NULL || gba_screen_pixels == NULL) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GBA PSRAM allocation failed");
    init_gamepak_buffer();
    if (!mia_gba_page_allocation_valid(gamepak_buffer_count)) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GBA ROM page allocation failed");
    init_sound();
    libretro_supports_bitmasks = true;
    retro_set_input_state(input_callback);
    memset(gamepak_backup, 0xff, sizeof(gamepak_backup));
    if (load_gamepak(NULL, runtime->selection.rom_path, FEAT_AUTODETECT, FEAT_AUTODETECT, SERIAL_MODE_DISABLED) != 0) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GBA ROM load failed");
    MiaCoreStatus save = load_native_save(runtime);
    if (save.code != MIA_CORE_OK) return save;
    reset_gba();
    return mia_core_ok();
}

MiaCoreStatus mia_emulator_core_flush(MiaEmulatorRuntime *runtime, MiaStorageFlushReason reason, bool force) {
    if (!force && flush_ram_count == 0u) return mia_core_ok();
    const size_t size = mia_gba_save_size((MiaGbaSaveState){(MiaGbaSaveType)backup_type, flash_bank_cnt, eeprom_size});
    if (size == 0u) return backup_type == BACKUP_UNKN ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, "unsupported GBA save type");
    MiaStorageStatus status = mia_app_save_flush(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, reason, gamepak_backup, size, NULL);
    if (status.code == MIA_STORAGE_OK) flush_ram_count = 0;
    return status.code == MIA_STORAGE_OK ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, status.message);
}

MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime) {
    int16_t audio[AUDIO_FRAMES * 2u];
    unsigned frames = 0;
    for (;;) {
        if (mia_app_input_exit_requested(&runtime->input, mia_emulator_host_buttons(), mia_host_millis())) return mia_core_ok();
        update_input();
        clear_gamepak_stickybits();
        mia_gbsp_execute_arm(execute_cycles);
        if (skip_next_frame == 0u) {
            MiaCoreStatus video = mia_core_adapter_submit_video(&runtime->adapter, gba_screen_pixels, GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT);
            if (video.code != MIA_CORE_OK) return video;
        }
        const u32 count = sound_read_samples(audio, AUDIO_FRAMES);
        MiaCoreStatus sound = mia_core_adapter_submit_audio(&runtime->adapter, audio, count);
        if (sound.code != MIA_CORE_OK) return sound;
        if (++frames % 60u == 0u) {
            MiaCoreStatus save = mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CORE_REQUEST, true);
            if (save.code != MIA_CORE_OK) return save;
        }
        mia_host_delay_ms(1);
    }
}
