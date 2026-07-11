#include "mia_emulator_runtime.h"
#include "mia_host_abi.h"

#include <stdio.h>
#include <string.h>

#define MIA_PICKER_VISIBLE 10u

typedef struct {
    const char *title;
    const char *scanning;
    const char *controls;
    const char *retry_controls;
    const char *missing_root;
    const char *no_roms;
} PickerText;

static const PickerText PICKER_EN = {"ROM Picker", "Scanning ROMs...", "UP/DN Select  A:Run  B:Exit", "A:Retry  B:Exit", "ROM root is missing", "No supported ROMs"};
static const PickerText PICKER_ZH = {"ROM选择", "正在扫描ROM...", "上/下 选择  A:运行  B:退出", "A:重试  B:退出", "ROM目录不存在", "没有支持的ROM"};

static const PickerText *picker_text(void) {
    return mia_host_language() == 1 ? &PICKER_ZH : &PICKER_EN;
}

static uint8_t utf8_length(const char *text) {
    const unsigned char first = (unsigned char)text[0];
    const size_t remaining = strnlen(text, 4);
    if (first < 0x80) return 1;
    if (remaining >= 2 && (first & 0xe0) == 0xc0 && (text[1] & 0xc0) == 0x80) return 2;
    if (remaining >= 3 && (first & 0xf0) == 0xe0 && (text[1] & 0xc0) == 0x80 && (text[2] & 0xc0) == 0x80) return 3;
    if (remaining >= 4 && (first & 0xf8) == 0xf0 && (text[1] & 0xc0) == 0x80 && (text[2] & 0xc0) == 0x80 && (text[3] & 0xc0) == 0x80) return 4;
    return 1;
}

static void copy_display_name(char *dest, size_t dest_size, const char *source, size_t max_cells) {
    size_t cells = 0;
    size_t output = 0;
    while (*source != '\0' && output + 1 < dest_size) {
        const uint8_t bytes = utf8_length(source);
        const size_t glyph_cells = bytes == 1 ? 1u : 2u;
        if (cells + glyph_cells > max_cells || output + bytes >= dest_size) break;
        memcpy(dest + output, source, bytes);
        output += bytes;
        source += bytes;
        cells += glyph_cells;
    }
    if (*source != '\0' && output + 1 < dest_size) dest[output++] = '.';
    dest[output] = '\0';
}

static void draw_picker(const MiaStorageTarget *target, const MiaStoragePickerResult *result, size_t selected, const char *message) {
    const PickerText *text = picker_text();
    mia_host_clear(MIA_HOST_BLACK);
    mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
    char title[48];
    snprintf(title, sizeof(title), "%s - %s", target->name, text->title);
    mia_host_draw_text(4, 2, title, MIA_HOST_BLACK, MIA_HOST_YELLOW);
    if (message != NULL) mia_host_draw_text(8, 34, message, MIA_HOST_RED, MIA_HOST_BLACK);
    size_t first = selected >= MIA_PICKER_VISIBLE ? selected - MIA_PICKER_VISIBLE + 1u : 0u;
    size_t row = 0;
    for (size_t index = first; index < result->count && row < MIA_PICKER_VISIBLE; ++index) {
        if (result->entries[index].kind != MIA_STORAGE_ENTRY_ROM) continue;
        const int32_t y = 42 + (int32_t)row * 16;
        const uint8_t active = index == selected;
        char display_name[128];
        copy_display_name(display_name, sizeof(display_name), result->entries[index].name, 38);
        mia_host_fill_rect(4, y - 2, 312, 14, active ? MIA_HOST_BLUE : MIA_HOST_BLACK);
        mia_host_draw_text(8, y, display_name, active ? MIA_HOST_YELLOW : MIA_HOST_WHITE, active ? MIA_HOST_BLUE : MIA_HOST_BLACK);
        ++row;
    }
    mia_host_draw_text(8, 222, message == NULL ? text->controls : text->retry_controls, MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_present();
}

static void draw_scan_status(const MiaStorageTarget *target) {
    const PickerText *text = picker_text();
    char title[48];
    snprintf(title, sizeof(title), "%s - %s", target->name, text->title);
    mia_host_clear(MIA_HOST_BLACK);
    mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
    mia_host_draw_text(4, 2, title, MIA_HOST_BLACK, MIA_HOST_YELLOW);
    mia_host_draw_text(72, 108, text->scanning, MIA_HOST_CYAN, MIA_HOST_BLACK);
    mia_host_present();
}

static bool wait_for_retry(void) {
    for (;;) {
        mia_host_buttons_poll();
        if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) return true;
        if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) return false;
        mia_host_delay_ms(20);
    }
}

static size_t next_rom(const MiaStoragePickerResult *result, size_t start, int direction) {
    size_t index = start;
    while ((direction > 0 && index + 1u < result->count) || (direction < 0 && index > 0u)) {
        index = direction > 0 ? index + 1u : index - 1u;
        if (result->entries[index].kind == MIA_STORAGE_ENTRY_ROM) return index;
    }
    return start;
}

MiaStorageStatus mia_emulator_picker_run(const MiaStorageContext *context, const MiaStorageTarget *target, MiaAppPickerSelection *selection) {
    MiaStoragePickerResult result;
    MiaStorageStatus status;
    for (;;) {
        draw_scan_status(target);
        status = mia_storage_picker_list(context, target, &result);
        if (status.code != MIA_STORAGE_OK) {
            char detail[160];
            const char *message = status.message;
            if (status.code == MIA_STORAGE_ERR_MISSING_ROOT && target != NULL && target->rom_root != NULL && status.message != NULL) {
                snprintf(detail, sizeof(detail), "%s: %s", picker_text()->missing_root, target->rom_root);
                message = detail;
            }
            draw_picker(target, &(MiaStoragePickerResult){0}, 0, message);
            if (!wait_for_retry()) return mia_storage_error(MIA_STORAGE_ERR_INTERRUPTED, "ROM selection cancelled");
            continue;
        }
        size_t first_rom = result.count;
        for (size_t index = 0; index < result.count; ++index) {
            if (result.entries[index].kind == MIA_STORAGE_ENTRY_ROM) { first_rom = index; break; }
        }
        if (first_rom != result.count) break;
        draw_picker(target, &result, 0, picker_text()->no_roms);
        mia_storage_picker_free(&result);
        if (!wait_for_retry()) return mia_storage_error(MIA_STORAGE_ERR_INTERRUPTED, "ROM selection cancelled");
    }
    size_t selected = 0;
    while (selected < result.count && result.entries[selected].kind != MIA_STORAGE_ENTRY_ROM) ++selected;
    draw_picker(target, &result, selected, NULL);
    for (;;) {
        mia_host_buttons_poll();
        if (mia_host_button_pressed(MIA_HOST_BUTTON_UP)) selected = next_rom(&result, selected, -1);
        if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN)) selected = next_rom(&result, selected, 1);
        if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) break;
        if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
            mia_storage_picker_free(&result);
            return mia_storage_error(MIA_STORAGE_ERR_INTERRUPTED, "ROM selection cancelled");
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_UP) || mia_host_button_pressed(MIA_HOST_BUTTON_DOWN)) draw_picker(target, &result, selected, NULL);
        mia_host_delay_ms(20);
    }
    status = mia_app_picker_select_entry(&result, selected, selection);
    mia_storage_picker_free(&result);
    return status;
}
