#include "megadrive_policy.h"

#include <string.h>

static uint32_t be32(const uint8_t *value) {
    return ((uint32_t)value[0] << 24) | ((uint32_t)value[1] << 16) |
           ((uint32_t)value[2] << 8) | value[3];
}

MiaMegadriveCoreConfig mia_megadrive_core_config(void) {
    return (MiaMegadriveCoreConfig){true, true, true, true, true, 3u, 26633u};
}

MiaMegadriveTiming mia_megadrive_timing(bool pal) {
    return pal ? (MiaMegadriveTiming){313u, 50u, 528u}
               : (MiaMegadriveTiming){262u, 60u, 444u};
}

MiaMegadriveGeometry mia_megadrive_geometry(bool h40, bool pal_240) {
    return (MiaMegadriveGeometry){h40 ? 320u : 256u, pal_240 ? 240u : 224u};
}

uint8_t mia_megadrive_pad_mask(uint32_t host) {
    static const uint32_t bits[] = {MIA_MD_HOST_UP, MIA_MD_HOST_DOWN,
        MIA_MD_HOST_LEFT, MIA_MD_HOST_RIGHT, MIA_MD_HOST_B, MIA_MD_HOST_X,
        MIA_MD_HOST_A, MIA_MD_HOST_START};
    uint8_t result = 0;
    for (size_t index = 0; index < sizeof(bits) / sizeof(bits[0]); ++index)
        if ((host & bits[index]) != 0u) result |= (uint8_t)(1u << index);
    return result;
}

MiaMegadriveSram mia_megadrive_sram_parse(const uint8_t *rom, size_t size) {
    if (rom == NULL || size < 0x1bcu || memcmp(&rom[0x1b0], "RA", 2) != 0)
        return (MiaMegadriveSram){0};
    const uint32_t start = be32(&rom[0x1b4]);
    const uint32_t end = be32(&rom[0x1b8]);
    if (end < start || (uint64_t)end - start + 1u > 0x10000u)
        return (MiaMegadriveSram){0};
    return (MiaMegadriveSram){true, start, (size_t)(end - start + 1u)};
}

bool mia_megadrive_save_should_flush(bool dirty, bool exiting) {
    return dirty || exiting;
}
