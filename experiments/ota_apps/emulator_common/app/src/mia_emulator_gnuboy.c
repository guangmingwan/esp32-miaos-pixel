#include "mia_emulator_runtime.h"
#include "mia_app_zip.h"
#include "gnuboy.h"
#include "mia_host_abi.h"
#ifdef MIA_EMULATOR_GNUBOY_ASPECT_REGION
#include "display_host.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define GNUBOY_ROM_MAX_SIZE (8u * 1024u * 1024u)

static uint8_t *zip_rom_data;

static bool has_extension(const char *path, const char *extension) {
    const char *dot = strrchr(path, '.');
    return dot != NULL && strcasecmp(dot + 1, extension) == 0;
}

#if defined(MIA_EMULATOR_DUAL_CORE_AUDIO) || defined(MIA_EMULATOR_GNUBOY_DISPLAY_TASK)
#undef IRAM_ATTR
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#endif

#ifdef MIA_EMULATOR_DUAL_CORE_AUDIO
#include <freertos/ringbuf.h>
#include <stdatomic.h>

#define GBC_AUDIO_RING_BYTES (16u * 1024u)
#define GBC_AUDIO_PREBUFFER_BYTES (8u * 1024u)

static RingbufHandle_t audio_ring;
static TaskHandle_t audio_task_handle;
static volatile bool audio_running;
static atomic_size_t audio_queued_bytes;

static void subtract_queued_bytes(size_t bytes) {
    size_t queued = atomic_load(&audio_queued_bytes);
    while (!atomic_compare_exchange_weak(&audio_queued_bytes, &queued, queued > bytes ? queued - bytes : 0u)) {}
}

static void audio_task(void *arg) {
    MiaEmulatorRuntime *runtime = arg;
    bool buffered = false;
    while (audio_running) {
        if (!buffered) {
            const size_t used = atomic_load(&audio_queued_bytes);
            if (used < GBC_AUDIO_PREBUFFER_BYTES) {
                vTaskDelay(pdMS_TO_TICKS(2));
                continue;
            }
            buffered = true;
        }
        size_t bytes = 0;
        int16_t *samples = xRingbufferReceive(audio_ring, &bytes, pdMS_TO_TICKS(20));
        if (samples == NULL) {
            buffered = false;
            continue;
        }
        subtract_queued_bytes(bytes);
        MiaCoreStatus status = mia_core_adapter_submit_audio(&runtime->adapter, samples, bytes / (2u * sizeof(int16_t)));
        vRingbufferReturnItem(audio_ring, samples);
        if (status.code != MIA_CORE_OK) mia_host_log(status.message);
    }
    audio_task_handle = NULL;
    vTaskDelete(NULL);
}

static bool start_audio_task(MiaEmulatorRuntime *runtime) {
    audio_ring = xRingbufferCreate(GBC_AUDIO_RING_BYTES, RINGBUF_TYPE_NOSPLIT);
    if (audio_ring == NULL) return false;
    atomic_store(&audio_queued_bytes, 0u);
    audio_running = true;
    if (xTaskCreatePinnedToCore(audio_task, "gbc_audio", 4096, runtime, 6, &audio_task_handle, 0) != pdPASS) {
        audio_running = false;
        vRingbufferDelete(audio_ring);
        audio_ring = NULL;
        return false;
    }
    return true;
}

static void stop_audio_task(void) {
    audio_running = false;
    for (unsigned wait = 0; audio_task_handle != NULL && wait < 60u; ++wait) vTaskDelay(pdMS_TO_TICKS(5));
    if (audio_task_handle != NULL) {
        vTaskDelete(audio_task_handle);
        audio_task_handle = NULL;
    }
    vRingbufferDelete(audio_ring);
    audio_ring = NULL;
}
#endif

#ifdef MIA_EMULATOR_GNUBOY_DISPLAY_TASK
#include <esp_heap_caps.h>

