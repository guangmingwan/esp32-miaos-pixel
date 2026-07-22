#include "audio.h"
#include "ringbuffer.h"
#include "mia_host_abi.h"

#include <pthread.h>
#include <stdint.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define GMU_AUDIO_RING_BYTES (256 * 1024)
#define GMU_AUDIO_CHUNK_BYTES 4096

static RingBuffer ring;
static pthread_mutex_t ring_mutex;
static pthread_mutex_t state_mutex;
static TaskHandle_t audio_task_handle;
static int device_open;
static int paused = 1;
static int sample_rate = 44100;
static int channels = 2;
static size_t sample_counter;
static unsigned volume = AUDIO_MAX_SW_VOLUME - 1;
static int16_t *pcm;
static int16_t *scaled;
static int16_t amplitudes[16];
static pthread_mutex_t spectrum_mutex;

static void update_spectrum(const int16_t *samples, size_t frames, int local_channels) {
    static const int32_t coefficients[8] = {
        60547, 46341, 25080, 0, -25080, -46341, -60547, -65536,
    };
    int16_t next[8] = {};

    if (frames < 16) return;
    for (size_t band = 0; band < 8; ++band) {
        int64_t s1 = 0;
        int64_t s2 = 0;
        const int32_t coefficient = coefficients[band];
        for (size_t sample = 0; sample < 16; ++sample) {
            int32_t value = samples[sample * (size_t)local_channels];
            if (local_channels == 2)
                value = (value + samples[sample * 2 + 1]) / 2;
            value >>= 4;
            int64_t s0 = value + ((int64_t)coefficient * s1 >> 15) - s2;
            s2 = s1;
            s1 = s0;
        }
        int64_t magnitude = s1 < 0 ? -s1 : s1;
        magnitude += s2 < 0 ? -s2 : s2;
        magnitude *= 2;
        next[band] = magnitude > 32767 ? 32767 : (int16_t)magnitude;
    }

    pthread_mutex_lock(&spectrum_mutex);
    memcpy(amplitudes, next, sizeof(next));
    pthread_mutex_unlock(&spectrum_mutex);
}

static void audio_task(void *arg) {
    (void)arg;

    while (1) {
        pthread_mutex_lock(&state_mutex);
        int open = device_open;
        int is_paused = paused;
        int local_channels = channels;
        unsigned local_volume = volume;
        pthread_mutex_unlock(&state_mutex);

        if (!open || is_paused || local_channels <= 0) {
            vTaskDelay(pdMS_TO_TICKS(4));
            continue;
        }

        pthread_mutex_lock(&ring_mutex);
        size_t available = ringbuffer_get_fill(&ring);
        size_t want = available > GMU_AUDIO_CHUNK_BYTES ? GMU_AUDIO_CHUNK_BYTES : available;
        if (want >= (size_t)local_channels * sizeof(int16_t)) {
            want -= want % ((size_t)local_channels * sizeof(int16_t));
            ringbuffer_read(&ring, (char *)pcm, want);
        }
        pthread_mutex_unlock(&ring_mutex);

        if (want == 0) {
            vTaskDelay(pdMS_TO_TICKS(2));
            continue;
        }

        uint32_t frames = (uint32_t)(want / ((size_t)local_channels * sizeof(int16_t)));
        update_spectrum(pcm, frames, local_channels);
        size_t sample_count = want / sizeof(int16_t);
        for (size_t i = 0; i < sample_count; ++i)
            scaled[i] = (int16_t)(((int32_t)pcm[i] * (int32_t)local_volume) /
                                  (int32_t)(AUDIO_MAX_SW_VOLUME - 1));
        if (mia_host_audio_write_pcm16(scaled, frames, (uint8_t)local_channels) >= 0) {
            pthread_mutex_lock(&state_mutex);
            sample_counter += frames;
            pthread_mutex_unlock(&state_mutex);
        }
    }
}

int audio_fill_buffer(char *data, size_t size) {
    int result;
    pthread_mutex_lock(&ring_mutex);
    result = ringbuffer_write(&ring, data, size);
    pthread_mutex_unlock(&ring_mutex);
    return result;
}

int audio_device_open(int requested_rate, int requested_channels) {
    if (requested_channels < 1 || requested_channels > 2) return -1;
    pthread_mutex_lock(&state_mutex);
    if (!device_open || sample_rate != requested_rate || channels != requested_channels) {
        if (!mia_host_audio_open((uint32_t)requested_rate, (uint8_t)requested_channels, 16)) {
            pthread_mutex_unlock(&state_mutex);
            return -1;
        }
        sample_rate = requested_rate;
        channels = requested_channels;
        device_open = 1;
        paused = 1;
        sample_counter = 0;
        pthread_mutex_lock(&ring_mutex);
        ringbuffer_clear(&ring);
        pthread_mutex_unlock(&ring_mutex);
    }
    pthread_mutex_unlock(&state_mutex);
    return 0;
}

