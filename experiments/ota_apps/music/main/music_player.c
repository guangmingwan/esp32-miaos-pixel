#include "music_player.h"

#include "mia_host_abi.h"
#include "music_i18n.h"

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/stream_buffer.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_system.h>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include "third_party/minimp3_ex.h"

#define DR_WAV_IMPLEMENTATION
#include "third_party/dr_wav.h"

#define DR_FLAC_IMPLEMENTATION
#include "third_party/dr_flac.h"

/* Header-only: implementation lives in third_party/stb_vorbis_impl.c so its
 * static helpers don't collide with minimp3's. */
#define STB_VORBIS_HEADER_ONLY
#include "third_party/stb_vorbis.c"

/* Decoded PCM chunk: ~52 ms of stereo 44.1 kHz audio. Larger chunk reduces
 * SD card read frequency (main cause of MP3 stutter) and amortizes decode cost. */
#define PCM_FRAMES_PER_CHUNK 2304
#define PCM_SAMPLES_PER_CHUNK (PCM_FRAMES_PER_CHUNK * 2)

#define FFT_SIZE 128
#define SPECTRUM_BARS 20

static const char *TAG = "music_player";

enum MusicFormat {
  MUSIC_FORMAT_UNKNOWN = 0,
  MUSIC_FORMAT_MP3,
  MUSIC_FORMAT_WAV,
  MUSIC_FORMAT_FLAC,
  MUSIC_FORMAT_OGG,
};

/* ---- Dual-core shared state (audio task on core 0, UI on core 1) ---- */

/* Audio path copied here before starting the audio task */
static char s_audio_path[256];
static volatile enum MusicFormat s_audio_format = MUSIC_FORMAT_UNKNOWN;

/* Control flags */
static volatile bool s_audio_open_ok = false;   /* latch: set once on successful file open */
static volatile bool s_audio_stop = false;
static volatile bool s_audio_running = false;
static volatile bool s_audio_exit = false;
static volatile TaskHandle_t s_audio_task_handle = NULL;

/* ---- Ring buffer (PSRAM): decouples SD-card-bound decode from I2S output ----
 * decode_task (priority 5) fills the ring; audio_task (priority 6) drains it
 * into I2S. When the decoder stalls on a slow SD read (minimp3's 128 KB IO
 * refill, FAT cluster lookups), the ring buffer keeps I2S fed for ~2 seconds. */
static StreamBufferHandle_t s_audio_stream = NULL;
static StaticStreamBuffer_t s_stream_obj;        /* metadata in internal RAM */
static uint8_t *s_stream_storage = NULL;         /* data in PSRAM */
static size_t s_stream_size = 0;
static volatile bool s_decode_eof = false;       /* set by decode task on EOF/error */
static TaskHandle_t s_decode_task_handle = NULL;
static int s_playback_sample_rate = 0;
static uint8_t s_playback_channels = 0;

/* PCM double-buffer for FFT data (audio -> UI) */
#define FFT_SHARED_SAMPLES 256
static int16_t s_fft_shared[FFT_SHARED_SAMPLES];
static volatile uint8_t s_fft_channels = 0;
static portMUX_TYPE s_fft_mux = portMUX_INITIALIZER_UNLOCKED;

/* ---- Static buffers ----
 * Decode and feed run in separate tasks (both on core 0) so each needs its
 * own PCM scratch buffer to avoid preemption races. */
static int16_t g_decode_pcm[PCM_SAMPLES_PER_CHUNK];  /* decode_task writes */
static int16_t g_feed_pcm[PCM_SAMPLES_PER_CHUNK];    /* audio_task reads for I2S */

/* Per-format decoder state (only one active at a time). */
static mp3dec_ex_t g_mp3;
static mp3dec_io_t g_mp3_io;
static FILE *g_audio_file;
static drwav g_wav;
static drflac *g_flac;
static stb_vorbis *g_vorbis;

/* ---- Static data for UI task only (core 1), no conflict ---- */
static float g_fft_win[FFT_SIZE];

static void set_status(char *status, size_t status_size, const char *message) {
  if (status == NULL || status_size == 0) {
    return;
  }
  snprintf(status, status_size, "%s", message);
}

static char ascii_lower(char value) {
  return (value >= 'A' && value <= 'Z') ? (char)(value - 'A' + 'a') : value;
}

