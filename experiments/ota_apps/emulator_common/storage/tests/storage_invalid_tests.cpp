#include "test_support.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

int main() {
    const MiaStorageTarget *coleco = nullptr;
    const MiaStorageTarget *doom = nullptr;
    require_status(mia_storage_target_find(&mia_storage_generated_targets, "coleco", &coleco), MIA_STORAGE_OK);
    require_status(mia_storage_target_find(&mia_storage_generated_targets, "doom", &doom), MIA_STORAGE_OK);
    auto root = make_temp_tree("invalid");
    fs::create_directories(root / "roms" / "doom");
    fs::create_directories(root / "roms" / "coleco");
    fs::create_directories(root / "saves" / "doom");
    std::ofstream(root / "roms" / "doom" / "bad.zip").put('z');

    MiaStorageContext context{root.c_str()};
    require_status(mia_storage_validate_requirements(&context, coleco), MIA_STORAGE_ERR_MISSING_REQUIRED_FILE);
    require_status(mia_storage_validate_requirements(&context, doom), MIA_STORAGE_ERR_MISSING_REQUIRED_FILE);
    std::ofstream(root / "roms" / "doom" / "doom1.wad").put('w');
    require_status(mia_storage_validate_requirements(&context, doom), MIA_STORAGE_OK);

    MiaStoragePickerResult result{};
    require_status(mia_storage_picker_select(&context, doom, "../doom/doom1.wad", &result), MIA_STORAGE_ERR_PATH_TRAVERSAL);
    require_status(mia_storage_picker_select(&context, doom, "bad.zip", &result), MIA_STORAGE_ERR_UNSUPPORTED_ARCHIVE);

    auto payload = bytes("x");
    MiaStorageSaveRequest traversal{doom, "../escape.sav", MIA_STORAGE_FLUSH_CORE_REQUEST, nullptr};
    require_status(mia_storage_save_write(&context, &traversal, payload.data(), payload.size()), MIA_STORAGE_ERR_PATH_TRAVERSAL);
    MiaStorageSaveRequest malformed{doom, "bad.sav", static_cast<MiaStorageFlushReason>(99), nullptr};
    require_status(mia_storage_save_write(&context, &malformed, payload.data(), payload.size()), MIA_STORAGE_ERR_INVALID_ARGUMENT);
    require_status(mia_storage_target_find(&mia_storage_generated_targets, "../../doom", &doom), MIA_STORAGE_ERR_INVALID_ARGUMENT);
}