size_t audio_get_playtime(void) {
    pthread_mutex_lock(&state_mutex);
    size_t result = sample_rate > 0 ? sample_counter * 1000u / (size_t)sample_rate : 0;
    pthread_mutex_unlock(&state_mutex);
    return result;
}

void audio_buffer_init(void) {
    ringbuffer_init(&ring, GMU_AUDIO_RING_BYTES);
    pthread_mutex_init(&ring_mutex, NULL);
    pthread_mutex_init(&state_mutex, NULL);
    pthread_mutex_init(&spectrum_mutex, NULL);
    audio_task_handle = NULL;
    device_open = 0;
    paused = 1;
    pcm = heap_caps_malloc(GMU_AUDIO_CHUNK_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    scaled = heap_caps_malloc(GMU_AUDIO_CHUNK_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (pcm != NULL && scaled != NULL &&
        xTaskCreatePinnedToCore(audio_task, "gmu_audio", 8192, NULL, 6,
                                &audio_task_handle, 0) != pdPASS)
        audio_task_handle = NULL;
}

void audio_buffer_clear(void) {
    pthread_mutex_lock(&ring_mutex);
    ringbuffer_clear(&ring);
    pthread_mutex_unlock(&ring_mutex);
    audio_set_pause(1);
}

void audio_buffer_free(void) {
    if (audio_task_handle != NULL) vTaskDelete(audio_task_handle);
    mia_host_audio_close();
    ringbuffer_free(&ring);
    pthread_mutex_destroy(&ring_mutex);
    pthread_mutex_destroy(&state_mutex);
    pthread_mutex_destroy(&spectrum_mutex);
    heap_caps_free(pcm);
    heap_caps_free(scaled);
    pcm = NULL;
    scaled = NULL;
}

void audio_device_close(void) {
    pthread_mutex_lock(&state_mutex);
    device_open = 0;
    paused = 1;
    pthread_mutex_unlock(&state_mutex);
    mia_host_audio_close();
}

int audio_get_status(void) {
    pthread_mutex_lock(&state_mutex);
    int result = device_open && !paused;
    pthread_mutex_unlock(&state_mutex);
    return result;
}

void audio_force_pause(int pause_state) {
    pthread_mutex_lock(&state_mutex);
    paused = pause_state != 0;
    pthread_mutex_unlock(&state_mutex);
    if (pause_state) mia_host_audio_stop();
}

int audio_set_pause(int pause_state) {
    audio_force_pause(pause_state);
    return pause_state != 0;
}

int audio_get_pause(void) {
    pthread_mutex_lock(&state_mutex);
    int result = paused;
    pthread_mutex_unlock(&state_mutex);
    return result;
}

void audio_set_volume(unsigned int vol) {
    pthread_mutex_lock(&state_mutex);
    volume = vol >= AUDIO_MAX_SW_VOLUME ? AUDIO_MAX_SW_VOLUME - 1 : vol;
    pthread_mutex_unlock(&state_mutex);
}

unsigned int audio_get_volume(void) {
    pthread_mutex_lock(&state_mutex);
    unsigned result = volume;
    pthread_mutex_unlock(&state_mutex);
    return result;
}

size_t audio_buffer_get_fill(void) {
    pthread_mutex_lock(&ring_mutex);
    size_t result = ringbuffer_get_fill(&ring);
    pthread_mutex_unlock(&ring_mutex);
    return result;
}

size_t audio_buffer_get_size(void) { return GMU_AUDIO_RING_BYTES; }
size_t audio_set_sample_counter(size_t sample) { sample_counter = sample * (size_t)channels; return sample_counter; }
size_t audio_increase_sample_counter(size_t offset) { sample_counter += offset; return sample_counter; }
size_t audio_get_sample_count(void) { return sample_counter; }
void audio_set_done(void) {}
void audio_set_fade_volume(unsigned int percent) { (void)percent; }
int audio_fade_out_step(unsigned int step_size) { (void)step_size; return 0; }
void audio_reset_fade_volume(void) {}
int audio_fade_out_in_progress(void) { return 0; }
int16_t *audio_spectrum_get_current_amplitudes(void) { return amplitudes; }
void audio_spectrum_register_for_access(void) {}
void audio_spectrum_unregister(void) {}
int audio_spectrum_read_lock(void) { return pthread_mutex_lock(&spectrum_mutex) == 0; }
void audio_spectrum_read_unlock(void) { pthread_mutex_unlock(&spectrum_mutex); }
