#include "music_player.h"

#include "mia_host_abi.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include "third_party/minimp3_ex.h"

#define PCM_FRAMES_PER_CHUNK 512
#define PCM_SAMPLES_PER_CHUNK (PCM_FRAMES_PER_CHUNK * 2)

#define FFT_SIZE 128
#define SPECTRUM_BARS 20

enum MusicFormat {
  MUSIC_FORMAT_UNKNOWN = 0,
  MUSIC_FORMAT_MP3,
};

/* ---- Dual-core shared state (audio task on core 0, UI on core 1) ---- */

/* Audio path copied here before starting the audio task */
static char s_audio_path[256];

/* Control flags */
static volatile bool s_audio_open_ok = false;   /* latch: set once on successful file open */
static volatile bool s_audio_stop = false;
static volatile bool s_audio_running = false;
static volatile bool s_audio_exit = false;
static volatile TaskHandle_t s_audio_task_handle = NULL;

/* PCM double-buffer for FFT data (audio → UI) */
#define FFT_SHARED_SAMPLES 256
static int16_t s_fft_shared[FFT_SHARED_SAMPLES];
static volatile uint8_t s_fft_channels = 0;
static portMUX_TYPE s_fft_mux = portMUX_INITIALIZER_UNLOCKED;

/* ---- Static buffers for the audio task only (core 0), no conflict ---- */
static mp3dec_ex_t g_mp3;
static int16_t g_pcm[PCM_SAMPLES_PER_CHUNK];
static char g_vfs_path[256];

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
  return MUSIC_FORMAT_UNKNOWN;
}

static const char *to_vfs_path(const char *path) {
  if (path == NULL || path[0] == '\0') {
    return NULL;
  }
  if (strncmp(path, "/sd/", 4) == 0 || strcmp(path, "/sd") == 0) {
    return path;
  }
  if (snprintf(g_vfs_path, sizeof(g_vfs_path), "/sd%s", path) >=
      (int)sizeof(g_vfs_path)) {
    return NULL;
  }
  return g_vfs_path;
}

int music_is_supported_file(const char *name) {
  return detect_format(name) != MUSIC_FORMAT_UNKNOWN;
}

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

/* ---- Audio task: runs on core 0 (dedicated decode + I2S) ---- */

static void audio_task_func(void *arg) {
  (void)arg;

  const char *vfs_path = s_audio_path;
  uint8_t audio_open = 0;

  memset(&g_mp3, 0, sizeof(g_mp3));
  if (mp3dec_ex_open(&g_mp3, vfs_path, MP3D_DO_NOT_SCAN) != 0) {
    s_audio_task_handle = NULL;
    vTaskDelete(NULL);
    return;
  }

  s_audio_open_ok = true;   /* latch — never cleared, UI uses this to distinguish open-ok from playback-done */
  portMEMORY_BARRIER();     /* ensure open_ok is globally visible before any subsequent stores */
  s_audio_running = true;

  while (!s_audio_stop) {
    size_t samples = mp3dec_ex_read(&g_mp3, g_pcm, PCM_SAMPLES_PER_CHUNK);
    if (samples == 0) {
      break;
    }

    uint8_t channels = (uint8_t)g_mp3.info.channels;
    if (channels == 0 || channels > 2 || g_mp3.info.hz <= 0) {
      break;
    }

    if (!audio_open) {
      if (!mia_host_audio_open((uint32_t)g_mp3.info.hz, channels, 16)) {
        break;
      }
      audio_open = 1;
    }

    if (mia_host_audio_write_pcm16(g_pcm, (uint32_t)(samples / channels),
                                   channels) < 0) {
      break;
    }

    /* Share PCM samples for UI spectrum (core 1 will read via critical section).
     * Always store mono-mixed samples so the UI's 128-element fft_buf works
     * regardless of the source channel count. */
    uint32_t copy_count = (samples / channels);
    if (copy_count > FFT_SHARED_SAMPLES) {
      copy_count = FFT_SHARED_SAMPLES;
    }
    portENTER_CRITICAL(&s_fft_mux);
    if (channels >= 2) {
      /* Stereo → mono mix on the fly as we copy */
      for (uint32_t i = 0; i < copy_count; i++) {
        int32_t l = g_pcm[i * 2];
        int32_t r = g_pcm[i * 2 + 1];
        s_fft_shared[i] = (int16_t)((l + r) >> 1);
      }
    } else {
      memcpy(s_fft_shared, g_pcm, copy_count * sizeof(int16_t));
    }
    s_fft_channels = 1;  /* always report mono to the UI */
    portEXIT_CRITICAL(&s_fft_mux);
  }

  mia_host_audio_stop();
  mia_host_audio_close();
  mp3dec_ex_close(&g_mp3);
  s_audio_running = false;
  s_audio_task_handle = NULL;
  vTaskDelete(NULL);
}

