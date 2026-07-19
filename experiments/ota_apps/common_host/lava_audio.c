#include "lava_audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mia_host_abi.h"
#include "lava_rix.h"

#define LAVA_AUDIO_RATE 22050u
#define LAVA_AUDIO_FRAMES 512u
#define LAVA_AUDIO_MAX_VOC_BYTES (128u * 1024u)

typedef struct {
    TaskHandle_t task;
    char game_dir[320];
    volatile int music_number;
    volatile int music_loop;
    volatile int music_enabled;
    volatile int sound_enabled;
    volatile int sound_number;
    volatile uint32_t sound_sequence;
    volatile int stop_requested;
} LavaAudioState;

typedef struct {
    uint8_t *samples;
    uint32_t count;
    uint32_t rate;
    uint32_t position;
    uint32_t step;
    uint32_t sequence;
} LavaVocSound;

static LavaAudioState g_audio;

static uint32_t read_u32_le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t read_u16_le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static void release_voc(LavaVocSound *sound) {
    if (sound->samples != NULL) {
        heap_caps_free(sound->samples);
        sound->samples = NULL;
    }
    sound->count = 0;
    sound->rate = 0;
    sound->position = 0;
    sound->step = 0;
}

static int load_voc_sound(int number, LavaVocSound *sound, const char *game_dir) {
    char path[384];
    FILE *file;
    uint8_t header[26];
    uint8_t *chunk = NULL;
    uint32_t count;
    uint32_t begin;
    uint32_t end;
    uint32_t size;
    uint32_t cursor;

    release_voc(sound);
    if (number <= 0 || game_dir == NULL) return 0;

    snprintf(path, sizeof(path), "%s/VOC.MKF", game_dir);
    file = fopen(path, "rb");
    if (file == NULL) return 0;

    if (fread(&begin, sizeof(begin), 1, file) != 1 || begin < 8 || (begin & 3u) != 0) {
        fclose(file);
        return 0;
    }
    count = begin / 4u;
    if ((uint32_t)number + 1u >= count) {
        fclose(file);
        return 0;
    }

    if (fseek(file, (long)number * 4L, SEEK_SET) != 0 ||
        fread(&begin, sizeof(begin), 1, file) != 1 ||
        fread(&end, sizeof(end), 1, file) != 1) {
        fclose(file);
        return 0;
    }
    if (end <= begin || end - begin > LAVA_AUDIO_MAX_VOC_BYTES) {
        fclose(file);
        return 0;
    }

    size = end - begin;
    chunk = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (chunk == NULL) chunk = heap_caps_malloc(size, MALLOC_CAP_8BIT);
    if (chunk == NULL || fseek(file, (long)begin, SEEK_SET) != 0 ||
        fread(chunk, 1, size, file) != size) {
        if (chunk != NULL) heap_caps_free(chunk);
        fclose(file);
        return 0;
    }
    fclose(file);

    if (size < sizeof(header) || memcmp(chunk, "Creative Voice File", 19) != 0) {
        heap_caps_free(chunk);
        return 0;
    }
    memcpy(header, chunk, sizeof(header));
    cursor = read_u16_le(header + 20);
    if (cursor < 26 || cursor >= size) cursor = 26;

    while (cursor + 4 <= size) {
        uint8_t block_type = chunk[cursor++];
        uint32_t block_size = (uint32_t)chunk[cursor] |
                              ((uint32_t)chunk[cursor + 1] << 8) |
                              ((uint32_t)chunk[cursor + 2] << 16);
        cursor += 3;
        if (block_type == 0) break;
        if (block_size > size - cursor) break;

        if ((block_type == 1 || block_type == 9) && block_size >= 2) {
            uint32_t rate;
            uint8_t codec;
            uint32_t payload_offset;
            uint32_t payload_size;
            if (block_type == 1) {
                uint8_t time_constant = chunk[cursor];
                codec = chunk[cursor + 1];
                rate = time_constant < 255 ?
                       1000000u / (256u - time_constant) : 0;
                payload_offset = cursor + 2;
                payload_size = block_size - 2;
            } else {
                rate = read_u32_le(chunk + cursor);
                codec = block_size >= 12 ? chunk[cursor + 5] : 1;
                payload_offset = cursor + 12;
                payload_size = block_size >= 12 ? block_size - 12 : 0;
            }
            if (codec == 0 && rate >= 4000 && rate <= 48000 && payload_size > 0) {
                sound->samples = heap_caps_malloc(payload_size,
                                                  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                if (sound->samples == NULL)
                    sound->samples = heap_caps_malloc(payload_size, MALLOC_CAP_8BIT);
                if (sound->samples != NULL) {
                    memcpy(sound->samples, chunk + payload_offset, payload_size);
                    sound->count = payload_size;
                    sound->rate = rate;
                    sound->position = 0;
                    sound->step = (rate << 16) / LAVA_AUDIO_RATE;
                    if (sound->step == 0) sound->step = 1;
                    heap_caps_free(chunk);
                    return 1;
                }
            }
        }
        cursor += block_size;
    }

    heap_caps_free(chunk);
    return 0;
}

static void lava_audio_task(void *argument) {
    LavaVocSound sound = {};
    LavaRixPlayer *rix = NULL;
    uint32_t current_sequence = 0;
    uint32_t write_failures = 0;
    int current_music = 0;
    int16_t pcm[LAVA_AUDIO_FRAMES * 2];
    (void)argument;

    if (!mia_host_audio_open(LAVA_AUDIO_RATE, 2, 16)) {
        MiaHostAudioStatus status = {};
        mia_host_audio_get_status(&status);
        printf("[LAVA][AUDIO] open failed rate=%u channels=%u bits=%u err=%ld\n",
               (unsigned)status.sample_rate, (unsigned)status.channels,
               (unsigned)status.bits_per_sample,
               (long)status.last_error);
        g_audio.task = NULL;
        vTaskDelete(NULL);
        return;
    }
    printf("[LAVA][AUDIO] opened rate=%u channels=2 bits=16\n", LAVA_AUDIO_RATE);

    for (;;) {
        if (g_audio.stop_requested) break;
        const int music_number = g_audio.music_enabled ? g_audio.music_number : 0;
        const uint32_t sequence = g_audio.sound_sequence;
        const int sound_number = g_audio.sound_number;
        if (sequence != current_sequence) {
            current_sequence = sequence;
            const int loaded = load_voc_sound(sound_number, &sound, g_audio.game_dir);
            printf("[LAVA][AUDIO] sound=%d loaded=%d samples=%u rate=%u\n",
                   sound_number, loaded, (unsigned)sound.count, (unsigned)sound.rate);
            sound.sequence = sequence;
        }
        if (music_number != current_music) {
            current_music = music_number;
            if (rix != NULL) {
                lava_rix_close(rix);
                rix = NULL;
            }
            if (current_music != 0 && g_audio.game_dir[0] != '\0') {
                char path[384];
                snprintf(path, sizeof(path), "%s/MUS.MKF", g_audio.game_dir);
                rix = lava_rix_open(path, current_music, LAVA_AUDIO_RATE);
                printf("[LAVA][AUDIO] rix=%d opened=%d\n", current_music, rix != NULL);
            }
        }

        memset(pcm, 0, sizeof(pcm));
        if (rix != NULL)
            lava_rix_render(rix, pcm, LAVA_AUDIO_FRAMES, g_audio.music_loop);
        for (uint32_t frame = 0; frame < LAVA_AUDIO_FRAMES; ++frame) {
            int32_t value = pcm[frame * 2];
            if (sound.samples != NULL && sound.position < (sound.count << 16)) {
                const uint32_t index = sound.position >> 16;
                value += ((int32_t)sound.samples[index] - 128) * 220;
                sound.position += sound.step;
            }
            if (value > 32767) value = 32767;
            if (value < -32768) value = -32768;
            pcm[frame * 2] = (int16_t)value;
            pcm[frame * 2 + 1] = (int16_t)value;
        }
        if (mia_host_audio_write_pcm16(pcm, LAVA_AUDIO_FRAMES, 2) < 0) {
            if (write_failures++ == 0) {
                MiaHostAudioStatus status = {};
                mia_host_audio_get_status(&status);
                printf("[LAVA][AUDIO] write failed err=%ld\n", (long)status.last_error);
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (rix != NULL) lava_rix_close(rix);
    release_voc(&sound);
    mia_host_audio_close();
    g_audio.task = NULL;
    vTaskDelete(NULL);
}

void lava_audio_start(const char *game_dir) {
    if (g_audio.task != NULL) return;
    memset(&g_audio, 0, sizeof(g_audio));
    g_audio.music_enabled = 1;
    g_audio.sound_enabled = 1;
    if (game_dir != NULL) {
        strncpy(g_audio.game_dir, game_dir, sizeof(g_audio.game_dir) - 1);
        g_audio.game_dir[sizeof(g_audio.game_dir) - 1] = '\0';
    }
    printf("[LAVA][AUDIO] start dir=%s\n", g_audio.game_dir);
    if (xTaskCreatePinnedToCore(lava_audio_task, "lava_audio", 6144, NULL, 4,
                                &g_audio.task, 0) != pdPASS) {
        printf("[LAVA][AUDIO] task create failed\n");
        g_audio.task = NULL;
    }
}

void lava_audio_stop(void) {
    if (g_audio.task == NULL) return;
    g_audio.stop_requested = 1;
    while (g_audio.task != NULL) vTaskDelay(pdMS_TO_TICKS(1));
}

void lava_audio_set_music(int number, int loop, int fade_time) {
    (void)fade_time;
    g_audio.music_number = number;
    g_audio.music_loop = loop;
    printf("[LAVA][AUDIO] music=%d loop=%d fade=%d\n", number, loop, fade_time);
}

void lava_audio_stop_music(void) {
    g_audio.music_number = 0;
}

void lava_audio_play_sound(int number) {
    if (!g_audio.sound_enabled) return;
    g_audio.sound_number = number;
    ++g_audio.sound_sequence;
    printf("[LAVA][AUDIO] request sound=%d\n", number);
}

void lava_audio_enable_music(int enabled) {
    g_audio.music_enabled = enabled ? 1 : 0;
    if (!enabled) g_audio.music_number = 0;
}

void lava_audio_enable_sound(int enabled) {
    g_audio.sound_enabled = enabled ? 1 : 0;
}
