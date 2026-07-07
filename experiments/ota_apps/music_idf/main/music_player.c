#include "music_player.h"

#include "mia_host_abi.h"

#include <stdio.h>
#include <string.h>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include "third_party/minimp3_ex.h"

#define PCM_FRAMES_PER_CHUNK 512
#define PCM_SAMPLES_PER_CHUNK (PCM_FRAMES_PER_CHUNK * 2)

enum MusicFormat {
  MUSIC_FORMAT_UNKNOWN = 0,
  MUSIC_FORMAT_MP3,
};

static mp3dec_ex_t g_mp3;
static int16_t g_pcm[PCM_SAMPLES_PER_CHUNK];
static char g_vfs_path[256];

static int should_stop_playback(void) {
  mia_host_buttons_poll();
  return mia_host_button_pressed(MIA_HOST_BUTTON_B) ||
         (mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
          mia_host_button_down(MIA_HOST_BUTTON_START));
}

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
  if (snprintf(g_vfs_path, sizeof(g_vfs_path), "/sd%s", path) >= (int)sizeof(g_vfs_path)) {
    return NULL;
  }
  return g_vfs_path;
}

int music_is_supported_file(const char *name) {
  return detect_format(name) != MUSIC_FORMAT_UNKNOWN;
}

static MusicPlayResult play_mp3(const char *path, char *status, size_t status_size) {
  const char *vfs_path = to_vfs_path(path);
  MusicPlayResult result = MUSIC_PLAY_OK;
  uint8_t audio_open = 0;

  if (vfs_path == NULL) {
    set_status(status, status_size, "Path too long");
    return MUSIC_PLAY_OPEN_FAILED;
  }
  memset(&g_mp3, 0, sizeof(g_mp3));
  if (mp3dec_ex_open(&g_mp3, vfs_path, MP3D_DO_NOT_SCAN) != 0) {
    set_status(status, status_size, "MP3 open failed");
    return MUSIC_PLAY_OPEN_FAILED;
  }

  while (1) {
    size_t samples;
    uint8_t channels;

    if (should_stop_playback()) {
      result = MUSIC_PLAY_STOPPED;
      break;
    }
    samples = mp3dec_ex_read(&g_mp3, g_pcm, PCM_SAMPLES_PER_CHUNK);
    if (samples == 0) {
      break;
    }
    channels = (uint8_t)g_mp3.info.channels;
    if (channels == 0 || channels > 2 || g_mp3.info.hz <= 0) {
      result = MUSIC_PLAY_UNSUPPORTED;
      break;
    }
    if (!audio_open) {
      if (!mia_host_audio_open((uint32_t)g_mp3.info.hz, channels, 16)) {
        result = MUSIC_PLAY_AUDIO_FAILED;
        break;
      }
      audio_open = 1;
    }
    if (mia_host_audio_write_pcm16(g_pcm, (uint32_t)(samples / channels), channels) < 0) {
      result = MUSIC_PLAY_AUDIO_FAILED;
      break;
    }
  }

  mia_host_audio_stop();
  mia_host_audio_close();
  mp3dec_ex_close(&g_mp3);
  set_status(status, status_size,
             result == MUSIC_PLAY_STOPPED ? "Stopped" : (result == MUSIC_PLAY_OK ? "Done" : "MP3 playback failed"));
  return result;
}

MusicPlayResult music_play_file(const char *path, char *status, size_t status_size) {
  switch (detect_format(path)) {
    case MUSIC_FORMAT_MP3:
      return play_mp3(path, status, status_size);
    case MUSIC_FORMAT_UNKNOWN:
    default:
      set_status(status, status_size, "Unsupported file");
      return MUSIC_PLAY_UNSUPPORTED;
  }
}

MusicPlayResult music_probe_file(const char *path, char *status, size_t status_size) {
  return music_play_file(path, status, status_size);
}
