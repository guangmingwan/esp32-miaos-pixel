#include "gba_policy.h"
#include "display_host.h"
#include "mia_emulator_runtime.h"
#include "mia_host_abi.h"

#include "common.h"

#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <mbedtls/md5.h>
#include <mbedtls/version.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AUDIO_FRAMES (GBA_SOUND_FREQUENCY / 60u + 1u)
#define AUDIO_BLOCK_COUNT 12u
#define AUDIO_PREBUFFER_BLOCKS 5u
#define AUDIO_DISPLAY_MIN_BLOCKS 3u
#define SAVE_FLUSH_INTERVAL_FRAMES 1800u
#define VIDEO_FRAME_DIVISOR 2u
#define GBA_DISPLAY_X 40
#define GBA_DISPLAY_Y 40
#define GBA_FRAMEBUFFER_CAPS (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)

#if MBEDTLS_VERSION_MAJOR >= 3
#define MIA_MD5_STARTS mbedtls_md5_starts
#define MIA_MD5_UPDATE mbedtls_md5_update
#define MIA_MD5_FINISH mbedtls_md5_finish
#else
#define MIA_MD5_STARTS mbedtls_md5_starts_ret
#define MIA_MD5_UPDATE mbedtls_md5_update_ret
#define MIA_MD5_FINISH mbedtls_md5_finish_ret
#endif

typedef struct {
    u32 frame_count;
    int16_t samples[AUDIO_FRAMES * 2u];
} GbaAudioBlock;

u32 idle_loop_target_pc = 0xFFFFFFFFu;
u32 translation_gate_target_pc[MAX_TRANSLATION_GATES];
u32 translation_gate_targets = 0;
boot_mode selected_boot_mode = boot_game;
u32 skip_next_frame = 0;
int sprite_limit = 1;
gbsp_memory_t *gbsp_memory = NULL;

extern u32 gamepak_buffer_count;
extern void mia_gbsp_execute_arm(u32 cycles);

static GbaAudioBlock *audio_blocks;
static QueueHandle_t audio_free_blocks;
static QueueHandle_t audio_ready_blocks;
static TaskHandle_t audio_task_handle;
static volatile bool audio_running;
static volatile bool audio_failed;

static void free_gba_core_memory(void) {
    free(gba_screen_pixels);
    gba_screen_pixels = NULL;
    if (gbsp_memory != NULL) {
        free(memory_map_read);
        free(iwram);
        free(gbsp_memory);
        gbsp_memory = NULL;
    }
}

void netpacket_poll_receive(void) {}
void netpacket_send(uint16_t port, const void *data, size_t size) {}
void set_fastforward_override(bool enabled) {}

static void audio_task(void *argument) {
    MiaEmulatorRuntime *runtime = argument;
    bool buffered = false;
    while (audio_running) {
        if (!buffered) {
            if (uxQueueMessagesWaiting(audio_ready_blocks) < AUDIO_PREBUFFER_BLOCKS) {
                vTaskDelay(pdMS_TO_TICKS(1));
                continue;
            }
            buffered = true;
        }
        uint8_t index = 0;
        if (xQueueReceive(audio_ready_blocks, &index, pdMS_TO_TICKS(20)) != pdTRUE) {
            buffered = false;
            continue;
        }
        if (!audio_running) {
            (void)xQueueSend(audio_free_blocks, &index, 0);
            break;
        }
        GbaAudioBlock *block = &audio_blocks[index];
        const int32_t written = mia_host_audio_write_pcm16(
            block->samples, block->frame_count, 2);
        (void)xQueueSend(audio_free_blocks, &index, 0);
        if (written != (int32_t)block->frame_count) {
            mia_host_log("GBA audio write failed");
            audio_failed = true;
            audio_running = false;
        } else {
            runtime->adapter.audio_submitted += block->frame_count;
        }
    }
    audio_task_handle = NULL;
    vTaskDelete(NULL);
}