static enum MusicFormat detect_format(const char *path) {
  const char *ext = strrchr(path, '.');
  if (ext == NULL) {
    return MUSIC_FORMAT_UNKNOWN;
  }
  ++ext;
  char lower[8] = {0};
  size_t index = 0;
  while (ext[index] != '\0' && index + 1 < sizeof(lower)) {
    lower[index] = ascii_lower(ext[index]);
    ++index;
  }
  if (strcmp(lower, "mp3") == 0) {
    return MUSIC_FORMAT_MP3;
  }
  if (strcmp(lower, "wav") == 0 || strcmp(lower, "wave") == 0) {
    return MUSIC_FORMAT_WAV;
  }
  if (strcmp(lower, "flac") == 0) {
    return MUSIC_FORMAT_FLAC;
  }
  if (strcmp(lower, "ogg") == 0 || strcmp(lower, "oga") == 0) {
    return MUSIC_FORMAT_OGG;
  }
  return MUSIC_FORMAT_UNKNOWN;
}

static const char *to_vfs_path(const char *path) {
  static char vfs_path[256];
  if (path == NULL || path[0] == '\0') {
    return NULL;
  }
  if (strncmp(path, "/sd/", 4) == 0 || strcmp(path, "/sd") == 0) {
    return path;
  }
  if (snprintf(vfs_path, sizeof(vfs_path), "/sd%s", path) >=
      (int)sizeof(vfs_path)) {
    return NULL;
  }
  return vfs_path;
}

int music_is_supported_file(const char *name) {
  return detect_format(name) != MUSIC_FORMAT_UNKNOWN;
}

/* ---- Generic VFS read/seek/tell callbacks for dr_flac / dr_wav ---- */

static size_t vfs_read_cb(void *user_data, void *buffer, size_t bytes) {
  FILE *file = (FILE *)user_data;
  return file == NULL ? 0 : fread(buffer, 1, bytes, file);
}

/* Shared fseek wrapper. DR_*_SEEK_SET=0, _CUR=1, _END=2 — identical values. */
static int vfs_seek_common(void *user_data, int offset, int origin) {
  FILE *file = (FILE *)user_data;
  if (file == NULL) {
    return 0;
  }
  int whence;
  switch (origin) {
    case 0: whence = SEEK_SET; break;
    case 1: whence = SEEK_CUR; break;
    case 2: whence = SEEK_END; break;
    default: return 0;
  }
  return fseek(file, offset, whence) == 0 ? 1 : 0;
}

static int vfs_tell_common(void *user_data, int64_t *cursor) {
  FILE *file = (FILE *)user_data;
  if (file == NULL || cursor == NULL) {
    return 0;
  }
  long pos = ftell(file);
  if (pos < 0) {
    return 0;
  }
  *cursor = (int64_t)pos;
  return 1;
}

/* dr_wav strongly-typed callbacks */
static drwav_bool32 wav_read_cb(void *user_data, void *buffer, size_t bytes) {
  return (drwav_bool32)vfs_read_cb(user_data, buffer, bytes);
}
static drwav_bool32 wav_seek_cb(void *user_data, int offset,
                                drwav_seek_origin origin) {
  return (drwav_bool32)vfs_seek_common(user_data, offset, (int)origin);
}
static drwav_bool32 wav_tell_cb(void *user_data, drwav_int64 *cursor) {
  return (drwav_bool32)vfs_tell_common(user_data, cursor);
}

/* dr_flac strongly-typed callbacks */
static size_t flac_read_cb(void *user_data, void *buffer, size_t bytes) {
  return vfs_read_cb(user_data, buffer, bytes);
}
static drflac_bool32 flac_seek_cb(void *user_data, int offset,
                                  drflac_seek_origin origin) {
  return (drflac_bool32)vfs_seek_common(user_data, offset, (int)origin);
}
static drflac_bool32 flac_tell_cb(void *user_data, drflac_int64 *cursor) {
  return (drflac_bool32)vfs_tell_common(user_data, cursor);
}

/* MP3 read/seek wrappers (minimp3 uses uint64_t position + separate user_data) */
static size_t mp3_file_read(void *buffer, size_t size, void *user_data) {
  FILE *file = (FILE *)user_data;
  return file == NULL ? 0 : fread(buffer, 1, size, file);
}

static int mp3_file_seek(uint64_t position, void *user_data) {
  FILE *file = (FILE *)user_data;
  if (file == NULL || position > LONG_MAX) {
    return -1;
  }
  return fseek(file, (long)position, SEEK_SET);
}

