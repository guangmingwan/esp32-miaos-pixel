#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { MIA_MD_PAD_UP, MIA_MD_PAD_DOWN, MIA_MD_PAD_LEFT, MIA_MD_PAD_RIGHT,
       MIA_MD_PAD_B, MIA_MD_PAD_C, MIA_MD_PAD_A, MIA_MD_PAD_START };
enum { MIA_MD_HOST_UP = 1u << 0, MIA_MD_HOST_DOWN = 1u << 1,
       MIA_MD_HOST_LEFT = 1u << 2, MIA_MD_HOST_RIGHT = 1u << 3,
       MIA_MD_HOST_A = 1u << 4, MIA_MD_HOST_B = 1u << 5,
       MIA_MD_HOST_X = 1u << 6, MIA_MD_HOST_START = 1u << 7 };

typedef struct { bool m68k, z80, ym2612, sn76489, audio_accurate; uint8_t controller_buttons; uint32_t sample_rate_hz; } MiaMegadriveCoreConfig;
typedef struct { uint16_t lines, refresh_hz, audio_frames; } MiaMegadriveTiming;
typedef struct { uint16_t width, height; } MiaMegadriveGeometry;
typedef struct { bool present; uint32_t start; size_t size; } MiaMegadriveSram;

MiaMegadriveCoreConfig mia_megadrive_core_config(void);
MiaMegadriveTiming mia_megadrive_timing(bool pal);
MiaMegadriveGeometry mia_megadrive_geometry(bool h40, bool pal_240);
uint8_t mia_megadrive_pad_mask(uint32_t host_mask);
MiaMegadriveSram mia_megadrive_sram_parse(const uint8_t *rom, size_t size);
bool mia_megadrive_save_should_flush(bool dirty, bool exiting);
