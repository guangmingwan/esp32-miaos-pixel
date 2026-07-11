#include "mia_hardware_input.h"

#include <string.h>

static MiaInputAction default_action(MiaHostKey key) {
    switch (key) {
    case MIA_HOST_KEY_A:
        return MIA_INPUT_ACTION_A;
    case MIA_HOST_KEY_B:
        return MIA_INPUT_ACTION_B;
    case MIA_HOST_KEY_UP:
        return MIA_INPUT_ACTION_UP;
    case MIA_HOST_KEY_DOWN:
        return MIA_INPUT_ACTION_DOWN;
    case MIA_HOST_KEY_LEFT:
        return MIA_INPUT_ACTION_LEFT;
    case MIA_HOST_KEY_RIGHT:
        return MIA_INPUT_ACTION_RIGHT;
    case MIA_HOST_KEY_START:
        return MIA_INPUT_ACTION_START;
    case MIA_HOST_KEY_SELECT:
        return MIA_INPUT_ACTION_SELECT;
    case MIA_HOST_KEY_BOOT:
    case MIA_HOST_KEY_M:
    case MIA_HOST_KEY_L:
    case MIA_HOST_KEY_R:
    case MIA_HOST_KEY_X:
    case MIA_HOST_KEY_Y:
        return MIA_INPUT_ACTION_NONE;
    }
    return MIA_INPUT_ACTION_NONE;
}

static MiaInputAction doom_action(MiaHostKey key) {
    switch (key) {
    case MIA_HOST_KEY_A:
        return MIA_INPUT_ACTION_FIRE;
    case MIA_HOST_KEY_B:
        return MIA_INPUT_ACTION_USE;
    case MIA_HOST_KEY_L:
        return MIA_INPUT_ACTION_STRAFE;
    case MIA_HOST_KEY_R:
        return MIA_INPUT_ACTION_RUN;
    default:
        return default_action(key);
    }
}

MiaHardwareStatus mia_input_map_host(const MiaHardwareTarget *target, MiaHostKey key, MiaInputMap *out_map) {
    if (target == NULL || out_map == NULL) {
        return mia_hardware_error(MIA_HARDWARE_ERR_INVALID_ARGUMENT, "invalid input map");
    }
    out_map->host_key = key;
    if (strcmp(target->name, "doom") == 0) {
        out_map->action = doom_action(key);
        return mia_hardware_ok();
    }
    if (strcmp(target->name, "msx") == 0 && key == MIA_HOST_KEY_M) {
        out_map->action = MIA_INPUT_ACTION_KEYBOARD;
        return mia_hardware_ok();
    }
    out_map->action = default_action(key);
    return mia_hardware_ok();
}

void mia_input_exit_debounce_init(MiaExitDebounce *debounce, uint32_t threshold_ms) {
    if (debounce == NULL) {
        return;
    }
    debounce->threshold_ms = threshold_ms;
    debounce->pressed_since_ms = 0;
    debounce->armed = true;
    debounce->combo_down = false;
}

bool mia_input_exit_requested(MiaExitDebounce *debounce, bool select_down, bool start_down, uint32_t now_ms) {
    if (debounce == NULL) {
        return false;
    }
    const bool combo = select_down && start_down;
    if (!combo) {
        debounce->combo_down = false;
        debounce->armed = true;
        debounce->pressed_since_ms = 0;
        return false;
    }
    if (!debounce->combo_down) {
        debounce->combo_down = true;
        debounce->pressed_since_ms = now_ms;
    }
    if (debounce->armed && now_ms - debounce->pressed_since_ms >= debounce->threshold_ms) {
        debounce->armed = false;
        return true;
    }
    return false;
}
