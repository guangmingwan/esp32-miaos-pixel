#include "gba_policy.h"

#include <string.h>

#define MIA_KIB 1024u
#define MIA_MIB (1024u * MIA_KIB)

size_t mia_gba_save_size(MiaGbaSaveState state) {
    switch (state.type) {
        case MIA_GBA_SAVE_SRAM:
            return 32u * MIA_KIB;
        case MIA_GBA_SAVE_FLASH:
            if (state.flash_size == 1u) return 64u * MIA_KIB;
            if (state.flash_size == 2u) return 128u * MIA_KIB;
            return 0u;
        case MIA_GBA_SAVE_EEPROM:
            if (state.eeprom_size == 1u) return 512u;
            if (state.eeprom_size == 16u) return 8u * MIA_KIB;
            return 0u;
    }
    return 0u;
}

bool mia_gba_bios_metadata_valid(size_t size, const uint8_t md5[16]) {
    static const uint8_t canonical[16] = {0xa8, 0x60, 0xe8, 0xc0, 0xb6, 0xd5, 0x73, 0xd1, 0x91, 0xe4, 0xec, 0x7d, 0xb1, 0xb1, 0xe4, 0xf6};
    return size == 16u * MIA_KIB && md5 != NULL && memcmp(md5, canonical, sizeof(canonical)) == 0;
}

bool mia_gba_rom_size_valid(size_t size) {
    return size >= 192u && size <= 32u * MIA_MIB;
}

bool mia_gba_page_allocation_valid(size_t page_blocks) {
    return page_blocks >= 1u && page_blocks <= 4u;
}
