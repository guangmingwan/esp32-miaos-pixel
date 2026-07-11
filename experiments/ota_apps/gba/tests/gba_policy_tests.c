#include "gba_policy.h"

#include <assert.h>
#include <string.h>

static void test_save_types_have_native_sizes(void) {
    assert(mia_gba_save_size((MiaGbaSaveState){MIA_GBA_SAVE_SRAM, 0, 0}) == 32u * 1024u);
    assert(mia_gba_save_size((MiaGbaSaveState){MIA_GBA_SAVE_FLASH, 2, 0}) == 128u * 1024u);
    assert(mia_gba_save_size((MiaGbaSaveState){MIA_GBA_SAVE_EEPROM, 0, 16}) == 8u * 1024u);
}

static void test_malformed_save_types_are_rejected(void) {
    assert(mia_gba_save_size((MiaGbaSaveState){(MiaGbaSaveType)99, 0, 0}) == 0u);
    assert(mia_gba_save_size((MiaGbaSaveState){MIA_GBA_SAVE_FLASH, 99, 0}) == 0u);
    assert(mia_gba_save_size((MiaGbaSaveState){MIA_GBA_SAVE_EEPROM, 0, 99}) == 0u);
}

static void test_bios_requires_canonical_metadata(void) {
    static const uint8_t canonical_md5[16] = {0xa8, 0x60, 0xe8, 0xc0, 0xb6, 0xd5, 0x73, 0xd1, 0x91, 0xe4, 0xec, 0x7d, 0xb1, 0xb1, 0xe4, 0xf6};
    uint8_t corrupt_md5[16] = {0};
    assert(mia_gba_bios_metadata_valid(16384u, canonical_md5));
    assert(!mia_gba_bios_metadata_valid(16383u, canonical_md5));
    assert(!mia_gba_bios_metadata_valid(16384u, corrupt_md5));
}

static void test_rom_and_psram_policies_are_bounded(void) {
    assert(mia_gba_rom_size_valid(32u * 1024u * 1024u));
    assert(!mia_gba_rom_size_valid(32u * 1024u * 1024u + 1u));
    assert(mia_gba_page_allocation_valid(1u));
    assert(!mia_gba_page_allocation_valid(0u));
}

int main(void) {
    test_save_types_have_native_sizes();
    test_malformed_save_types_are_rejected();
    test_bios_requires_canonical_metadata();
    test_rom_and_psram_policies_are_bounded();
    return 0;
}
