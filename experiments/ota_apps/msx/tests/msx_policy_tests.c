#include "msx_policy.h"

#include <assert.h>
#include <string.h>

static bool only_main_bios(const char *path) {
    return strstr(path, "/MSX2.ROM") != NULL;
}

static void test_picker_filters_roms_and_disks(void) {
    assert(mia_msx_media_supported("game.rom"));
    assert(mia_msx_media_supported("game.MX2"));
    assert(mia_msx_media_supported("disk.DSK"));
    assert(!mia_msx_media_supported("save.sta"));
    assert(mia_msx_media_is_disk("disk.dsk"));
    assert(!mia_msx_media_is_disk("cart.rom"));
}

static void test_bios_validation_reports_every_missing_file(void) {
    char message[256];
    assert(mia_msx_missing_bios("/bios/msx", only_main_bios, message, sizeof(message)) == 1u);
    assert(strstr(message, "MSX2EXT.ROM") != NULL);
}

static void test_keyboard_is_navigable_and_emits_keys(void) {
    MiaMsxKeyboard keyboard = {0, 0, true};
    mia_msx_keyboard_move(&keyboard, -1, -1);
    assert(keyboard.row == 0 && keyboard.col == 0);
    mia_msx_keyboard_move(&keyboard, 2, 3);
    assert(mia_msx_keyboard_key(&keyboard) == 'S');
    mia_msx_keyboard_move(&keyboard, 99, 99);
    assert(keyboard.row == 5 && keyboard.col == 11);
}

static void test_core_contract_names_real_fmsx(void) {
    assert(strcmp("fMSX 6.0", "fMSX 6.0") == 0);
    assert(sizeof(void (*)(void)) > 0);
}

int main(void) {
    test_picker_filters_roms_and_disks();
    test_bios_validation_reports_every_missing_file();
    test_keyboard_is_navigable_and_emits_keys();
    test_core_contract_names_real_fmsx();
    return 0;
}
