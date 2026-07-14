#include "mia_emulator_runtime.h"
#include "mia_app_zip.h"
#include "mia_snes_contract.h"
#include "mia_host_abi.h"
#include "display_host.h"
#include "snes9x.h"

#include <esp_heap_caps.h>
#include <esp_rom_sys.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdlib.h>
#include <string.h>

#define SNES_AUDIO_BLOCK_COUNT 6u
#define SNES_AUDIO_PREBUFFER_BLOCKS 2u
#define SNES_AUDIO_MAX_FRAMES 640u
#define SNES_VIDEO_BUFFER_COUNT 2u
#define SNES_INITIAL_FRAMESKIP 3u
#define SNES_VIDEO_DIRTY_GAP_ROWS 2u

typedef struct {
    uint16_t frame_count;
    int16_t samples[SNES_AUDIO_MAX_FRAMES * 2u];
} SnesAudioBlock;

static MiaEmulatorRuntime *active_runtime;
static int16_t *mix_buffer;
static MiaSnesPacing pacing;
static uint32_t current_host_buttons;
static SnesAudioBlock *audio_blocks;
static QueueHandle_t audio_free_blocks;
static QueueHandle_t audio_ready_blocks;
static TaskHandle_t audio_task_handle;
static volatile bool audio_running;
static volatile bool audio_failed;
static uint32_t audio_dropped_blocks;
static uint8_t *video_buffers[SNES_VIDEO_BUFFER_COUNT];
static QueueHandle_t video_free_buffers;
static QueueHandle_t video_ready_buffers;
static TaskHandle_t video_task_handle;
static volatile bool video_running;
static volatile bool video_failed;
static uint32_t video_row_hashes[SNES_HEIGHT];
static uint32_t video_next_row_hashes[SNES_HEIGHT];
static bool video_hashes_valid;

static uint32_t hash_video_row(const uint16_t *pixels) {
    uint32_t hash = 2166136261u;
    for (uint32_t column = 0; column < SNES_WIDTH; column += 2u) {
        uint32_t pair;
        memcpy(&pair, pixels + column, sizeof(pair));
        hash ^= pair;
        hash = (hash << 5) | (hash >> 27);
    }
    return hash;
}

static int32_t present_video_frame(const uint16_t *pixels) {
    const uint8_t *source = (const uint8_t *)pixels;
    for (uint32_t row = 0; row < SNES_HEIGHT; ++row) {
        video_next_row_hashes[row] = hash_video_row(
            (const uint16_t *)(source + row * (uint32_t)GFX.Pitch));
    }

    uint32_t row = 0;
    while (row < SNES_HEIGHT) {
        while (row < SNES_HEIGHT && video_hashes_valid &&
               video_next_row_hashes[row] == video_row_hashes[row]) {
            ++row;
        }
        if (row == SNES_HEIGHT) break;

        const uint32_t first_row = row;
        uint32_t last_changed = row;
        uint32_t unchanged = 0;
        for (++row; row < SNES_HEIGHT; ++row) {
            const bool changed = !video_hashes_valid ||
                video_next_row_hashes[row] != video_row_hashes[row];
            if (changed) {
                last_changed = row;
                unchanged = 0;
            } else if (++unchanged > SNES_VIDEO_DIRTY_GAP_ROWS) {
                break;
            }
        }

        const uint32_t rows = last_changed - first_row + 1u;
        const int32_t result = display_host_present_rgb565_region(
            (const uint16_t *)(source + first_row * (uint32_t)GFX.Pitch),
            (320 - SNES_WIDTH) / 2, (240 - SNES_HEIGHT) / 2 + (int32_t)first_row,
            SNES_WIDTH, rows, GFX.Pitch);
        if (result != MIA_HOST_RESULT_OK) return result;
    }

    memcpy(video_row_hashes, video_next_row_hashes, sizeof(video_row_hashes));
    video_hashes_valid = true;
    return MIA_HOST_RESULT_OK;
}

static void video_task(void *argument) {
    MiaEmulatorRuntime *runtime = argument;
    while (video_running) {
        uint8_t index = 0;
        if (xQueueReceive(video_ready_buffers, &index, pdMS_TO_TICKS(20)) != pdTRUE) {
            continue;
        }
        const int32_t result = present_video_frame((const uint16_t *)video_buffers[index]);
        (void)xQueueSend(video_free_buffers, &index, 0);
        if (result != MIA_HOST_RESULT_OK) {
            video_failed = true;
            video_running = false;
        } else {
            runtime->adapter.frames_submitted++;
        }
    }
    video_task_handle = NULL;
    vTaskDelete(NULL);
}

