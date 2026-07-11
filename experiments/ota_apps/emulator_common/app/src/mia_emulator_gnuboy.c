#include "mia_emulator_runtime.h"
#include "gnuboy.h"
#include "mia_host_abi.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef MIA_EMULATOR_DUAL_CORE_AUDIO
#undef IRAM_ATTR
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>
#include <freertos/task.h>
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

static void video_callback(void *buffer) {
    if (buffer != NULL) (void)mia_core_adapter_submit_video(&mia_emulator_runtime.adapter, buffer, 160u * 144u);
}

static void audio_callback(void *buffer, size_t length) {
    if (buffer == NULL) return;
#ifdef MIA_EMULATOR_DUAL_CORE_AUDIO
    if (audio_ring != NULL) {
        const size_t bytes = length * sizeof(int16_t);
        if (xRingbufferSend(audio_ring, buffer, bytes, pdMS_TO_TICKS(20)) == pdTRUE) {
            atomic_fetch_add(&audio_queued_bytes, bytes);
        } else {
            mia_host_log("GBC audio ring full");
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
    if (gnuboy_init(MIA_EMULATOR_SAMPLE_RATE, GB_AUDIO_STEREO_S16, GB_PIXEL_565_LE, video_callback, audio_callback) < 0) return mia_core_error(MIA_CORE_ERR_CALLBACK, "gnuboy init failed");
    gnuboy_set_framebuffer(framebuffer);
    gnuboy_set_soundbuffer(soundbuffer, 1024);
    if (gnuboy_load_rom_file(runtime->selection.rom_path) < 0) return mia_core_error(MIA_CORE_ERR_CALLBACK, "gnuboy ROM load failed");
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

MiaCoreStatus mia_emulator_core_run(MiaEmulatorRuntime *runtime) {
#ifdef MIA_EMULATOR_DUAL_CORE_AUDIO
    if (!start_audio_task(runtime)) return mia_core_error(MIA_CORE_ERR_CALLBACK, "GBC audio task start failed");
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
            return status;
        }
        if (mia_app_input_exit_requested(&runtime->input, mia_emulator_host_buttons(), mia_host_millis())) {
#ifdef MIA_EMULATOR_DUAL_CORE_AUDIO
            stop_audio_task();
#endif
            return mia_core_ok();
        }
        gnuboy_set_pad((int)mia_app_input_gnuboy_mask(input));
        gnuboy_run((frames & 1u) == 0u);
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