/* ---- Dual-core MP3 playback entry point (UI on core 1, starts audio on core 0) ---- */

extern void switch_to_launcher(void);

static MusicPlayResult play_mp3(const char *path, char *status,
                                size_t status_size) {
  const char *vfs_path = to_vfs_path(path);
  if (vfs_path == NULL) {
    set_status(status, status_size, "Path too long");
    return MUSIC_PLAY_OPEN_FAILED;
  }

  /* Copy path for audio task (it runs on core 0 with its own stack) */
  snprintf(s_audio_path, sizeof(s_audio_path), "%s", vfs_path);

  /* Get short filename for display */
  const char *fname = strrchr(path, '/');
  fname = fname ? fname + 1 : path;
  char name_buf[64];
  snprintf(name_buf, sizeof(name_buf), "%s", fname);

  /* Draw initial Now Playing screen on core 1 (UI) */
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, 320, 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 6, "Now Playing", MIA_HOST_BLACK, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 24, name_buf, MIA_HOST_CYAN, MIA_HOST_BLACK);
  mia_host_draw_text(8, 220, "B:Stop   SEL+ST:Exit", MIA_HOST_GREEN,
                     MIA_HOST_BLACK);

  /* Reset control flags before starting audio task */
  s_audio_open_ok = false;
  s_audio_running = false;
  s_audio_stop = false;
  s_audio_exit = false;

  /* Start audio task on core 0 (priority 6 — slightly higher than UI at 5) */
  xTaskCreatePinnedToCore(audio_task_func, "music_audio", 32768, NULL, 6,
                          (TaskHandle_t *)&s_audio_task_handle, 0);

  /* Wait for audio file to open (or fail) with timeout.
   * s_audio_open_ok is a latch set once on success — it stays true
   * even after playback ends, so short files don't get mis-detected.
   * s_audio_task_handle is set to NULL when the audio task deletes
   * itself on open failure — detect that to exit early.
   * The timeout is generous (10 s) because mp3dec_ex_open() over
   * SPI SD card can be slow for some files. */
  mia_host_draw_text(84, 96, "Opening...", MIA_HOST_YELLOW, MIA_HOST_BLACK);
  mia_host_present();
  int wait_ms = 0;
  while (!s_audio_open_ok && s_audio_task_handle != NULL && wait_ms < 10000) {
    mia_host_delay_ms(20);
    wait_ms += 20;
  }

  if (!s_audio_open_ok) {
    mia_host_draw_text(84, 96, "Open failed", MIA_HOST_RED, MIA_HOST_BLACK);
    mia_host_present();
    set_status(status, status_size, "MP3 open failed");
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
                 ? "Stopped"
                 : (result == MUSIC_PLAY_OK ? "Done" : "Playback failed"));
  return result;
}

MusicPlayResult music_play_file(const char *path, char *status,
                                size_t status_size) {
  switch (detect_format(path)) {
    case MUSIC_FORMAT_MP3:
      return play_mp3(path, status, status_size);
    case MUSIC_FORMAT_UNKNOWN:
    default:
      set_status(status, status_size, "Unsupported file");
      return MUSIC_PLAY_UNSUPPORTED;
  }
}

MusicPlayResult music_probe_file(const char *path, char *status,
                                 size_t status_size) {
  return music_play_file(path, status, status_size);
}
