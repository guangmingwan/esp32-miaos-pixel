#include "megadrive_policy.h"

#include <assert.h>
#include <string.h>

static void test_core_configuration_is_complete(void) {
    const MiaMegadriveCoreConfig config = mia_megadrive_core_config();
    assert(config.m68k && config.z80 && config.ym2612 && config.sn76489);
    assert(config.audio_accurate && config.controller_buttons == 3u);
    assert(config.sample_rate_hz == 26633u);
}

static void test_region_timing_and_geometry_are_dynamic(void) {
    const MiaMegadriveTiming ntsc = mia_megadrive_timing(false);
    const MiaMegadriveTiming pal = mia_megadrive_timing(true);
    assert(ntsc.lines == 262u && ntsc.refresh_hz == 60u && ntsc.audio_frames == 444u);
    assert(pal.lines == 313u && pal.refresh_hz == 50u && pal.audio_frames == 528u);
    assert(mia_megadrive_geometry(false, false).width == 256u);
    assert(mia_megadrive_geometry(true, true).width == 320u);
    assert(mia_megadrive_geometry(true, true).height == 240u);
}

static void test_three_button_mapping_matches_core_order(void) {
    const uint32_t host = MIA_MD_HOST_UP | MIA_MD_HOST_A | MIA_MD_HOST_B |
                          MIA_MD_HOST_X | MIA_MD_HOST_START;
    const uint8_t core = mia_megadrive_pad_mask(host);
    assert((core & (1u << MIA_MD_PAD_UP)) != 0u);
    assert((core & (1u << MIA_MD_PAD_A)) != 0u);
    assert((core & (1u << MIA_MD_PAD_B)) != 0u);
    assert((core & (1u << MIA_MD_PAD_C)) != 0u);
    assert((core & (1u << MIA_MD_PAD_START)) != 0u);
}

static void test_sram_header_and_dirty_lifecycle_are_bounded(void) {
    uint8_t rom[0x200] = {0};
    memcpy(&rom[0x1b0], "RA", 2);
    rom[0x1b5] = 0x20;
    rom[0x1b7] = 0x01;
    rom[0x1b9] = 0x20;
    rom[0x1ba] = 0x80;
    MiaMegadriveSram sram = mia_megadrive_sram_parse(rom, sizeof(rom));
    assert(sram.present && sram.start == 0x200001u && sram.size == 0x8000u);
    assert(!mia_megadrive_save_should_flush(false, false));
    assert(mia_megadrive_save_should_flush(true, false));
    assert(mia_megadrive_save_should_flush(false, true));
}

int main(void) {
    test_core_configuration_is_complete();
    test_region_timing_and_geometry_are_dynamic();
    test_three_button_mapping_matches_core_order();
    test_sram_header_and_dirty_lifecycle_are_bounded();
    return 0;
}
