#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum MusicPlayResult {
  MUSIC_PLAY_OK = 0,
  MUSIC_PLAY_STOPPED = 1,
  MUSIC_PLAY_UNSUPPORTED = -1,
  MUSIC_PLAY_OPEN_FAILED = -2,
  MUSIC_PLAY_AUDIO_FAILED = -3,
  MUSIC_PLAY_DECODE_FAILED = -4,
} MusicPlayResult;

int music_is_supported_file(const char *name);
MusicPlayResult music_play_file(const char *path, char *status, size_t status_size);
