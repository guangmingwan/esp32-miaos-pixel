#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mia_app_input.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MIA_SMSPLUS_MODE_SMS = 0,
    MIA_SMSPLUS_MODE_GG,
    MIA_SMSPLUS_MODE_COLECO,
} MiaSmsPlusMode;

typedef struct {
    uint8_t pad;
    uint8_t system;
} MiaSmsPlusInput;

MiaSmsPlusInput mia_smsplus_map_input(MiaSmsPlusMode mode, uint32_t input);
bool mia_smsplus_convert_frame(const uint8_t *indices, size_t pixel_count,
                               const uint16_t *palette, size_t palette_count,
                               uint16_t *output, size_t output_count);
uint8_t mia_smsplus_coleco_keypad_code(size_t index);
#ifdef __cplusplus
}
#endif
