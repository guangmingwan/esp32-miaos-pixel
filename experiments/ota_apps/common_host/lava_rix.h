#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LavaRixPlayer LavaRixPlayer;

LavaRixPlayer *lava_rix_open(const char *path, int song, uint32_t sample_rate);
void lava_rix_close(LavaRixPlayer *player);
int lava_rix_render(LavaRixPlayer *player, int16_t *stereo_samples, uint32_t frames,
                    int loop);

#ifdef __cplusplus
}
#endif
