#include "test_support.h"

#include "mia_runtime_target.h"

int main() {
    const MiaRuntimeTarget *target = nullptr;
    require_status(mia_runtime_target_find(&mia_runtime_generated_targets, "doom", &target), MIA_RUNTIME_OK);
    require_true(std::string(target->manifest_category) == "Games", "doom should keep Games manifest category");
    require_true(target->geometry.width == 320 && target->geometry.height == 200, "doom geometry should come from Todo 1 target data");

    require_status(mia_runtime_target_find(&mia_runtime_generated_targets, "col", &target), MIA_RUNTIME_OK);
    require_true(std::string(target->name) == "coleco", "target lookup should resolve aliases");
    require_true(std::string(target->rom_root) == "/roms/coleco", "target lookup should preserve rom root");

    require_status(mia_runtime_target_find(&mia_runtime_generated_targets, "../../doom", &target), MIA_RUNTIME_ERR_INVALID_ARGUMENT);
    require_status(mia_runtime_target_find(&mia_runtime_generated_targets, "not-real", &target), MIA_RUNTIME_ERR_TARGET_NOT_FOUND);
}
