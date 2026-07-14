#include "mia_emulator_smsplus.h"

MiaSmsPlusInput mia_smsplus_map_input(MiaSmsPlusMode mode, uint32_t input) {
    MiaSmsPlusInput mapped = {0};
    if (input & MIA_APP_CORE_INPUT_UP) mapped.pad |= 0x01u;
    if (input & MIA_APP_CORE_INPUT_DOWN) mapped.pad |= 0x02u;
    if (input & MIA_APP_CORE_INPUT_LEFT) mapped.pad |= 0x04u;
    if (input & MIA_APP_CORE_INPUT_RIGHT) mapped.pad |= 0x08u;
    if (input & MIA_APP_CORE_INPUT_B) mapped.pad |= 0x10u;
    if (input & MIA_APP_CORE_INPUT_A) mapped.pad |= 0x20u;
    switch (mode) {
    case MIA_SMSPLUS_MODE_SMS:
        if (input & MIA_APP_CORE_INPUT_START) mapped.system |= 0x02u;
        if (input & MIA_APP_CORE_INPUT_SELECT) mapped.system |= 0x01u;
        break;
    case MIA_SMSPLUS_MODE_GG:
        if (input & MIA_APP_CORE_INPUT_START) mapped.system |= 0x01u;
        if (input & MIA_APP_CORE_INPUT_SELECT) mapped.system |= 0x02u;
        break;
    case MIA_SMSPLUS_MODE_COLECO:
        break;
    }
    return mapped;
}

bool mia_smsplus_convert_frame(const uint8_t *indices, size_t pixel_count,
                               const uint16_t *palette, size_t palette_count,
                               uint16_t *output, size_t output_count) {
    if (indices == NULL || palette == NULL || output == NULL || output_count < pixel_count) return false;
    for (size_t index = 0; index < pixel_count; ++index) {
        if (indices[index] >= palette_count) return false;
        output[index] = palette[indices[index]];
    }
    return true;
}

uint8_t mia_smsplus_coleco_keypad_code(size_t index) {
    static const uint8_t codes[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 11};
    return index < sizeof(codes) ? codes[index] : 0xffu;
}