/* ---- Decoder backend abstraction ----
 * Each backend exposes open/read/close. Open must populate sample_rate and
 * channels on success. Read returns frames decoded (0 = EOF), -1 on error. */

typedef struct {
  int (*open)(const char *vfs_path, int *sample_rate, uint8_t *channels);
  int (*read_pcm16)(int16_t *out, int max_frames);
  void (*close)(void);
} DecoderBackend;

/* ---- MP3 backend ---- */
static int mp3_open(const char *vfs_path, int *sample_rate, uint8_t *channels) {
  memset(&g_mp3, 0, sizeof(g_mp3));
  memset(&g_mp3_io, 0, sizeof(g_mp3_io));
  g_audio_file = fopen(vfs_path, "rb");
  if (g_audio_file == NULL) {
    ESP_LOGE(TAG, "mp3 fopen failed errno=%d", errno);
    return -1;
  }
  g_mp3_io.read = mp3_file_read;
  g_mp3_io.read_data = g_audio_file;
  g_mp3_io.seek = mp3_file_seek;
  g_mp3_io.seek_data = g_audio_file;
  if (mp3dec_ex_open_cb(&g_mp3, &g_mp3_io, MP3D_DO_NOT_SCAN) != 0) {
    ESP_LOGE(TAG, "mp3 open failed last_error=%d", g_mp3.last_error);
    mp3dec_ex_close(&g_mp3);
    fclose(g_audio_file);
    g_audio_file = NULL;
    return -1;
  }
  *sample_rate = g_mp3.info.hz;
  *channels = (uint8_t)g_mp3.info.channels;
  return 0;
}

static int mp3_read_pcm16(int16_t *out, int max_frames) {
  /* mp3dec_ex_read returns samples (channels * frames). */
  size_t samples = mp3dec_ex_read(&g_mp3, out, (size_t)max_frames * 2);
  return g_mp3.info.channels > 0 ? (int)(samples / g_mp3.info.channels) : 0;
}

static void mp3_close(void) {
  mp3dec_ex_close(&g_mp3);
  if (g_audio_file) {
    fclose(g_audio_file);
    g_audio_file = NULL;
  }
}

/* ---- WAV backend ---- */
static int wav_open(const char *vfs_path, int *sample_rate, uint8_t *channels) {
  g_audio_file = fopen(vfs_path, "rb");
  if (g_audio_file == NULL) {
    ESP_LOGE(TAG, "wav fopen failed errno=%d", errno);
    return -1;
  }
  if (!drwav_init(&g_wav, wav_read_cb, wav_seek_cb, wav_tell_cb, g_audio_file,
                  NULL)) {
    ESP_LOGE(TAG, "wav init failed");
    drwav_uninit(&g_wav);
    fclose(g_audio_file);
    g_audio_file = NULL;
    return -1;
  }
  if (g_wav.channels == 0 || g_wav.channels > 2 || g_wav.sampleRate == 0) {
    ESP_LOGE(TAG, "wav unsupported format channels=%u rate=%u",
             g_wav.channels, g_wav.sampleRate);
    drwav_uninit(&g_wav);
    fclose(g_audio_file);
    g_audio_file = NULL;
    return -1;
  }
  *sample_rate = (int)g_wav.sampleRate;
  *channels = (uint8_t)g_wav.channels;
  return 0;
}

static int wav_read_pcm16(int16_t *out, int max_frames) {
  return (int)drwav_read_pcm_frames_s16(&g_wav, (drwav_uint64)max_frames, out);
}

static void wav_close(void) {
  drwav_uninit(&g_wav);
  if (g_audio_file) {
    fclose(g_audio_file);
    g_audio_file = NULL;
  }
}

/* ---- FLAC backend ---- */
static int flac_open(const char *vfs_path, int *sample_rate, uint8_t *channels) {
  g_audio_file = fopen(vfs_path, "rb");
  if (g_audio_file == NULL) {
    ESP_LOGE(TAG, "flac fopen failed errno=%d", errno);
    return -1;
  }
  g_flac = drflac_open(flac_read_cb, flac_seek_cb, flac_tell_cb, g_audio_file, NULL);
  if (g_flac == NULL) {
    ESP_LOGE(TAG, "flac open failed");
    fclose(g_audio_file);
    g_audio_file = NULL;
    return -1;
  }
  if (g_flac->channels == 0 || g_flac->channels > 2 || g_flac->sampleRate == 0) {
    ESP_LOGE(TAG, "flac unsupported format channels=%u rate=%u",
             g_flac->channels, g_flac->sampleRate);
    drflac_close(g_flac);
    g_flac = NULL;
    fclose(g_audio_file);
    g_audio_file = NULL;
    return -1;
  }
  *sample_rate = (int)g_flac->sampleRate;
  *channels = (uint8_t)g_flac->channels;
  return 0;
}

