#include "mia_emulator_runtime.h"
#include "mia_app_zip.h"
#include "mia_host_abi.h"
#include "display_host.h"
#include "pce-go.h"
#include "psg.h"

#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define PCE_ROM_MIN_SIZE 0x2000u
#define PCE_ROM_MAX_SIZE 0x1000000u
#define PCE_MAX_FRAME_WIDTH (XBUF_WIDTH - 16u)
#define PCE_MAX_FRAME_HEIGHT XBUF_HEIGHT
#define PCE_DISPLAY_BUFFER_COUNT 2u
#define PCE_AUDIO_CHUNK_FRAMES 62u
#define PCE_FRAME_TIME_US 16667LL

typedef struct {
    uint16_t *pixels;
    uint16_t width;
    uint16_t height;
} PceDisplayFrame;

static uint8_t indexed[XBUF_WIDTH * XBUF_HEIGHT];
static uint16_t palette[256];
static bool exit_requested;
static bool draw_frame;
static QueueHandle_t display_ready_queue;
static QueueHandle_t display_free_queue;
static TaskHandle_t display_task_handle;
static TaskHandle_t audio_task_handle;
static volatile bool display_running;
static volatile bool audio_running;
static PceDisplayFrame display_frames[PCE_DISPLAY_BUFFER_COUNT];
static uint16_t frame_width;
static uint16_t frame_height;
static int64_t next_frame_us;

static bool has_zip_extension(const char *path) {
    const char *extension = strrchr(path, '.');
    return extension != NULL && strcasecmp(extension, ".zip") == 0;
}

uint8_t *osd_gfx_framebuffer(int width, int height) {
    draw_frame = display_free_queue != NULL && uxQueueMessagesWaiting(display_free_queue) > 0u &&
                 width > 0 && height > 0 && width <= (int)PCE_MAX_FRAME_WIDTH &&
                 height <= (int)PCE_MAX_FRAME_HEIGHT;
    if (!draw_frame) return NULL;
    frame_width = (uint16_t)width;
    frame_height = (uint16_t)height;
    return indexed + 16u;
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
    if (draw_frame) {
        PceDisplayFrame *frame = NULL;
        if (xQueueReceive(display_free_queue, &frame, 0) == pdTRUE) {
            frame->width = frame_width;
            frame->height = frame_height;
            for (size_t y = 0; y < frame->height; ++y) {
                const uint8_t *source = indexed + 16u + y * XBUF_WIDTH;
                uint16_t *destination = frame->pixels + y * frame->width;
                for (size_t x = 0; x < frame->width; ++x) destination[x] = palette[source[x]];
            }
            if (xQueueSend(display_ready_queue, &frame, 0) != pdTRUE)
                (void)xQueueSend(display_free_queue, &frame, 0);
        }
    }
    draw_frame = false;

    const int64_t now = esp_timer_get_time();
    if (next_frame_us == 0 || now > next_frame_us + PCE_FRAME_TIME_US * 2) next_frame_us = now;
    next_frame_us += PCE_FRAME_TIME_US;
    const int64_t sleep_us = next_frame_us - esp_timer_get_time();
    if (sleep_us >= 1000) vTaskDelay(pdMS_TO_TICKS((uint32_t)(sleep_us / 1000)));
    else taskYIELD();
}

static void display_task(void *arg) {
    (void)arg;
    uint16_t previous_width = 0;
    uint16_t previous_height = 0;
    while (display_running || uxQueueMessagesWaiting(display_ready_queue) != 0u) {
        PceDisplayFrame *frame = NULL;
        if (xQueueReceive(display_ready_queue, &frame, pdMS_TO_TICKS(20)) != pdTRUE) continue;
        uint32_t output_width = frame->width;
        uint32_t output_height = frame->height;
        if (output_width > 320u || output_height > 240u) {
            if (output_width * 240u > output_height * 320u) {
                output_height = output_height * 320u / output_width;
                output_width = 320u;
            } else {
                output_width = output_width * 240u / output_height;
                output_height = 240u;
            }
            output_width &= ~1u;
            output_height &= ~1u;
        }
        if (output_width != previous_width || output_height != previous_height) {
            display_host_fill_screen_rgb565(0);
            previous_width = (uint16_t)output_width;
            previous_height = (uint16_t)output_height;
        }
        const int32_t x = (320 - (int32_t)output_width) / 2;
        const int32_t y = (240 - (int32_t)output_height) / 2;
        const int32_t result = output_width == frame->width && output_height == frame->height
            ? display_host_present_rgb565_region(frame->pixels, x, y, frame->width,
                                                 frame->height, frame->width * sizeof(uint16_t))
            : display_host_present_rgb565_scaled_region(
                  frame->pixels, frame->width, frame->height,
                  frame->width * sizeof(uint16_t), x, y, output_width, output_height);
        if (result != MIA_HOST_RESULT_OK) mia_host_log("PCE display submission failed");
        (void)xQueueSend(display_free_queue, &frame, portMAX_DELAY);
    }
    display_task_handle = NULL;
    vTaskDelete(NULL);
}

