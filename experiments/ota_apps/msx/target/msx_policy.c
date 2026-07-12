#include "msx_policy.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

const char *const mia_msx_bios_files[MIA_MSX_BIOS_COUNT] = {
    "MSX2.ROM", "MSX2EXT.ROM",
};

static const uint8_t keys[MIA_MSX_KEYBOARD_ROWS][MIA_MSX_KEYBOARD_COLS] = {
    {0x1b, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x9a, 0x7f, 0x9b},
    {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '='},
    {'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '\b'},
    {'^', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\r'},
    {'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, 0},
    {'[', ']', ' ', ' ', ' ', ' ', ' ', '\\', '\'', 0, 0, 0},
};

static const char *extension(const char *path) {
    return path == NULL ? NULL : strrchr(path, '.');
}

bool mia_msx_media_supported(const char *path) {
    const char *ext = extension(path);
    return ext != NULL && (!strcasecmp(ext, ".rom") || !strcasecmp(ext, ".mx1") ||
                           !strcasecmp(ext, ".mx2") || !strcasecmp(ext, ".dsk"));
}

bool mia_msx_media_is_disk(const char *path) {
    const char *ext = extension(path);
#ifdef MIA_MSX_MUTANT_DISK_REJECTED
    return ext != NULL && !strcasecmp(ext, ".rom");
#else
    return ext != NULL && !strcasecmp(ext, ".dsk");
#endif
}

uint8_t mia_msx_keyboard_key(const MiaMsxKeyboard *keyboard) {
    return keyboard == NULL ? 0 : keys[keyboard->row][keyboard->col];
}

void mia_msx_keyboard_move(MiaMsxKeyboard *keyboard, int8_t dx, int8_t dy) {
    int col = keyboard->col + dx;
    int row = keyboard->row + dy;
    if (col < 0) col = 0;
    if (row < 0) row = 0;
    if (col >= MIA_MSX_KEYBOARD_COLS) col = MIA_MSX_KEYBOARD_COLS - 1;
    if (row >= MIA_MSX_KEYBOARD_ROWS) row = MIA_MSX_KEYBOARD_ROWS - 1;
    keyboard->col = (uint8_t)col;
    keyboard->row = (uint8_t)row;
}

size_t mia_msx_missing_bios(const char *directory, bool (*exists)(const char *),
                            char *message, size_t capacity) {
    size_t missing = 0;
    size_t used = 0;
    if (capacity != 0) message[0] = '\0';
    for (size_t index = 0; index < MIA_MSX_BIOS_COUNT; ++index) {
        char path[160];
        snprintf(path, sizeof(path), "%s/%s", directory, mia_msx_bios_files[index]);
        if (exists(path)) continue;
        ++missing;
        if (used < capacity) {
            int written = snprintf(message + used, capacity - used, "%s%s",
                                   used == 0 ? "" : ", ", mia_msx_bios_files[index]);
            if (written > 0) used += (size_t)written;
        }
    }
    return missing;
}
