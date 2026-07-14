#include "mia_emulator_runtime.h"
#include "gw_system.h"
#include "mia_host_abi.h"

#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdlib.h>

unsigned char *ROM_DATA;
unsigned int ROM_DATA_LENGTH;
static uint32_t gw_input;

#define GW_FRAME_PIXELS (320u * 240u)
#define GW_DISPLAY_BUFFER_COUNT 3u

static QueueHandle_t display_ready_queue;
static QueueHandle_t display_free_queue;
static TaskHandle_t display_task_handle;
static volatile bool display_running;
static uint16_t *display_buffers[GW_DISPLAY_BUFFER_COUNT];

unsigned int gw_get_buttons(void) { return gw_input; }

static void display_task(void *arg) {
    MiaEmulatorRuntime *runtime = arg;
    while (display_running || uxQueueMessagesWaiting(display_ready_queue) != 0u) {
        uint16_t *frame = NULL;
        if (xQueueReceive(display_ready_queue, &frame, pdMS_TO_TICKS(20)) != pdTRUE) continue;
        MiaCoreStatus status = mia_core_adapter_submit_video(&runtime->adapter, frame, GW_FRAME_PIXELS);
        if (status.code != MIA_CORE_OK) mia_host_log(status.message);
        (void)xQueueSend(display_free_queue, &frame, portMAX_DELAY);
    }
    display_task_handle = NULL;
    vTaskDelete(NULL);
}

static bool start_display_task(MiaEmulatorRuntime *runtime) {
    display_ready_queue = xQueueCreate(1, sizeof(uint16_t *));
    display_free_queue = xQueueCreate(GW_DISPLAY_BUFFER_COUNT, sizeof(uint16_t *));
    if (display_ready_queue == NULL || display_free_queue == NULL) return false;
    for (unsigned index = 0; index < GW_DISPLAY_BUFFER_COUNT; ++index) {
        display_buffers[index] = heap_caps_malloc(GW_FRAME_PIXELS * sizeof(uint16_t),
                                                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (display_buffers[index] == NULL) return false;
        (void)xQueueSend(display_free_queue, &display_buffers[index], 0);
    }
    display_running = true;
    if (xTaskCreatePinnedToCore(display_task, "gw_display", 4096, runtime, 5,
                                &display_task_handle, 0) != pdPASS) {
        display_running = false;
        return false;
    }
    return true;
}

static void stop_display_task(void) {
    display_running = false;
    for (unsigned wait = 0; display_task_handle != NULL && wait < 100u; ++wait) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (display_task_handle != NULL) {
        vTaskDelete(display_task_handle);
        display_task_handle = NULL;
    }
    if (display_ready_queue != NULL) vQueueDelete(display_ready_queue);
    if (display_free_queue != NULL) vQueueDelete(display_free_queue);
    display_ready_queue = NULL;
    display_free_queue = NULL;
    for (unsigned index = 0; index < GW_DISPLAY_BUFFER_COUNT; ++index) {
        heap_caps_free(display_buffers[index]);
        display_buffers[index] = NULL;
    }
}

static MiaCoreStatus read_rom(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GW ROM open failed");
    ROM_DATA = malloc(400000u);
    if (ROM_DATA == NULL) { fclose(file); return mia_core_error(MIA_CORE_ERR_CALLBACK, "GW ROM allocation failed"); }
    ROM_DATA_LENGTH = (unsigned int)fread(ROM_DATA, 1, 400000u, file);
    fclose(file);
    return ROM_DATA_LENGTH > 0 ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, "GW ROM read failed");
}

static MiaCoreStatus load_save(MiaEmulatorRuntime *runtime, bool *loaded) {
    gw_state_t state;
    size_t size = 0;
    *loaded = false;
    MiaStorageStatus status = mia_app_save_load(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, (uint8_t *)&state, sizeof(state), &size);
    if (status.code == MIA_STORAGE_ERR_MISSING_REQUIRED_FILE) return mia_core_ok();
    if (status.code != MIA_STORAGE_OK || size != sizeof(state) || !gw_state_load(&state)) {
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "GW state load failed");
    }
    *loaded = true;
    return mia_core_ok();
}

static void initialize_watch_mode(void) {
    gw_input = 0;
    gw_system_reset();
    (void)gw_system_run(GW_AUDIO_FREQ * 2u);
    gw_input = GW_BUTTON_TIME;
    (void)gw_system_run(GW_AUDIO_FREQ / 2u);
    gw_input = 0;
    (void)gw_system_run(GW_AUDIO_FREQ * 2u);
    gw_input = GW_BUTTON_A;
    (void)gw_system_run(GW_AUDIO_FREQ / 2u);
    gw_input = 0;
    (void)gw_system_run(GW_AUDIO_FREQ);
}

MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime) {
    MiaCoreStatus status = read_rom(runtime->selection.rom_path);
    if (status.code != MIA_CORE_OK) return status;
    if (!gw_system_romload() || !gw_system_config()) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GW core init failed");
    gw_system_sound_init();
    gw_system_start();
    gw_system_reset();
    bool loaded = false;
    status = load_save(runtime, &loaded);
    if (status.code == MIA_CORE_OK && !loaded) initialize_watch_mode();
    return status;
}

MiaCoreStatus mia_emulator_core_flush(MiaEmulatorRuntime *runtime, MiaStorageFlushReason reason, bool force) {
    (void)force;
    gw_state_t state;
    if (!gw_state_save(&state)) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GW state export failed");
    MiaStorageStatus status = mia_app_save_flush(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, reason, (const uint8_t *)&state, sizeof(state), NULL);
    return status.code == MIA_STORAGE_OK ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, status.message);
}

MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime) {
    static int16_t audio[GW_AUDIO_BUFFER_LENGTH * 2];
    if (!start_display_task(runtime)) {
        stop_display_task();
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "GW display task start failed");
    }
    unsigned frames = 0;
    for (;;) {
        uint32_t input = 0;
        MiaCoreStatus status = mia_core_adapter_poll_input(&runtime->adapter, &input);
        if (status.code != MIA_CORE_OK) {
            stop_display_task();
            return status;
        }
        if (mia_app_input_exit_requested(&runtime->input, mia_emulator_host_buttons(), mia_host_millis())) {
            stop_display_task();
            return mia_core_ok();
        }
        gw_input = mia_app_input_gw_mask(input);
        (void)gw_system_run(GW_SYSTEM_CYCLES);
        if (uxQueueMessagesWaiting(display_ready_queue) == 0u) {
            uint16_t *framebuffer = NULL;
            if (xQueueReceive(display_free_queue, &framebuffer, 0) == pdTRUE) {
                gw_system_blit(framebuffer);
                if (xQueueSend(display_ready_queue, &framebuffer, 0) != pdTRUE) {
                    (void)xQueueSend(display_free_queue, &framebuffer, 0);
                }
            }
        }
        for (size_t index = 0; index < GW_AUDIO_BUFFER_LENGTH; ++index) {
            const int16_t sample = (int16_t)((int)gw_audio_buffer[index] << 7);
            audio[index * 2u] = sample;
            audio[index * 2u + 1u] = sample;
        }
        status = mia_core_adapter_submit_audio(&runtime->adapter, audio, GW_AUDIO_BUFFER_LENGTH);
        if (status.code != MIA_CORE_OK) {
            stop_display_task();
            return status;
        }
        gw_audio_buffer_copied = true;
        if (++frames % 128u == 0u) {
            status = mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CORE_REQUEST, false);
            if (status.code != MIA_CORE_OK) {
                stop_display_task();
                return status;
            }
        }
    }
}
