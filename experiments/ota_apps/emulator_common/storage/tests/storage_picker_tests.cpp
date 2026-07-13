#include "test_support.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

static bool has_entry(const std::vector<MiaStoragePickerEntry> &entries, std::string_view name, MiaStorageEntryKind kind) {
    return std::any_of(entries.begin(), entries.end(), [name, kind](const MiaStoragePickerEntry &entry) {
        return entry.kind == kind && name == entry.name;
    });
}

int main() {
    const MiaStorageTarget *target = nullptr;
    require_status(mia_storage_target_find(&mia_storage_generated_targets, "nes", &target), MIA_STORAGE_OK);
    auto root = make_temp_tree("picker");
    fs::create_directories(root / "roms" / "nes" / "subdir");
    fs::create_directories(root / "saves" / "nes");
    std::ofstream(root / "roms" / "nes" / "mario.nes").put('n');
    std::ofstream(root / "roms" / "nes" / "famicom.fc").put('f');
    std::ofstream(root / "roms" / "nes" / "music.nsf").put('s');
    std::ofstream(root / "roms" / "nes" / "archive.zip").put('z');
    const std::string long_name(251, 'a');
    std::ofstream(root / "roms" / "nes" / (long_name + ".nes")).put('l');
    std::ofstream(root / "roms" / "nes" / "マリオ.nes").put('u');
    symlink("/etc/passwd", (root / "roms" / "nes" / "outside.nes").c_str());

    MiaStorageContext context{root.c_str()};
    MiaStoragePickerResult result{};
    require_status(mia_storage_picker_list(&context, target, &result), MIA_STORAGE_OK);
    auto entries = picker_entries(result);
    require_true(has_entry(entries, "subdir", MIA_STORAGE_ENTRY_DIRECTORY), "directories should be visible");
    require_true(has_entry(entries, "mario.nes", MIA_STORAGE_ENTRY_ROM), "nes extension should match target data");
    require_true(has_entry(entries, "famicom.fc", MIA_STORAGE_ENTRY_ROM), "fc extension should match target data");
    require_true(has_entry(entries, "music.nsf", MIA_STORAGE_ENTRY_ROM), "nsf extension should match target data");
    require_true(has_entry(entries, "マリオ.nes", MIA_STORAGE_ENTRY_ROM), "UTF-8 names should be preserved");
    require_true(has_entry(entries, long_name + ".nes", MIA_STORAGE_ENTRY_ROM), "255-byte names should be preserved");
    require_true(!has_entry(entries, "archive.zip", MIA_STORAGE_ENTRY_ROM), "ZIP archives should be rejected");
    require_true(!has_entry(entries, "outside.nes", MIA_STORAGE_ENTRY_ROM), "symlinks outside roots should not be followed");
    mia_storage_picker_free(&result);

    const MiaStorageTarget missing_root{"missing", "/roms/missing", "/saves/missing", target->extensions, target->extension_count, nullptr, 0, nullptr, 0};
    const MiaStorageStatus missing_root_status = mia_storage_picker_list(&context, &missing_root, &result);
    require_status(missing_root_status, MIA_STORAGE_ERR_MISSING_ROOT);
    require_true(missing_root_status.message != nullptr, "missing root should include a status message");
    require_true(std::string_view(missing_root_status.message) == "ROM root is missing",
                 "missing root should keep the base status message");
    require_true(std::string_view(missing_root.rom_root) == "/roms/missing",
                 "missing root target should preserve its rom_root");

    fs::create_directories(root / "sd" / "roms" / "GBC");
    std::ofstream(root / "sd" / "roms" / "GBC" / "demo.gbc").put('g');
    const std::string mounted_root = (root / "sd").string();
    const MiaStorageContext mounted_context{mounted_root.c_str()};
    const char *gbc_extensions[] = {"gbc"};
    const MiaStorageTarget uppercase_root{"gbc", "/roms/gbc", "/saves/gbc", gbc_extensions, 1, nullptr, 0, nullptr, 0};
    require_status(mia_storage_picker_list(&mounted_context, &uppercase_root, &result), MIA_STORAGE_OK);
    entries = picker_entries(result);
    require_true(has_entry(entries, "demo.gbc", MIA_STORAGE_ENTRY_ROM),
                 "lowercase target root should find an uppercase ROM directory");
    mia_storage_picker_free(&result);
    require_status(mia_storage_picker_select(&mounted_context, &uppercase_root, "demo.gbc", &result), MIA_STORAGE_OK);
    require_true(result.count == 1 && std::string_view(result.entries[0].path).find("/sd/roms/GBC/demo.gbc") != std::string_view::npos,
                 "selected ROM should preserve the existing uppercase directory path");
    mia_storage_picker_free(&result);

    fs::create_directories(root / "sd" / "roms" / "FC");
    std::ofstream(root / "sd" / "roms" / "FC" / "famicom.fc").put('f');
    const char *nes_alternate_roots[] = {"/roms/FC"};
    const MiaStorageTarget multi_root_nes{
        "nes", "/roms/nes", "/saves/nes", target->extensions, target->extension_count,
        nullptr, 0, nes_alternate_roots, 1};
    require_status(mia_storage_picker_list(&mounted_context, &multi_root_nes, &result), MIA_STORAGE_OK);
    entries = picker_entries(result);
    require_true(has_entry(entries, "famicom.fc", MIA_STORAGE_ENTRY_ROM),
                 "alternate ROM roots should be merged into the picker");
    mia_storage_picker_free(&result);
    require_status(mia_storage_picker_select(&mounted_context, &multi_root_nes, "famicom.fc", &result), MIA_STORAGE_OK);
    require_true(result.count == 1 && std::string_view(result.entries[0].path).find("/sd/roms/FC/famicom.fc") != std::string_view::npos,
                 "selection should preserve an alternate root path");
    mia_storage_picker_free(&result);

    fs::create_directories(root / "roms" / "empty");
    const MiaStorageTarget empty_root{"empty", "/roms/empty", "/saves/empty", target->extensions, target->extension_count, nullptr, 0, nullptr, 0};
    require_status(mia_storage_picker_list(&context, &empty_root, &result), MIA_STORAGE_OK);
    require_true(result.count == 0, "empty roots should return an empty picker result");
    mia_storage_picker_free(&result);

    for (size_t index = 0; index < mia_storage_generated_targets.count; ++index) {
        const MiaStorageTarget *candidate = &mia_storage_generated_targets.targets[index];
        require_true(candidate->rom_root[0] == '/', "rom roots should come from generated target data");
        require_true(candidate->extension_count > 0, "every generated target should have filters");
    }
}