static bool start_video_task(MiaEmulatorRuntime *runtime) {
    video_free_buffers = xQueueCreate(SNES_VIDEO_BUFFER_COUNT, sizeof(uint8_t));
    video_ready_buffers = xQueueCreate(1, sizeof(uint8_t));
    if (video_free_buffers == NULL || video_ready_buffers == NULL) return false;
    for (uint8_t index = 0; index < SNES_VIDEO_BUFFER_COUNT; ++index) {
        (void)xQueueSend(video_free_buffers, &index, 0);
    }
    video_hashes_valid = false;
    video_failed = false;
    video_running = true;
    if (xTaskCreatePinnedToCore(video_task, "snes_video", 4096, runtime, 5,
                                &video_task_handle, 0) != pdPASS) {
        video_running = false;
        return false;
    }
    return true;
}

static void stop_video_task(void) {
    video_running = false;
    for (unsigned wait = 0; video_task_handle != NULL && wait < 40u; ++wait) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (video_task_handle != NULL) {
        vTaskDelete(video_task_handle);
        video_task_handle = NULL;
    }
    if (video_free_buffers != NULL) vQueueDelete(video_free_buffers);
    if (video_ready_buffers != NULL) vQueueDelete(video_ready_buffers);
    video_free_buffers = NULL;
    video_ready_buffers = NULL;
}

static bool begin_video_frame(uint8_t *index) {
    if (video_failed || !video_running || index == NULL) return false;
    if (xQueueReceive(video_free_buffers, index, 0) != pdTRUE) return false;
    GFX.Screen = video_buffers[*index];
    return true;
}

static bool submit_video_frame(uint8_t index) {
    if (xQueueSend(video_ready_buffers, &index, 0) == pdTRUE) return true;
    (void)xQueueSend(video_free_buffers, &index, 0);
    return false;
}

static void wait_until(int64_t deadline_us) {
    int64_t remaining = deadline_us - esp_timer_get_time();
    if (remaining > 1500) {
        const TickType_t ticks = pdMS_TO_TICKS((uint32_t)(remaining - 500) / 1000u);
        if (ticks > 0) vTaskDelay(ticks);
    }
    remaining = deadline_us - esp_timer_get_time();
    if (remaining > 0) esp_rom_delay_us((uint32_t)remaining);
}