static int flac_read_pcm16(int16_t *out, int max_frames) {
  return (int)drflac_read_pcm_frames_s16(g_flac, (drflac_uint64)max_frames, out);
}

static void flac_close(void) {
  if (g_flac) {
    drflac_close(g_flac);
    g_flac = NULL;
  }
  if (g_audio_file) {
    fclose(g_audio_file);
    g_audio_file = NULL;
  }
}

/* ---- OGG (Vorbis) backend ---- */
static int ogg_open(const char *vfs_path, int *sample_rate, uint8_t *channels) {
  int ogg_err = 0;
  /* stb_vorbis_open_filename manages its own FILE handle. */
  g_vorbis = stb_vorbis_open_filename(vfs_path, &ogg_err, NULL);
  if (g_vorbis == NULL) {
    ESP_LOGE(TAG, "ogg open failed err=%d", ogg_err);
    return -1;
  }
  stb_vorbis_info info = stb_vorbis_get_info(g_vorbis);
  if (info.channels == 0 || info.channels > 2 || info.sample_rate == 0) {
    ESP_LOGE(TAG, "ogg unsupported format channels=%d rate=%u",
             info.channels, info.sample_rate);
    stb_vorbis_close(g_vorbis);
    g_vorbis = NULL;
    return -1;
  }
  *sample_rate = (int)info.sample_rate;
  *channels = (uint8_t)info.channels;
  return 0;
}

static int ogg_read_pcm16(int16_t *out, int max_frames) {
  stb_vorbis_info info = stb_vorbis_get_info(g_vorbis);
  /* stb_vorbis_get_samples_short_interleaved returns samples (not frames). */
  int n = stb_vorbis_get_samples_short_interleaved(
      g_vorbis, info.channels, out, max_frames * info.channels);
  return n;
}

static void ogg_close(void) {
  if (g_vorbis) {
    stb_vorbis_close(g_vorbis);
    g_vorbis = NULL;
  }
}

static const DecoderBackend BACKENDS[] = {
    [MUSIC_FORMAT_MP3]  = { mp3_open,  mp3_read_pcm16,  mp3_close },
    [MUSIC_FORMAT_WAV]  = { wav_open,  wav_read_pcm16,  wav_close },
    [MUSIC_FORMAT_FLAC] = { flac_open, flac_read_pcm16, flac_close },
    [MUSIC_FORMAT_OGG]  = { ogg_open,  ogg_read_pcm16,  ogg_close },
};

/* ---- Spectrum analyzer (128-point FFT) — runs on core 1 (UI) ---- */

static void init_fft_window(void) {
  static uint8_t inited = 0;
  if (inited) return;
  for (int i = 0; i < FFT_SIZE; i++) {
    g_fft_win[i] =
        0.5f * (1.0f - cosf(2.0f * (float)M_PI * i / (FFT_SIZE - 1)));
  }
  inited = 1;
}

