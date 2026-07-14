#include "mia_nes_contract.h"

#include <string.h>
#include <strings.h>

static int has_extension(const char *path, const char *extension) {
    const char *dot = path == NULL ? NULL : strrchr(path, '.');
    return dot != NULL && strcasecmp(dot + 1, extension) == 0;
}

static int mapper_supported(uint8_t mapper) {
    static const uint8_t supported[] = {0, 1, 2, 3, 4, 5, 7, 9, 15, 18, 19, 20, 23, 24, 31, 32, 33, 41, 42, 46, 50, 64, 66, 71, 73, 74, 75, 76, 78, 79, 85, 87, 119, 160, 162, 176, 185, 192, 193, 194, 195, 206, 228, 229, 231};
    for (size_t index = 0; index < sizeof(supported); ++index) {
        if (supported[index] == mapper) return 1;
    }
    return 0;
}

MiaNesResult mia_nes_validate_extension(const char *path) {
    return has_extension(path, "nes") || has_extension(path, "fc") ||
           has_extension(path, "fds") || has_extension(path, "nsf") ||
           has_extension(path, "zip")
               ? MIA_NES_OK : MIA_NES_UNSUPPORTED_FILE;
}

MiaNesResult mia_nes_validate_image(const uint8_t *header, size_t size, int fds_bios_valid) {
    if (header == NULL || size < 16) return MIA_NES_INVALID_HEADER;
    if (memcmp(header, "NESM\x1a", 5) == 0) return MIA_NES_OK;
    if (memcmp(header, "FDS\x1a", 4) == 0 || memcmp(header, "\x01*NINTENDO-HVC*", 15) == 0) {
        return fds_bios_valid ? MIA_NES_OK : MIA_NES_FDS_BIOS_REQUIRED;
    }
    if (memcmp(header, "NES\x1a", 4) != 0) return MIA_NES_INVALID_HEADER;
    const uint8_t mapper = (uint8_t)((header[7] & 0xf0u) | (header[6] >> 4u));
    return mapper_supported(mapper) ? MIA_NES_OK : MIA_NES_UNSUPPORTED_MAPPER;
}
