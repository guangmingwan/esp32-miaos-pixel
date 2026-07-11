#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    MIA_GBA_SAVE_SRAM = 0,
    MIA_GBA_SAVE_FLASH = 1,
    MIA_GBA_SAVE_EEPROM = 2,
} MiaGbaSaveType;

typedef struct {
    MiaGbaSaveType type;
    uint32_t flash_size;
    uint32_t eeprom_size;
} MiaGbaSaveState;

size_t mia_gba_save_size(MiaGbaSaveState state);
bool mia_gba_bios_metadata_valid(size_t size, const uint8_t md5[16]);
bool mia_gba_rom_size_valid(size_t size);
bool mia_gba_page_allocation_valid(size_t page_blocks);