static void draw_spectrum_bars(const int16_t *pcm, uint32_t frames,
                                uint8_t channels) {
  if (frames < (uint32_t)FFT_SIZE) return;
  init_fft_window();

  float real[FFT_SIZE], imag[FFT_SIZE];

  /* Mono mix + Hanning window */
  for (int i = 0; i < FFT_SIZE; i++) {
    float s;
    if (channels >= 2) {
      s = (pcm[i * 2] + pcm[i * 2 + 1]) * 0.5f;
    } else {
      s = (float)pcm[i];
    }
    real[i] = s * g_fft_win[i];
    imag[i] = 0.0f;
  }

  /* Bit-reversal permutation */
  for (int i = 0; i < FFT_SIZE; i++) {
    int rev = 0;
    for (int b = 0; b < 7; b++) {
      rev = (rev << 1) | ((i >> b) & 1);
    }
    if (rev > i) {
      float t = real[i];
      real[i] = real[rev];
      real[rev] = t;
      t = imag[i];
      imag[i] = imag[rev];
      imag[rev] = t;
    }
  }

  /* Cooley-Tukey radix-2 DIT FFT */
  for (int len = 2; len <= FFT_SIZE; len <<= 1) {
    float wlen_r = cosf(2.0f * (float)M_PI / len);
    float wlen_i = -sinf(2.0f * (float)M_PI / len);
    for (int i = 0; i < FFT_SIZE; i += len) {
      float wr = 1.0f, wi = 0.0f;
      for (int j = 0; j < len / 2; j++) {
        int i1 = i + j;
        int i2 = i + j + len / 2;
        float tr = wr * real[i2] - wi * imag[i2];
        float ti = wr * imag[i2] + wi * real[i2];
        real[i2] = real[i1] - tr;
        imag[i2] = imag[i1] - ti;
        real[i1] += tr;
        imag[i1] += ti;
        float nw_r = wr * wlen_r - wi * wlen_i;
        wi = wr * wlen_i + wi * wlen_r;
        wr = nw_r;
      }
    }
  }

  /* Average bins into bars (skip DC at bin 0) */
  float bars[SPECTRUM_BARS];
  int bins_per_bar = (FFT_SIZE / 2) / SPECTRUM_BARS;
  for (int b = 0; b < SPECTRUM_BARS; b++) {
    float sum = 0.0f;
    for (int k = 0; k < bins_per_bar; k++) {
      int bin = b * bins_per_bar + k + 1;
      if (bin < FFT_SIZE / 2) {
        sum += sqrtf(real[bin] * real[bin] + imag[bin] * imag[bin]);
      }
    }
    bars[b] = sum / bins_per_bar;
  }

  /* Normalize */
  float peak = 0.001f;
  for (int b = 0; b < SPECTRUM_BARS; b++) {
    if (bars[b] > peak) peak = bars[b];
  }

  /* Draw bars */
  const int AREA_Y = 44;
  const int AREA_H = 170;
  const int BAR_W = 14;
  const int GAP = 2;

  mia_host_fill_rect(0, AREA_Y, 320, AREA_H, MIA_HOST_BLACK);

  for (int b = 0; b < SPECTRUM_BARS; b++) {
    float norm = bars[b] / peak;
    /* sqrt compression: makes quiet bars visible while peak stays full height */
    norm = sqrtf(norm);
    int h = (int)(norm * AREA_H);
    if (h < 1) h = 1;
    if (h > AREA_H) h = AREA_H;
    int x = b * (BAR_W + GAP);
    int y = AREA_Y + AREA_H - h;
    uint8_t color;
    if (norm < 0.4f)
      color = MIA_HOST_GREEN;
    else if (norm < 0.7f)
      color = MIA_HOST_YELLOW;
    else
      color = MIA_HOST_RED;
    mia_host_fill_rect(x, y, BAR_W, h, color);
  }
}

/* ---- Decode task (core 0, priority 5): decoder -> ring buffer producer ---- */

static void decode_task_func(void *arg) {
  (void)arg;
  const DecoderBackend *backend = &BACKENDS[s_audio_format];
  uint32_t chunks = 0;

  while (!s_audio_stop) {
    int frames = backend->read_pcm16(g_decode_pcm, PCM_FRAMES_PER_CHUNK);
    if (frames < 0) {
      ESP_LOGE(TAG, "decode failed chunk=%lu", (unsigned long)chunks);
      break;
    }
    if (frames == 0) {
      ESP_LOGI(TAG, "decode ended chunks=%lu", (unsigned long)chunks);
      break;
    }

    size_t bytes = (size_t)frames * s_playback_channels * sizeof(int16_t);
    /* Block (up to 2 s) if ring is full — this throttles decode to roughly
     * real-time once the ring is saturated, which is exactly what we want. */
    size_t sent = xStreamBufferSend(s_audio_stream, g_decode_pcm, bytes,
                                    pdMS_TO_TICKS(2000));
    if (sent < bytes) {
      ESP_LOGE(TAG, "ring send timeout chunk=%lu", (unsigned long)chunks);
      break;
    }
    ++chunks;
  }

  s_decode_eof = true;
  s_decode_task_handle = NULL;
  vTaskDelete(NULL);
}

