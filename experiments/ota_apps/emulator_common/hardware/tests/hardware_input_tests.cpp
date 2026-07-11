#include "test_support.h"

#include "mia_hardware_input.h"
#include "mia_hardware_target.h"

int main() {
    const MiaHardwareTarget *msx = nullptr;
    const MiaHardwareTarget *doom = nullptr;
    require_status(mia_hardware_target_find(&mia_hardware_generated_targets, "msx", &msx), MIA_HARDWARE_OK);
    require_status(mia_hardware_target_find(&mia_hardware_generated_targets, "doom", &doom), MIA_HARDWARE_OK);
    MiaInputMap mapped{};
    require_status(mia_input_map_host(msx, MIA_HOST_KEY_M, &mapped), MIA_HARDWARE_OK);
    require_true(mapped.action == MIA_INPUT_ACTION_KEYBOARD, "MSX M key should open keyboard action");
    require_status(mia_input_map_host(doom, MIA_HOST_KEY_A, &mapped), MIA_HARDWARE_OK);
    require_true(mapped.action == MIA_INPUT_ACTION_FIRE, "DOOM A should map to fire");
    require_status(mia_input_map_host(doom, MIA_HOST_KEY_B, &mapped), MIA_HARDWARE_OK);
    require_true(mapped.action == MIA_INPUT_ACTION_USE, "DOOM B should map to use");

    MiaExitDebounce debounce{};
    mia_input_exit_debounce_init(&debounce, 120);
    require_true(!mia_input_exit_requested(&debounce, true, false, 0), "single key should not exit");
    require_true(!mia_input_exit_requested(&debounce, true, true, 100), "combo must satisfy hold threshold");
    require_true(mia_input_exit_requested(&debounce, true, true, 221), "combo should fire once after debounce threshold");
    require_true(!mia_input_exit_requested(&debounce, true, true, 400), "held combo should not repeat");
    require_true(!mia_input_exit_requested(&debounce, false, false, 500), "release should only re-arm");
    require_true(!mia_input_exit_requested(&debounce, true, true, 621), "re-press starts a new debounce window");
    require_true(mia_input_exit_requested(&debounce, true, true, 741), "repeated interruption should work after release and debounce");
}