#define GNUBOY_FRAME_PIXELS (160u * 144u)
#define GNUBOY_DISPLAY_BUFFER_COUNT 3u

static QueueHandle_t display_ready_queue;
static QueueHandle_t display_free_queue;
static TaskHandle_t display_task_handle;
static volatile bool display_running;
static uint16_t *display_buffers[GNUBOY_DISPLAY_BUFFER_COUNT];

static void display_task(void *arg) {
    (void)arg;
    while (display_running || uxQueueMessagesWaiting(display_ready_queue) != 0u) {
        uint16_t *frame = NULL;
        if (xQueueReceive(display_ready_queue, &frame, pdMS_TO_TICKS(20)) != pdTRUE) continue;
        if (display_host_present_rgb565_scaled_region(frame, 160, 144, 160 * sizeof(uint16_t),
                                                      27, 0, 266, 240) != MIA_HOST_RESULT_OK) {
            mia_host_log("Gnuboy video presentation failed");
        }
        (void)xQueueSend(display_free_queue, &frame, portMAX_DELAY);
    }
    display_task_handle = NULL;
    vTaskDelete(NULL);
}

static bool start_display_task(void) {
    display_ready_queue = xQueueCreate(1, sizeof(uint16_t *));
    display_free_queue = xQueueCreate(GNUBOY_DISPLAY_BUFFER_COUNT, sizeof(uint16_t *));
    if (display_ready_queue == NULL || display_free_queue == NULL) return false;
    for (unsigned i = 0; i < GNUBOY_DISPLAY_BUFFER_COUNT; ++i) {
        display_buffers[i] = heap_caps_malloc(GNUBOY_FRAME_PIXELS * sizeof(uint16_t),
                                              MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (display_buffers[i] == NULL) return false;
        (void)xQueueSend(display_free_queue, &display_buffers[i], 0);
    }
    display_running = true;
    if (xTaskCreatePinnedToCore(display_task, "gb_display", 4096, NULL, 5,
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
    for (unsigned i = 0; i < GNUBOY_DISPLAY_BUFFER_COUNT; ++i) {
        heap_caps_free(display_buffers[i]);
        display_buffers[i] = NULL;
    }
}
#endif

static void video_callback(void *buffer) {
    if (buffer == NULL) return;
#ifdef MIA_EMULATOR_GNUBOY_DISPLAY_TASK
    uint16_t *frame = NULL;
    if (xQueueReceive(display_free_queue, &frame, 0) != pdTRUE) {
        (void)xQueueReceive(display_ready_queue, &frame, 0);
    }
    if (frame == NULL) return;
    memcpy(frame, buffer, GNUBOY_FRAME_PIXELS * sizeof(uint16_t));
    uint16_t *replaced = NULL;
    if (xQueueReceive(display_ready_queue, &replaced, 0) == pdTRUE) {
        (void)xQueueSend(display_free_queue, &replaced, 0);
    }
    if (xQueueSend(display_ready_queue, &frame, 0) != pdTRUE) {
        (void)xQueueSend(display_free_queue, &frame, 0);
    }
#elif defined(MIA_EMULATOR_GNUBOY_ASPECT_REGION)
    enum { SOURCE_WIDTH = 160, SOURCE_HEIGHT = 144, OUTPUT_WIDTH = 266, OUTPUT_HEIGHT = 240 };
    if (display_host_present_rgb565_scaled_region(
            buffer, SOURCE_WIDTH, SOURCE_HEIGHT, SOURCE_WIDTH * sizeof(uint16_t),
            (320 - OUTPUT_WIDTH) / 2, 0, OUTPUT_WIDTH, OUTPUT_HEIGHT) !=
        MIA_HOST_RESULT_OK) {
        mia_host_log("Gnuboy video presentation failed");
    }
#else
    (void)mia_core_adapter_submit_video(&mia_emulator_runtime.adapter, buffer, 160u * 144u);
#endif
}

static void audio_callback(void *buffer, size_t length) {
    if (buffer == NULL) return;
#ifdef MIA_EMULATOR_DUAL_CORE_AUDIO
    if (audio_ring != NULL) {
        const size_t bytes = length * sizeof(int16_t);
        if (xRingbufferSend(audio_ring, buffer, bytes, portMAX_DELAY) == pdTRUE) {
            atomic_fetch_add(&audio_queued_bytes, bytes);
        } else {
            mia_host_log("Gnuboy audio queue stopped");
        }
    }
#else
    (void)mia_core_adapter_submit_audio(&mia_emulator_runtime.adapter, buffer, length / 2u);
#endif
}

static MiaCoreStatus load_save(MiaEmulatorRuntime *runtime) {
    const size_t capacity = gnuboy_sram_size();
    if (capacity == 0) return mia_core_ok();
    uint8_t *data = malloc(capacity);
    if (data == NULL) return mia_core_error(MIA_CORE_ERR_CALLBACK, "SRAM allocation failed");
    size_t size = 0;
    MiaStorageStatus status = mia_app_save_load(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, data, capacity, &size);
    if (status.code == MIA_STORAGE_ERR_MISSING_REQUIRED_FILE) {
        free(data);
        return mia_core_ok();
    }
    const int imported = status.code == MIA_STORAGE_OK ? gnuboy_import_sram(data, size) : -1;
    free(data);
    return imported == 0 ? mia_core_ok() : mia_core_error(MIA_CORE_ERR_CALLBACK, "SRAM load failed");
}

MiaCoreStatus mia_emulator_core_boot(MiaEmulatorRuntime *runtime) {
    static uint16_t framebuffer[160 * 144];
    static int16_t soundbuffer[1024 * 2];
#ifdef MIA_DISPLAY_RGB565_WIRE_ORDER
    const gb_video_fmt_t pixel_format = GB_PIXEL_565_BE;
#else
    const gb_video_fmt_t pixel_format = GB_PIXEL_565_LE;
#endif
    if (gnuboy_init(MIA_EMULATOR_SAMPLE_RATE, GB_AUDIO_STEREO_S16, pixel_format, video_callback, audio_callback) < 0) return mia_core_error(MIA_CORE_ERR_CALLBACK, "gnuboy init failed");
    gnuboy_set_framebuffer(framebuffer);
    gnuboy_set_soundbuffer(soundbuffer, 1024);
    if (has_extension(runtime->selection.rom_path, "zip")) {
        static const char *const extensions[] = {"gb", "gbc"};
        size_t size = 0;
        MiaCoreStatus status = mia_app_zip_extract(runtime->selection.rom_path,
            extensions, 2u, GNUBOY_ROM_MAX_SIZE, &zip_rom_data, &size, NULL, 0u);
        if (status.code != MIA_CORE_OK || gnuboy_load_rom(zip_rom_data, size) < 0)
            return mia_core_error(MIA_CORE_ERR_CALLBACK, "gnuboy ZIP load failed");
    } else if (gnuboy_load_rom_file(runtime->selection.rom_path) < 0) {
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "gnuboy ROM load failed");
    }
#ifdef MIA_EMULATOR_DEFAULT_PALETTE
    gnuboy_set_palette(MIA_EMULATOR_DEFAULT_PALETTE);
#endif
#ifdef MIA_EMULATOR_GNUBOY_ASPECT_REGION
    mia_host_fill_screen_rgb565(0);
#endif
    gnuboy_reset(true);
    return load_save(runtime);
}

MiaCoreStatus mia_emulator_core_flush(MiaEmulatorRuntime *runtime, MiaStorageFlushReason reason, bool force) {
    if (!force && !gnuboy_sram_dirty()) return mia_core_ok();
    const size_t capacity = gnuboy_sram_size();
    if (capacity == 0) return mia_core_ok();
    uint8_t *data = malloc(capacity);
    if (data == NULL) return mia_core_error(MIA_CORE_ERR_CALLBACK, "SRAM allocation failed");
    const int size = gnuboy_export_sram(data, capacity);
    MiaStorageStatus status = size > 0 ? mia_app_save_flush(&runtime->storage, &runtime->storage_target, runtime->selection.save_name, reason, data, (size_t)size, NULL) : mia_storage_error(MIA_STORAGE_ERR_IO, "SRAM export failed");
    free(data);
    if (status.code != MIA_STORAGE_OK) return mia_core_error(MIA_CORE_ERR_CALLBACK, status.message);
    gnuboy_mark_sram_saved();
    return mia_core_ok();
}

MiaCoreStatus mia_emulator_core_save_state(MiaEmulatorRuntime *runtime, const char *path) {
    (void)runtime;
    return gnuboy_save_state(path) == 0 ? mia_core_ok() :
        mia_core_error(MIA_CORE_ERR_CALLBACK, "Gnuboy state save failed");
}

MiaCoreStatus mia_emulator_core_load_state(MiaEmulatorRuntime *runtime, const char *path) {
    (void)runtime;
    return gnuboy_load_state(path) == 0 ? mia_core_ok() :
        mia_core_error(MIA_CORE_ERR_CALLBACK, "Gnuboy state load failed");
}

MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime) {
#ifdef MIA_EMULATOR_GNUBOY_DISPLAY_TASK
    if (!start_display_task()) {
        stop_display_task();
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "Gnuboy display task start failed");
    }
#endif
#ifdef MIA_EMULATOR_DUAL_CORE_AUDIO
    if (!start_audio_task(runtime)) {
#ifdef MIA_EMULATOR_GNUBOY_DISPLAY_TASK
        stop_display_task();
#endif
        return mia_core_error(MIA_CORE_ERR_CALLBACK, "GBC audio task start failed");
    }
#endif
    unsigned frames = 0;
#ifdef MIA_EMULATOR_PERF_LOG
    uint32_t report_started = mia_host_millis();
#endif
    for (;;) {
        uint32_t input = 0;
        MiaCoreStatus status = mia_core_adapter_poll_input(&runtime->adapter, &input);
        if (status.code != MIA_CORE_OK) {
#ifdef MIA_EMULATOR_DUAL_CORE_AUDIO
            stop_audio_task();
#endif
#ifdef MIA_EMULATOR_GNUBOY_DISPLAY_TASK
            stop_display_task();
#endif
            return status;
        }
        if (mia_app_input_menu_requested(&runtime->input, mia_emulator_host_buttons())) {
#ifdef MIA_EMULATOR_DUAL_CORE_AUDIO
            stop_audio_task();
#endif
#ifdef MIA_EMULATOR_GNUBOY_DISPLAY_TASK
            stop_display_task();
#endif
            return mia_core_ok();
        }
        gnuboy_set_pad((int)mia_app_input_gnuboy_mask(input));
#ifdef MIA_EMULATOR_GNUBOY_DISPLAY_TASK
        gnuboy_run(true);
#else
        gnuboy_run((frames & 1u) == 0u);
#endif
        if (++frames % 60u == 0u) {
            status = mia_emulator_core_flush(runtime, MIA_STORAGE_FLUSH_CORE_REQUEST, false);
            if (status.code != MIA_CORE_OK) mia_host_log(status.message);
        }
#ifdef MIA_EMULATOR_PERF_LOG
        if (frames % 300u == 0u) {
            const uint32_t now = mia_host_millis();
            const uint32_t elapsed = now - report_started;
            char message[96];
            snprintf(message, sizeof(message), "GBC perf fps=%lu.%lu audio=%u",
                     (unsigned long)(elapsed == 0 ? 0 : 300000u / elapsed),
                     (unsigned long)(elapsed == 0 ? 0 : (3000000u / elapsed) % 10u),
                     (unsigned)atomic_load(&audio_queued_bytes));
            mia_host_log(message);
            report_started = now;
        }
#endif
    }
}