/* ---- Audio task (core 0, priority 6): ring buffer consumer -> I2S ----
 * Opens the decoder, creates the PSRAM ring buffer, launches the decode task,
 * waits for a pre-fill, then drains the ring into I2S. SD-card stalls during
 * decode are absorbed by the ring buffer (up to ~2 s of buffered audio). */

static void audio_task_func(void *arg) {
  (void)arg;

  const char *vfs_path = s_audio_path;
  uint32_t feed_chunks = 0;

  const DecoderBackend *backend = &BACKENDS[s_audio_format];

  ESP_LOGI(TAG, "open fmt=%d path=%s", s_audio_format, vfs_path);
  if (backend->open(vfs_path, &s_playback_sample_rate,
                    &s_playback_channels) != 0) {
    s_audio_task_handle = NULL;
    vTaskDelete(NULL);
    return;
  }

  if (s_playback_channels == 0 || s_playback_channels > 2 ||
      s_playback_sample_rate <= 0) {
    ESP_LOGE(TAG, "invalid audio format hz=%d channels=%d",
             s_playback_sample_rate, s_playback_channels);
    backend->close();
    s_audio_task_handle = NULL;
    vTaskDelete(NULL);
    return;
  }

  ESP_LOGI(TAG, "open ok hz=%d channels=%d", s_playback_sample_rate,
           s_playback_channels);
  s_audio_open_ok = true;
  portMEMORY_BARRIER();
  s_audio_running = true;

  /* Allocate ring buffer: ~2 seconds of audio in PSRAM. */
  s_stream_size = (size_t)s_playback_sample_rate * s_playback_channels *
                  sizeof(int16_t) * 2;
  if (s_stream_size < PCM_SAMPLES_PER_CHUNK * sizeof(int16_t) * 4) {
    s_stream_size = PCM_SAMPLES_PER_CHUNK * sizeof(int16_t) * 4;
  }
  s_stream_storage = (uint8_t *)heap_caps_malloc(s_stream_size,
                                                 MALLOC_CAP_SPIRAM);
  if (s_stream_storage == NULL) {
    ESP_LOGE(TAG, "PSRAM ring alloc failed size=%u", (unsigned)s_stream_size);
    backend->close();
    s_audio_running = false;
    s_audio_task_handle = NULL;
    vTaskDelete(NULL);
    return;
  }
  s_audio_stream = xStreamBufferCreateStatic(s_stream_size, 1,
                                             s_stream_storage, &s_stream_obj);
  s_decode_eof = false;
  s_decode_task_handle = NULL;

  /* Start decode task (lower priority than us so we can preempt it for I2S). */
  const BaseType_t dec_result = xTaskCreatePinnedToCore(
      decode_task_func, "music_dec", 24576, NULL, 5,
      (TaskHandle_t *)&s_decode_task_handle, 0);
  if (dec_result != pdPASS) {
    ESP_LOGE(TAG, "decode task create failed result=%ld", (long)dec_result);
    vStreamBufferDelete(s_audio_stream);
    s_audio_stream = NULL;
    heap_caps_free(s_stream_storage);
    s_stream_storage = NULL;
    backend->close();
    s_audio_running = false;
    s_audio_task_handle = NULL;
    vTaskDelete(NULL);
    return;
  }

  /* Pre-fill: wait until ring has ~0.5 s of audio before opening I2S.
   * This gives a head start so the first SD stall doesn't cause an
   * immediate underrun. */
  size_t prefill_target = s_stream_size / 4;
  int prefill_wait = 0;
  while (!s_audio_stop && !s_decode_eof &&
         xStreamBufferBytesAvailable(s_audio_stream) < prefill_target &&
         prefill_wait < 5000) {
    mia_host_delay_ms(10);
    prefill_wait += 10;
  }
  ESP_LOGI(TAG, "prefill ring=%u wait=%d eof=%d",
           (unsigned)xStreamBufferBytesAvailable(s_audio_stream),
           prefill_wait, s_decode_eof);

  if (!mia_host_audio_open((uint32_t)s_playback_sample_rate,
                           s_playback_channels, 16)) {
    ESP_LOGE(TAG, "audio open failed hz=%d channels=%d",
             s_playback_sample_rate, s_playback_channels);
    s_audio_stop = true;
  } else {
    ESP_LOGI(TAG, "audio output opened");
  }

  /* Feed loop: drain ring buffer into I2S. */
  size_t chunk_bytes = PCM_FRAMES_PER_CHUNK * s_playback_channels *
                       sizeof(int16_t);
  while (!s_audio_stop) {
    if (s_decode_eof && xStreamBufferBytesAvailable(s_audio_stream) == 0) {
      break;  /* decoder finished and ring drained */
    }
    size_t got = xStreamBufferReceive(s_audio_stream, g_feed_pcm,
                                      chunk_bytes, pdMS_TO_TICKS(500));
    if (got == 0) {
      continue;
    }
    uint32_t frames = (uint32_t)(got / (s_playback_channels * sizeof(int16_t)));
    if (mia_host_audio_write_pcm16(g_feed_pcm, frames,
                                   s_playback_channels) < 0) {
      ESP_LOGE(TAG, "audio write failed chunk=%lu frames=%u",
               (unsigned long)feed_chunks, (unsigned)frames);
      break;
    }
    ++feed_chunks;

    /* Update FFT shared buffer for UI spectrum. */
    uint32_t copy_count = frames;
    if (copy_count > FFT_SHARED_SAMPLES) {
      copy_count = FFT_SHARED_SAMPLES;
    }
    portENTER_CRITICAL(&s_fft_mux);
    if (s_playback_channels >= 2) {
      for (uint32_t i = 0; i < copy_count; i++) {
        int32_t l = g_feed_pcm[i * 2];
        int32_t r = g_feed_pcm[i * 2 + 1];
        s_fft_shared[i] = (int16_t)((l + r) >> 1);
      }
    } else {
      memcpy(s_fft_shared, g_feed_pcm, copy_count * sizeof(int16_t));
    }
    s_fft_channels = 1;
    portEXIT_CRITICAL(&s_fft_mux);
  }

  /* Signal decode task to stop and wait for it. */
  s_audio_stop = true;
  int stop_wait = 0;
  while (s_decode_task_handle != NULL && stop_wait < 300) {
    mia_host_delay_ms(10);
    stop_wait += 10;
  }

  mia_host_audio_stop();
  mia_host_audio_close();
  backend->close();

  if (s_audio_stream) {
    vStreamBufferDelete(s_audio_stream);
    s_audio_stream = NULL;
  }
  if (s_stream_storage) {
    heap_caps_free(s_stream_storage);
    s_stream_storage = NULL;
  }

  ESP_LOGI(TAG, "audio task stopped feed_chunks=%lu stop=%d exit=%d",
           (unsigned long)feed_chunks, s_audio_stop, s_audio_exit);
  s_audio_running = false;
  s_audio_task_handle = NULL;
  vTaskDelete(NULL);
}

