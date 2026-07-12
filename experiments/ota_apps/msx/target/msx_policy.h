#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MIA_MSX_KEYBOARD_COLS 12u
#define MIA_MSX_KEYBOARD_ROWS 6u
#define MIA_MSX_BIOS_COUNT 2u

typedef struct {
    uint8_t row;
    uint8_t col;
    bool visible;
} MiaMsxKeyboard;

extern const char *const mia_msx_bios_files[MIA_MSX_BIOS_COUNT];

bool mia_msx_media_supported(const char *path);
bool mia_msx_media_is_disk(const char *path);
uint8_t mia_msx_keyboard_key(const MiaMsxKeyboard *keyboard);
void mia_msx_keyboard_move(MiaMsxKeyboard *keyboard, int8_t dx, int8_t dy);
size_t mia_msx_missing_bios(const char *directory, bool (*exists)(const char *),
                            char *message, size_t capacity);
