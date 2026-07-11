#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "mia_hardware_status.h"
#include "mia_hardware_target.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MIA_HOST_KEY_BOOT = 0,
    MIA_HOST_KEY_START,
    MIA_HOST_KEY_M,
    MIA_HOST_KEY_L,
    MIA_HOST_KEY_R,
    MIA_HOST_KEY_SELECT,
    MIA_HOST_KEY_A,
    MIA_HOST_KEY_B,
    MIA_HOST_KEY_X,
    MIA_HOST_KEY_Y,
    MIA_HOST_KEY_UP,
    MIA_HOST_KEY_DOWN,
    MIA_HOST_KEY_LEFT,
    MIA_HOST_KEY_RIGHT,
} MiaHostKey;

typedef enum {
    MIA_INPUT_ACTION_NONE = 0,
    MIA_INPUT_ACTION_A,
    MIA_INPUT_ACTION_B,
    MIA_INPUT_ACTION_UP,
    MIA_INPUT_ACTION_DOWN,
    MIA_INPUT_ACTION_LEFT,
    MIA_INPUT_ACTION_RIGHT,
    MIA_INPUT_ACTION_START,
    MIA_INPUT_ACTION_SELECT,
    MIA_INPUT_ACTION_KEYBOARD,
    MIA_INPUT_ACTION_FIRE,
    MIA_INPUT_ACTION_USE,
    MIA_INPUT_ACTION_STRAFE,
    MIA_INPUT_ACTION_RUN,
} MiaInputAction;

typedef struct {
    MiaHostKey host_key;
    MiaInputAction action;
} MiaInputMap;

typedef struct {
    uint32_t threshold_ms;
    uint32_t pressed_since_ms;
    bool armed;
    bool combo_down;
} MiaExitDebounce;

MiaHardwareStatus mia_input_map_host(const MiaHardwareTarget *target, MiaHostKey key, MiaInputMap *out_map);
void mia_input_exit_debounce_init(MiaExitDebounce *debounce, uint32_t threshold_ms);
bool mia_input_exit_requested(MiaExitDebounce *debounce, bool select_down, bool start_down, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