/* ---- Dual-core playback entry point (UI on core 1, starts audio on core 0) ---- */

extern void switch_to_launcher(void);

static MusicPlayResult play_audio_file(const char *path, char *status,
                                       size_t status_size) {
  const MusicText *text = music_text();
  const char *vfs_path = to_vfs_path(path);
  if (vfs_path == NULL) {
    set_status(status, status_size, text->path_too_long);
    return MUSIC_PLAY_OPEN_FAILED;
  }

  /* Copy path for audio task (it runs on core 0 with its own stack) */
  snprintf(s_audio_path, sizeof(s_audio_path), "%s", vfs_path);
  s_audio_format = detect_format(path);
  ESP_LOGI(TAG, "play request path=%s vfs=%s fmt=%d", path, s_audio_path,
           s_audio_format);

  /* Get short filename for display */
  const char *fname = strrchr(path, '/');
  fname = fname ? fname + 1 : path;
  char name_buf[64];
  snprintf(name_buf, sizeof(name_buf), "%s", fname);

  /* Draw initial Now Playing screen on core 1 (UI) */
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, 320, 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, mia_host_text_y_centered(0, 20), text->now_playing,
                     MIA_HOST_BLACK, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 24, name_buf, MIA_HOST_CYAN, MIA_HOST_BLACK);
  mia_host_draw_text(8, 220, text->playback_controls, MIA_HOST_GREEN,
                     MIA_HOST_BLACK);

  /* Reset control flags before starting audio task */
  s_audio_open_ok = false;
  s_audio_running = false;
  s_audio_stop = false;
  s_audio_exit = false;

  /* Start audio task on core 0 (priority 6 — slightly higher than UI at 5) */
  const BaseType_t task_result =
      xTaskCreatePinnedToCore(audio_task_func, "music_audio", 32768, NULL, 6,
                              (TaskHandle_t *)&s_audio_task_handle, 0);
  if (task_result != pdPASS) {
    ESP_LOGE(TAG, "audio task create failed result=%ld free_heap=%u",
             (long)task_result, (unsigned)esp_get_free_heap_size());
    set_status(status, status_size, text->playback_failed);
    return MUSIC_PLAY_AUDIO_FAILED;
  }

  /* Wait for audio file to open (or fail) with timeout.
   * s_audio_open_ok is a latch set once on success — it stays true
   * even after playback ends, so short files don't get mis-detected.
   * s_audio_task_handle is set to NULL when the audio task deletes
   * itself on open failure — detect that to exit early.
   * The timeout is generous (10 s) because mp3dec_ex_open() over
   * SPI SD card can be slow for some files. */
  mia_host_draw_text(84, 96, text->opening, MIA_HOST_YELLOW, MIA_HOST_BLACK);
  mia_host_present();
  int wait_ms = 0;
  while (!s_audio_open_ok && s_audio_task_handle != NULL && wait_ms < 10000) {
    mia_host_delay_ms(20);
    wait_ms += 20;
  }

  if (!s_audio_open_ok) {
    ESP_LOGE(TAG, "audio task did not open file wait_ms=%d handle=%p", wait_ms,
             (void *)s_audio_task_handle);
    mia_host_draw_text(84, 96, text->open_failed, MIA_HOST_RED, MIA_HOST_BLACK);
    mia_host_present();
    set_status(status, status_size, text->audio_open_failed);
    return MUSIC_PLAY_OPEN_FAILED;
  }

  /* ---- UI loop on core 1: spectrum + buttons, audio runs in parallel on core 0 ---- */
  MusicPlayResult result = MUSIC_PLAY_OK;

  while (s_audio_running && !s_audio_exit) {
    /* Poll buttons on core 1 */
    mia_host_buttons_poll();

    if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
      s_audio_stop = true;
    }

    if (!s_audio_stop &&  /* B triggers stop — SEL+ST triggers exit */
        mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
        mia_host_button_down(MIA_HOST_BUTTON_START)) {
      s_audio_stop = true;
      s_audio_exit = true;
    }

    /* Grab latest PCM data for spectrum (non-blocking critical section) */
    int16_t fft_buf[FFT_SIZE];
    uint8_t channels = 0;
    portENTER_CRITICAL(&s_fft_mux);
    memcpy(fft_buf, s_fft_shared, FFT_SIZE * sizeof(int16_t));
    channels = s_fft_channels;
    portEXIT_CRITICAL(&s_fft_mux);

    draw_spectrum_bars(fft_buf, (uint32_t)FFT_SIZE, channels);
    mia_host_present();

    mia_host_delay_ms(30);  /* ~33 Hz UI update — leave CPU for audio decode on core 0 */
  }

  /* ---- Playback ended or stop/exit requested ---- */

  if (s_audio_exit) {
    /* Audio task already signaled stop; wait briefly for it to finish */
    int timeout = 300;  /* 3 seconds max */
    while (s_audio_running && timeout > 0) {
      mia_host_delay_ms(10);
      --timeout;
    }
    switch_to_launcher();
    esp_restart();
    /* never reached */
  }

  /* B-stop or song-end: wait for audio task to finish */
  int timeout = 300;
  while (s_audio_running && timeout > 0) {
    mia_host_delay_ms(10);
    --timeout;
  }

  if (s_audio_stop) {
    result = MUSIC_PLAY_STOPPED;
  }

  set_status(status, status_size,
             result == MUSIC_PLAY_STOPPED
                  ? text->stopped
                  : (result == MUSIC_PLAY_OK ? text->done : text->playback_failed));
  ESP_LOGI(TAG, "play returned result=%d status=%s", result, status);
  return result;
}

MusicPlayResult music_play_file(const char *path, char *status,
                                size_t status_size) {
  enum MusicFormat fmt = detect_format(path);
  if (fmt == MUSIC_FORMAT_UNKNOWN) {
    set_status(status, status_size, music_text()->unsupported_file);
    return MUSIC_PLAY_UNSUPPORTED;
  }
  return play_audio_file(path, status, status_size);
}

MusicPlayResult music_probe_file(const char *path, char *status,
                                 size_t status_size) {
  return music_play_file(path, status, status_size);
}