static bool start_audio_task(MiaEmulatorRuntime *runtime) {
    audio_blocks = heap_caps_calloc(AUDIO_BLOCK_COUNT, sizeof(*audio_blocks),
                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    audio_free_blocks = xQueueCreate(AUDIO_BLOCK_COUNT, sizeof(uint8_t));
    audio_ready_blocks = xQueueCreate(AUDIO_BLOCK_COUNT, sizeof(uint8_t));
    if (audio_blocks == NULL || audio_free_blocks == NULL || audio_ready_blocks == NULL) return false;
    for (uint8_t index = 0; index < AUDIO_BLOCK_COUNT; ++index) {
        (void)xQueueSend(audio_free_blocks, &index, 0);
    }
    audio_failed = false;
    audio_running = true;
    if (xTaskCreatePinnedToCore(audio_task, "gba_audio", 4096, runtime, 6,
                                &audio_task_handle, 0) != pdPASS) {
        audio_running = false;
        return false;
    }
    return true;
}

static void stop_audio_task(void) {
    audio_running = false;
    for (unsigned wait = 0; audio_task_handle != NULL && wait < 40u; ++wait) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (audio_task_handle != NULL) {
        vTaskDelete(audio_task_handle);
        audio_task_handle = NULL;
    }
    if (audio_free_blocks != NULL) vQueueDelete(audio_free_blocks);
    if (audio_ready_blocks != NULL) vQueueDelete(audio_ready_blocks);
    free(audio_blocks);
    audio_blocks = NULL;
    audio_free_blocks = NULL;
    audio_ready_blocks = NULL;
}

static MiaCoreStatus queue_audio(const int16_t *samples, u32 frame_count) {
    if (frame_count == 0u) return mia_core_ok();
    if (audio_failed || !audio_running) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GBA audio task failed");
    uint8_t index = 0;
    if (xQueueReceive(audio_free_blocks, &index, pdMS_TO_TICKS(20)) != pdTRUE) {
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "GBA audio queue stalled");
    }
    GbaAudioBlock *block = &audio_blocks[index];
    block->frame_count = frame_count;
    memcpy(block->samples, samples, (size_t)frame_count * 2u * sizeof(int16_t));
    if (xQueueSend(audio_ready_blocks, &index, pdMS_TO_TICKS(20)) != pdTRUE) {
        (void)xQueueSend(audio_free_blocks, &index, 0);
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "GBA audio queue stalled");
    }
    return mia_core_ok();
}

static MiaCoreStatus submit_video(MiaEmulatorRuntime *runtime) {
    const int32_t result = display_host_present_rgb565_region(
        gba_screen_pixels, GBA_DISPLAY_X, GBA_DISPLAY_Y, GBA_SCREEN_WIDTH,
        GBA_SCREEN_HEIGHT, GBA_SCREEN_WIDTH * sizeof(uint16_t));
    if (result != MIA_HOST_RESULT_OK) {
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "GBA video presentation failed");
    }
    runtime->adapter.frames_submitted += 1u;
    return mia_core_ok();
}

static int16_t input_callback(unsigned port, unsigned device, unsigned index, unsigned id) {
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
    FILE *file = fopen(MIA_GBA_BIOS_PATH, "rb");
    if (file == NULL) return false;
    mbedtls_md5_context context;
    mbedtls_md5_init(&context);
    (void)MIA_MD5_STARTS(&context);
    uint8_t chunk[512];
    size_t size = 0;
    for (size_t count = fread(chunk, 1, sizeof(chunk), file); count != 0; count = fread(chunk, 1, sizeof(chunk), file)) {
        (void)MIA_MD5_UPDATE(&context, chunk, count);
        size += count;
    }
    fclose(file);
    uint8_t digest[16];
    (void)MIA_MD5_FINISH(&context, digest);
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
    if (!validate_bios()) return mia_core_error(MIA_CORE_ERR_CALLBACK, "canonical GBA BIOS missing or corrupt");
    gbsp_memory = heap_caps_calloc(1, sizeof(*gbsp_memory), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (gbsp_memory != NULL) {
        iwram = heap_caps_calloc(
            1, GBSP_IWRAM_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        memory_map_read = heap_caps_calloc(
            GBSP_MEMORY_MAP_ENTRIES, sizeof(*memory_map_read),
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    gba_screen_pixels = heap_caps_malloc(GBA_SCREEN_BUFFER_SIZE, GBA_FRAMEBUFFER_CAPS);
    if (gbsp_memory == NULL || iwram == NULL || memory_map_read == NULL ||
        gba_screen_pixels == NULL) {
        free_gba_core_memory();
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "GBA memory allocation failed");
    }
    if (load_bios((char *)MIA_GBA_BIOS_PATH) != 0) return mia_core_error(MIA_CORE_ERR_CALLBACK, "canonical GBA BIOS missing or corrupt");
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
    display_host_fill_screen_rgb565(0);
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
    if (!start_audio_task(runtime)) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GBA audio task allocation failed");
    for (;;) {
        if (mia_app_input_exit_requested(&runtime->input, mia_emulator_host_buttons(), mia_host_millis())) {
            stop_audio_task();
            return mia_core_ok();
        }
        update_input();
        clear_gamepak_stickybits();
        mia_gbsp_execute_arm(execute_cycles);
        const bool audio_has_headroom =
            uxQueueMessagesWaiting(audio_ready_blocks) >= AUDIO_DISPLAY_MIN_BLOCKS;
        if (skip_next_frame == 0u && audio_has_headroom &&
            (frames % VIDEO_FRAME_DIVISOR) == 0u) {
            MiaCoreStatus video = submit_video(runtime);
            if (video.code != MIA_CORE_OK) {
                stop_audio_task();
                return video;
            }
        }
        const u32 count = sound_read_samples(audio, AUDIO_FRAMES);
        MiaCoreStatus sound = queue_audio(audio, count);
        if (sound.code != MIA_CORE_OK) {
            stop_audio_task();
            return sound;
        }
        if (++frames % SAVE_FLUSH_INTERVAL_FRAMES == 0u) {
            MiaCoreStatus save = mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CORE_REQUEST, false);
            if (save.code != MIA_CORE_OK) {
                stop_audio_task();
                return save;
            }
        }
        taskYIELD();
    }
}