static void audio_task(void *argument) {
    MiaEmulatorRuntime *runtime = argument;
    bool buffered = false;
    while (audio_running) {
        if (!buffered) {
            if (uxQueueMessagesWaiting(audio_ready_blocks) < SNES_AUDIO_PREBUFFER_BLOCKS) {
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
        SnesAudioBlock *block = &audio_blocks[index];
        const int32_t written = mia_host_audio_write_pcm16(block->samples, block->frame_count, 2);
        (void)xQueueSend(audio_free_blocks, &index, 0);
        if (written != (int32_t)block->frame_count) {
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
    audio_blocks = heap_caps_calloc(SNES_AUDIO_BLOCK_COUNT, sizeof(*audio_blocks),
                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    audio_free_blocks = xQueueCreate(SNES_AUDIO_BLOCK_COUNT, sizeof(uint8_t));
    audio_ready_blocks = xQueueCreate(SNES_AUDIO_BLOCK_COUNT, sizeof(uint8_t));
    if (audio_blocks == NULL || audio_free_blocks == NULL || audio_ready_blocks == NULL) return false;
    for (uint8_t index = 0; index < SNES_AUDIO_BLOCK_COUNT; ++index) {
        (void)xQueueSend(audio_free_blocks, &index, 0);
    }
    audio_failed = false;
    audio_dropped_blocks = 0;
    audio_running = true;
    if (xTaskCreatePinnedToCore(audio_task, "snes_audio", 4096, runtime, 6,
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

static MiaCoreStatus queue_audio(const int16_t *samples, uint32_t frame_count) {
    if (frame_count == 0u) return mia_core_ok();
    if (frame_count > SNES_AUDIO_MAX_FRAMES || audio_failed || !audio_running) {
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "SNES audio task failed");
    }
    uint8_t index = 0;
    if (xQueueReceive(audio_free_blocks, &index, 0) != pdTRUE) {
        if (xQueueReceive(audio_ready_blocks, &index, 0) != pdTRUE) {
            audio_dropped_blocks++;
            return mia_core_ok();
        }
        audio_dropped_blocks++;
    }
    SnesAudioBlock *block = &audio_blocks[index];
    block->frame_count = (uint16_t)frame_count;
    memcpy(block->samples, samples, (size_t)frame_count * 2u * sizeof(int16_t));
    if (xQueueSend(audio_ready_blocks, &index, 0) != pdTRUE) {
        (void)xQueueSend(audio_free_blocks, &index, 0);
        audio_dropped_blocks++;
    }
    return mia_core_ok();
}

static uint32_t snes_pad(uint32_t value) {
    uint32_t pad = 0;
    if (value & (1u << MIA_HOST_BUTTON_A)) pad |= SNES_A_MASK;
    if (value & (1u << MIA_HOST_BUTTON_B)) pad |= SNES_B_MASK;
    if (value & (1u << MIA_HOST_BUTTON_X)) pad |= SNES_X_MASK;
    if (value & (1u << MIA_HOST_BUTTON_Y)) pad |= SNES_Y_MASK;
    if (value & (1u << MIA_HOST_BUTTON_L)) pad |= SNES_TL_MASK;
    if (value & (1u << MIA_HOST_BUTTON_R)) pad |= SNES_TR_MASK;
    if (value & (1u << MIA_HOST_BUTTON_SELECT)) pad |= SNES_SELECT_MASK;
    if (value & (1u << MIA_HOST_BUTTON_START)) pad |= SNES_START_MASK;
    if (value & (1u << MIA_HOST_BUTTON_UP)) pad |= SNES_UP_MASK;
    if (value & (1u << MIA_HOST_BUTTON_DOWN)) pad |= SNES_DOWN_MASK;
    if (value & (1u << MIA_HOST_BUTTON_LEFT)) pad |= SNES_LEFT_MASK;
    if (value & (1u << MIA_HOST_BUTTON_RIGHT)) pad |= SNES_RIGHT_MASK;
    return pad;
}

bool S9xInitDisplay(void) {
    GFX.Pitch = SNES_WIDTH * 2;
    GFX.ZPitch = SNES_WIDTH;
    for (uint8_t index = 0; index < SNES_VIDEO_BUFFER_COUNT; ++index) {
        video_buffers[index] = heap_caps_malloc((size_t)GFX.Pitch * SNES_HEIGHT_EXTENDED,
                                                MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    GFX.Screen = video_buffers[0];
    GFX.SubScreen = heap_caps_malloc((size_t)GFX.Pitch * SNES_HEIGHT_EXTENDED, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    GFX.ZBuffer = heap_caps_malloc((size_t)GFX.ZPitch * SNES_HEIGHT_EXTENDED, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    GFX.SubZBuffer = heap_caps_malloc((size_t)GFX.ZPitch * SNES_HEIGHT_EXTENDED, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (GFX.Screen != NULL && video_buffers[1] != NULL && GFX.SubScreen != NULL &&
        GFX.ZBuffer != NULL && GFX.SubZBuffer != NULL) return true;
    S9xDeinitDisplay();
    return false;
}

void S9xDeinitDisplay(void) {
    for (uint8_t index = 0; index < SNES_VIDEO_BUFFER_COUNT; ++index) {
        free(video_buffers[index]);
        video_buffers[index] = NULL;
    }
    GFX.Screen = NULL;
    free(GFX.SubScreen);
    free(GFX.ZBuffer);
    free(GFX.SubZBuffer);
    GFX.SubScreen = NULL;
    GFX.ZBuffer = NULL;
    GFX.SubZBuffer = NULL;
}

uint32_t S9xReadJoypad(int32_t port) {
    if (port != 0 || active_runtime == NULL) return 0;
    return snes_pad(current_host_buttons);
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
    const char *dot = strrchr(runtime->selection.rom_path, '.');
    const bool zipped = dot != NULL && strcasecmp(dot + 1, "zip") == 0;
    if (!zipped && !mia_snes_extension_supported(runtime->selection.rom_path)) return mia_core_error(MIA_CORE_ERR_INVALID_ARGUMENT, "unsupported SNES file");
    display_host_fill_screen_rgb565(0);
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
    const size_t mix_buffer_bytes = SNES_AUDIO_MAX_FRAMES * 2u * sizeof(int16_t);
    mix_buffer = heap_caps_malloc(mix_buffer_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (mix_buffer == NULL) {
        mix_buffer = heap_caps_malloc(mix_buffer_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (mix_buffer == NULL || !S9xInitDisplay() || !S9xInitMemory() || !S9xInitAPU() || !S9xInitSound(0, 0) || !S9xInitGFX()) return mia_core_error(MIA_CORE_ERR_CALLBACK, "SNES core init failed");
    bool loaded = false;
    if (zipped) {
        static const char *const extensions[] = {"sfc", "smc"};
        const size_t capacity = Memory.ROM_AllocSize;
        size_t size = 0;
        MiaCoreStatus status = mia_app_zip_extract_into(runtime->selection.rom_path,
            extensions, 2u, Memory.ROM, capacity, &size, NULL, 0u);
        if (status.code == MIA_CORE_OK) {
            Memory.ROM_AllocSize = size;
            loaded = LoadROM(NULL);
            Memory.ROM_AllocSize = capacity;
        }
    } else {
        loaded = LoadROM(runtime->selection.rom_path);
    }
    if (!loaded) return mia_core_error(MIA_CORE_ERR_CALLBACK, "SNES ROM load failed");
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
    if (!start_audio_task(runtime) || !start_video_task(runtime)) {
        stop_video_task();
        stop_audio_task();
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "SNES worker task start failed");
    }
    unsigned save_counter = 0;
    uint32_t audio_remainder = 0;
    uint8_t frameskip = SNES_INITIAL_FRAMESKIP;
    uint8_t skip_frames = 0;
    uint32_t window_frames = 0;
    uint32_t window_late_frames = 0;
    uint32_t window_display_busy_frames = 0;
    const uint32_t frame_rate = Memory.ROMFramesPerSecond > 0 ?
        (uint32_t)Memory.ROMFramesPerSecond : 60u;
    const uint32_t budget = frame_rate > 55u ? 16667u : 20000u;
    int64_t next_deadline = esp_timer_get_time() + budget;
    int64_t window_started = esp_timer_get_time();
    memset(&pacing, 0, sizeof(pacing));
    for (;;) {
        const int64_t started = esp_timer_get_time();
        const uint32_t host = mia_emulator_host_buttons();
        current_host_buttons = host;
        if (mia_app_input_exit_requested(&runtime->input, host, mia_host_millis())) {
            stop_video_task();
            stop_audio_task();
            return mia_core_ok();
        }
        if (video_failed || audio_failed) {
            stop_video_task();
            stop_audio_task();
            return mia_core_error(MIA_CORE_ERR_CALLBACK, "SNES worker task failed");
        }

        uint8_t video_index = 0;
        const bool wants_render = skip_frames == 0u;
        const bool render_frame = wants_render && begin_video_frame(&video_index);
        if (wants_render && !render_frame) window_display_busy_frames++;
        IPPU.RenderThisFrame = render_frame;
        S9xMainLoop();
        MiaCoreStatus status = mia_core_ok();
        if (render_frame && !submit_video_frame(video_index)) {
            window_display_busy_frames++;
        }
        if (skip_frames == 0u) skip_frames = frameskip;
        else skip_frames--;

        audio_remainder += MIA_EMULATOR_SAMPLE_RATE;
        const uint32_t audio_frames = audio_remainder / frame_rate;
        audio_remainder %= frame_rate;
        S9xMixSamples(mix_buffer, audio_frames * 2u);
        status = queue_audio(mix_buffer, audio_frames);
        if (status.code != MIA_CORE_OK) {
            stop_video_task();
            stop_audio_task();
            return status;
        }
        const uint32_t elapsed = (uint32_t)(esp_timer_get_time() - started);
        mia_snes_pacing_record(&pacing, elapsed, budget, render_frame);
        window_frames++;
        if (elapsed > budget) window_late_frames++;
        if (++save_counter % 60u == 0u) {
            status = mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CORE_REQUEST, false);
            if (status.code != MIA_CORE_OK) {
                stop_video_task();
                stop_audio_task();
                return status;
            }
        }

        const int64_t now = esp_timer_get_time();
        if (now - window_started >= 1000000) {
            frameskip = mia_snes_adjust_frameskip(frameskip, window_frames, frame_rate,
                                                  window_late_frames,
                                                  window_display_busy_frames);
            window_frames = 0;
            window_late_frames = 0;
            window_display_busy_frames = 0;
            window_started = now;
        }

        wait_until(next_deadline);
        next_deadline += budget;
        if (esp_timer_get_time() - next_deadline > (int64_t)budget * 4) {
            next_deadline = esp_timer_get_time() + budget;
        }
    }
}
