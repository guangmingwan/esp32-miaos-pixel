#pragma once

#include <stdint.h>

typedef struct {
    uint32_t frames;
    uint32_t late_frames;
    uint32_t skipped_frames;
} MiaSnesPacing;

int mia_snes_extension_supported(const char *path);
void mia_snes_pacing_record(MiaSnesPacing *pacing, uint32_t elapsed_us, uint32_t budget_us, int rendered);
uint8_t mia_snes_adjust_frameskip(uint8_t frameskip, uint32_t emulated_frames,
                                  uint32_t target_frames, uint32_t late_frames,
                                  uint32_t display_busy_frames);
