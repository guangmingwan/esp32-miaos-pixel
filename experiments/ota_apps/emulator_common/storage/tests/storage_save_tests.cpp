#include "test_support.h"

#include <filesystem>
#include <fstream>
#include <sys/stat.h>

namespace fs = std::filesystem;

static std::string read_text(const fs::path &path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

int main() {
    const MiaStorageTarget *target = nullptr;
    require_status(mia_storage_target_find(&mia_storage_generated_targets, "gb", &target), MIA_STORAGE_OK);
    auto root = make_temp_tree("save");
    fs::create_directories(root / "saves" / "gb");
    MiaStorageContext context{root.c_str()};
    MiaStorageSaveRequest request{target, "tetris.sav", MIA_STORAGE_FLUSH_CORE_REQUEST, nullptr};
    auto first = bytes("native-save-v1");
    require_status(mia_storage_save_write(&context, &request, first.data(), first.size()), MIA_STORAGE_OK);
    require_true(read_text(root / "saves" / "gb" / "tetris.sav") == "native-save-v1", "save bytes should be written without conversion");

    MiaStorageFault fault{MIA_STORAGE_FAULT_BEFORE_REPLACE};
    MiaStorageSaveRequest interrupted{target, "tetris.sav", MIA_STORAGE_FLUSH_ROM_CHANGE, &fault};
    auto second = bytes("native-save-v2");
    require_status(mia_storage_save_write(&context, &interrupted, second.data(), second.size()), MIA_STORAGE_ERR_INTERRUPTED);
    require_true(read_text(root / "saves" / "gb" / "tetris.sav") == "native-save-v1", "interrupted writes should preserve prior save");
    require_true(!fs::exists(root / "saves" / "gb" / "tetris.sav.tmp"), "stale temp files should be removed");

    MiaStorageSaveRequest clean_exit{target, "tetris.sav", MIA_STORAGE_FLUSH_CLEAN_EXIT, nullptr};
    require_status(mia_storage_save_write(&context, &clean_exit, second.data(), second.size()), MIA_STORAGE_OK);
    require_true(read_text(root / "saves" / "gb" / "tetris.sav") == "native-save-v2", "clean exit flush should replace atomically");

    MiaStorageFault no_replace{MIA_STORAGE_FAULT_RENAME_NO_REPLACE};
    MiaStorageSaveRequest fat_replace{target, "tetris.sav", MIA_STORAGE_FLUSH_CORE_REQUEST, &no_replace};
    auto third = bytes("native-save-v3");
    require_status(mia_storage_save_write(&context, &fat_replace, third.data(), third.size()), MIA_STORAGE_OK);
    require_true(read_text(root / "saves" / "gb" / "tetris.sav") == "native-save-v3",
                 "FAT replacement should preserve save updates when rename cannot overwrite");
    require_true(!fs::exists(root / "saves" / "gb" / "tetris.sav.bak"), "successful FAT replacement should remove its backup");

    MiaStorageFault full{MIA_STORAGE_FAULT_WRITE_FULL};
    MiaStorageSaveRequest full_request{target, "full.sav", MIA_STORAGE_FLUSH_CORE_REQUEST, &full};
    require_status(mia_storage_save_write(&context, &full_request, second.data(), second.size()), MIA_STORAGE_ERR_IO);
    require_true(!fs::exists(root / "saves" / "gb" / "full.sav.tmp"), "full writes should leave no temp save");

    fs::create_directories(root / "saves" / "readonly");
    chmod((root / "saves" / "readonly").c_str(), 0555);
    const MiaStorageTarget readonly{"readonly", "/roms/gb", "/saves/readonly", target->extensions, target->extension_count, nullptr, 0};
    MiaStorageSaveRequest unwritable{&readonly, "blocked.sav", MIA_STORAGE_FLUSH_CORE_REQUEST, nullptr};
    require_status(mia_storage_save_write(&context, &unwritable, second.data(), second.size()), MIA_STORAGE_ERR_IO);
    chmod((root / "saves" / "readonly").c_str(), 0755);
}
