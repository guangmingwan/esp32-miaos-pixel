#pragma once

typedef struct {
  const char *title;
  const char *now_playing;
  const char *browse_status;
  const char *browse_controls;
  const char *directory_empty;
  const char *no_audio_files;
  const char *playback_controls;
  const char *opening;
  const char *open_failed;
  const char *path_too_long;
  const char *audio_open_failed;
  const char *stopped;
  const char *done;
  const char *playback_failed;
  const char *unsupported_file;
} MusicText;

const MusicText *music_text(void);
