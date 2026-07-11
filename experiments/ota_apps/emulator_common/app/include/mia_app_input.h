#pragma once

#include <stdint.h>

#include "mia_hardware_input.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIA_APP_CORE_INPUT_A 0x01u
#define MIA_APP_CORE_INPUT_B 0x02u
#define MIA_APP_CORE_INPUT_UP 0x04u
#define MIA_APP_CORE_INPUT_DOWN 0x08u
#define MIA_APP_CORE_INPUT_LEFT 0x10u
#define MIA_APP_CORE_INPUT_RIGHT 0x20u
#define MIA_APP_CORE_INPUT_START 0x40u
#define MIA_APP_CORE_INPUT_SELECT 0x80u

typedef struct {
    MiaExitDebounce exit_debounce;
} MiaAppInputState;

void mia_app_input_init(MiaAppInputState *state, uint32_t exit_threshold_ms);
uint32_t mia_app_input_core_mask(const MiaHardwareTarget *target, uint32_t host_button_bits);
bool mia_app_input_exit_requested(MiaAppInputState *state, uint32_t host_button_bits, uint32_t now_ms);
uint32_t mia_app_input_gnuboy_mask(uint32_t app_mask);
uint32_t mia_app_input_gw_mask(uint32_t app_mask);

#ifdef __cplusplus
}
#endif
