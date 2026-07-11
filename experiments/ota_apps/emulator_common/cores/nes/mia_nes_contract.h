#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum {
    MIA_NES_OK = 0,
    MIA_NES_INVALID_HEADER,
    MIA_NES_UNSUPPORTED_MAPPER,
    MIA_NES_FDS_BIOS_REQUIRED,
    MIA_NES_UNSUPPORTED_FILE,
} MiaNesResult;

MiaNesResult mia_nes_validate_extension(const char *path);
MiaNesResult mia_nes_validate_image(const uint8_t *header, size_t size, int fds_bios_valid);
