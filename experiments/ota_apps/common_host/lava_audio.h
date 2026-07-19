#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void lava_audio_start(const char *game_dir);
void lava_audio_stop(void);
void lava_audio_set_music(int number, int loop, int fade_time);
void lava_audio_stop_music(void);
void lava_audio_play_sound(int number);
void lava_audio_enable_music(int enabled);
void lava_audio_enable_sound(int enabled);

#ifdef __cplusplus
}
#endif
