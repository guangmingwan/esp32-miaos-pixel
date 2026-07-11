#include "mia_app_input.h"

static bool host_bit(uint32_t buttons, MiaHostKey key) {
    return (buttons & (1u << (uint32_t)key)) != 0u;
}

static uint32_t bit_for_action(MiaInputAction action) {
    switch (action) {
    case MIA_INPUT_ACTION_A:
    case MIA_INPUT_ACTION_FIRE:
        return MIA_APP_CORE_INPUT_A;
    case MIA_INPUT_ACTION_B:
    case MIA_INPUT_ACTION_USE:
        return MIA_APP_CORE_INPUT_B;
    case MIA_INPUT_ACTION_UP:
        return MIA_APP_CORE_INPUT_UP;
    case MIA_INPUT_ACTION_DOWN:
        return MIA_APP_CORE_INPUT_DOWN;
    case MIA_INPUT_ACTION_LEFT:
        return MIA_APP_CORE_INPUT_LEFT;
    case MIA_INPUT_ACTION_RIGHT:
        return MIA_APP_CORE_INPUT_RIGHT;
    case MIA_INPUT_ACTION_START:
        return MIA_APP_CORE_INPUT_START;
    case MIA_INPUT_ACTION_SELECT:
        return MIA_APP_CORE_INPUT_SELECT;
    case MIA_INPUT_ACTION_NONE:
    case MIA_INPUT_ACTION_KEYBOARD:
    case MIA_INPUT_ACTION_STRAFE:
    case MIA_INPUT_ACTION_RUN:
        return 0;
    }
    return 0;
}

void mia_app_input_init(MiaAppInputState *state, uint32_t exit_threshold_ms) {
    if (state != NULL) {
        mia_input_exit_debounce_init(&state->exit_debounce, exit_threshold_ms);
    }
}

uint32_t mia_app_input_core_mask(const MiaHardwareTarget *target, uint32_t host_button_bits) {
    uint32_t mask = 0;
    for (MiaHostKey key = MIA_HOST_KEY_BOOT; key <= MIA_HOST_KEY_RIGHT; key = (MiaHostKey)((uint32_t)key + 1u)) {
        if (!host_bit(host_button_bits, key)) {
            continue;
        }
        MiaInputMap map;
        if (mia_input_map_host(target, key, &map).code == MIA_HARDWARE_OK) {
            mask |= bit_for_action(map.action);
        }
    }
    return mask;
}

bool mia_app_input_exit_requested(MiaAppInputState *state, uint32_t host_button_bits, uint32_t now_ms) {
    return state != NULL && mia_input_exit_requested(&state->exit_debounce, host_bit(host_button_bits, MIA_HOST_KEY_SELECT), host_bit(host_button_bits, MIA_HOST_KEY_START), now_ms);
}

uint32_t mia_app_input_gnuboy_mask(uint32_t input) {
    uint32_t mapped = 0;
    const uint32_t horizontal = input & (MIA_APP_CORE_INPUT_LEFT | MIA_APP_CORE_INPUT_RIGHT);
    const uint32_t vertical = input & (MIA_APP_CORE_INPUT_UP | MIA_APP_CORE_INPUT_DOWN);
    /* This board's GBC image is mirrored on both input axes at the gnuboy boundary. */
    if (horizontal == MIA_APP_CORE_INPUT_LEFT) mapped |= 0x01u;
    if (horizontal == MIA_APP_CORE_INPUT_RIGHT) mapped |= 0x02u;
    if (vertical == MIA_APP_CORE_INPUT_DOWN) mapped |= 0x04u;
    if (vertical == MIA_APP_CORE_INPUT_UP) mapped |= 0x08u;
    if (input & MIA_APP_CORE_INPUT_A) mapped |= 0x10u;
    if (input & MIA_APP_CORE_INPUT_B) mapped |= 0x20u;
    if (input & MIA_APP_CORE_INPUT_SELECT) mapped |= 0x40u;
    if (input & MIA_APP_CORE_INPUT_START) mapped |= 0x80u;
    return mapped;
}

uint32_t mia_app_input_gw_mask(uint32_t input) {
    uint32_t mapped = 0;
    if (input & MIA_APP_CORE_INPUT_LEFT) mapped |= 0x01u;
    if (input & MIA_APP_CORE_INPUT_UP) mapped |= 0x02u;
    if (input & MIA_APP_CORE_INPUT_RIGHT) mapped |= 0x04u;
    if (input & MIA_APP_CORE_INPUT_DOWN) mapped |= 0x08u;
    if (input & MIA_APP_CORE_INPUT_A) mapped |= 0x10u;
    if (input & MIA_APP_CORE_INPUT_B) mapped |= 0x20u;
    if (input & MIA_APP_CORE_INPUT_SELECT) mapped |= 0x40u;
    if (input & MIA_APP_CORE_INPUT_START) mapped |= 0x80u;
    return mapped;
}