static void audio_task(void *arg) {
    (void)arg;
    int16_t samples[PCE_AUDIO_CHUNK_FRAMES * 2u];
    while (audio_running) {
        psg_update(samples, PCE_AUDIO_CHUNK_FRAMES, 0xff);
        if (mia_host_audio_write_pcm16(samples, PCE_AUDIO_CHUNK_FRAMES, 2) < 0) {
            mia_host_log("PCE audio submission failed");
            break;
        }
    }
    audio_task_handle = NULL;
    vTaskDelete(NULL);
}

static bool start_runtime_tasks(void) {
    display_ready_queue = xQueueCreate(1, sizeof(PceDisplayFrame *));
    display_free_queue = xQueueCreate(PCE_DISPLAY_BUFFER_COUNT, sizeof(PceDisplayFrame *));
    if (display_ready_queue == NULL || display_free_queue == NULL) return false;
    const size_t frame_bytes = PCE_MAX_FRAME_WIDTH * PCE_MAX_FRAME_HEIGHT * sizeof(uint16_t);
    for (size_t index = 0; index < PCE_DISPLAY_BUFFER_COUNT; ++index) {
        display_frames[index].pixels = heap_caps_malloc(frame_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (display_frames[index].pixels == NULL) return false;
        PceDisplayFrame *frame = &display_frames[index];
        (void)xQueueSend(display_free_queue, &frame, 0);
    }
    display_host_fill_screen_rgb565(0);
    display_running = true;
    audio_running = true;
    if (xTaskCreatePinnedToCore(display_task, "pce_display", 4096, NULL, 5,
                                &display_task_handle, 1) != pdPASS ||
        xTaskCreatePinnedToCore(audio_task, "pce_audio", 3072, NULL, 6,
                                &audio_task_handle, 1) != pdPASS) {
        display_running = false;
        audio_running = false;
        return false;
    }
    next_frame_us = 0;
    return true;
}

static void stop_runtime_tasks(void) {
    audio_running = false;
    display_running = false;
    for (unsigned wait = 0; (audio_task_handle != NULL || display_task_handle != NULL) && wait < 100u; ++wait)
        vTaskDelay(pdMS_TO_TICKS(5));
    if (audio_task_handle != NULL) {
        vTaskDelete(audio_task_handle);
        audio_task_handle = NULL;
    }
    if (display_task_handle != NULL) {
        vTaskDelete(display_task_handle);
        display_task_handle = NULL;
    }
    if (display_ready_queue != NULL) vQueueDelete(display_ready_queue);
    if (display_free_queue != NULL) vQueueDelete(display_free_queue);
    display_ready_queue = NULL;
    display_free_queue = NULL;
    for (size_t index = 0; index < PCE_DISPLAY_BUFFER_COUNT; ++index) {
        heap_caps_free(display_frames[index].pixels);
        display_frames[index] = (PceDisplayFrame){0};
    }
}

MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime) {
    if (InitPCE(22050, true) != 0) return mia_core_error(MIA_CORE_ERR_CALLBACK, "PCE core init failed");
    if (has_zip_extension(runtime->selection.rom_path)) {
        uint8_t *data = NULL;
        size_t size = 0;
        static const char *const extensions[] = {"pce"};
        MiaCoreStatus status = mia_app_zip_extract(
            runtime->selection.rom_path, extensions, 1u, PCE_ROM_MAX_SIZE,
            &data, &size, NULL, 0u);
        if (status.code != MIA_CORE_OK || size < PCE_ROM_MIN_SIZE) {
            free(data);
            return mia_core_error(MIA_CORE_ERR_CALLBACK, "PCE ZIP corrupt or unsupported");
        }
        if (LoadCard(data, size) != 0) {
            free(data);
            return mia_core_error(MIA_CORE_ERR_CALLBACK, "PCE ROM corrupt or unsupported");
        }
    } else if (LoadFile(runtime->selection.rom_path) != 0) {
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "PCE ROM corrupt or unsupported");
    }
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
    (void)runtime;
    exit_requested = false;
    if (!start_runtime_tasks()) {
        stop_runtime_tasks();
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "PCE runtime task start failed");
    }
    RunPCE();
    stop_runtime_tasks();
    if (!exit_requested) return mia_core_error(MIA_CORE_ERR_CALLBACK, "PCE core stopped unexpectedly");
    return mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CLEAN_EXIT, true);
}
