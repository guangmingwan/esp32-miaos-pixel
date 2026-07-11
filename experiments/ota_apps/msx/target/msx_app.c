#include "msx_policy.h"
#include "msx_machine.h"
#include "mia_emulator_runtime.h"
#include "mia_host_abi.h"
#include "msxfix.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static bool file_exists(const char *path) {
    struct stat info;
    return stat(path, &info) == 0 && S_ISREG(info.st_mode);
}

int mia_msx_main(void) {
    static const char *const extensions[] = {"rom", "mx1", "mx2", "dsk"};
    MiaStorageContext storage = {""};
    MiaStorageTarget target = {"msx", "/roms/msx", "/saves/msx", extensions, 4, NULL, 0};
    MiaAppPickerSelection selection = {0};
    char missing[256];
    const size_t count = mia_msx_missing_bios("/bios/msx", file_exists, missing, sizeof(missing));
    if (count != 0) {
        char message[320];
        snprintf(message, sizeof(message), "MSX BIOS missing from /bios/msx: %s", missing);
        mia_host_log(message);
        return 1;
    }
    MiaStorageStatus picked = mia_emulator_picker_run(&storage, &target, &selection);
    if (picked.code != MIA_STORAGE_OK || !mia_msx_media_supported(selection.rom_path)) return 1;
    char *argv[14] = {"fmsx", "-ram", "2", "-vram", "2", "-skip", "50",
                      "-home", "/bios/msx", "-joy", "1", NULL, NULL, NULL};
    int argc = 11;
    if (mia_msx_media_is_disk(selection.rom_path)) argv[argc++] = "-diska";
    argv[argc++] = selection.rom_path;
    mia_msx_set_save_name(selection.save_name);
    return fmsx_main(argc, argv);
}
