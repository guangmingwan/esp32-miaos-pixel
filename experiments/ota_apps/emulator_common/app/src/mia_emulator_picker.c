#include "mia_emulator_runtime.h"
#include "mia_host_abi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MIA_PICKER_COVERS
#include "display_host.h"
#include <esp32s3/rom/tjpgd.h>
#endif

#define MIA_PICKER_VISIBLE 10u

#ifdef MIA_PICKER_COVERS
#define MIA_COVER_X 202
#define MIA_COVER_Y 34
#define MIA_COVER_WIDTH 112u
#define MIA_COVER_HEIGHT 176u
#define MIA_COVER_DECODE_STRIDE 320u
#define MIA_COVER_JPEG_WORK_SIZE 3100u

typedef struct {
    FILE *file;
    uint16_t *pixels;
} PickerJpegDevice;

static UINT cover_jpeg_input(JDEC *decoder, BYTE *buffer, UINT length) {
    PickerJpegDevice *device = decoder != NULL ? decoder->device : NULL;
    if (device == NULL || device->file == NULL) return 0;
    if (buffer == NULL) return fseek(device->file, (long)length, SEEK_CUR) == 0 ? length : 0;
    return (UINT)fread(buffer, 1, length, device->file);
}

static UINT cover_jpeg_output(JDEC *decoder, void *bitmap, JRECT *rect) {
    PickerJpegDevice *device = decoder != NULL ? decoder->device : NULL;
    if (device == NULL || device->pixels == NULL || bitmap == NULL || rect == NULL ||
        rect->right >= MIA_COVER_DECODE_STRIDE || rect->bottom >= MIA_DISPLAY_HEIGHT) return 0;
    const uint8_t *rgb = bitmap;
    const UINT block_width = (UINT)(rect->right - rect->left + 1u);
    for (UINT y = rect->top; y <= rect->bottom; ++y) {
        uint16_t *output = device->pixels + (size_t)y * MIA_COVER_DECODE_STRIDE + rect->left;
        for (UINT x = 0; x < block_width; ++x) {
            const uint8_t r = *rgb++;
            const uint8_t g = *rgb++;
            const uint8_t b = *rgb++;
            output[x] = (uint16_t)(((uint16_t)(r & 0xf8u) << 8) |
                                   ((uint16_t)(g & 0xfcu) << 3) | (b >> 3));
        }
    }
    return 1;
}

static bool cover_path(const char *rom_path, char *path, size_t path_size) {
    const char *slash = strrchr(rom_path, '/');
    const char *name = slash != NULL ? slash + 1 : rom_path;
    const char *dot = strrchr(name, '.');
    if (dot == NULL || dot == name) return false;
    const size_t directory_length = slash != NULL ? (size_t)(slash - rom_path) : 0u;
    const int written = snprintf(path, path_size, "%.*s%simages/%.*s.jpg",
                                 (int)directory_length, rom_path,
                                 slash != NULL ? "/" : "", (int)(dot - name), name);
    return written > 0 && (size_t)written < path_size;
}

static bool decode_cover(const char *path, uint16_t *pixels, uint16_t *width, uint16_t *height) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) return false;
    void *work = malloc(MIA_COVER_JPEG_WORK_SIZE);
    if (work == NULL) {
        fclose(file);
        return false;
    }
    PickerJpegDevice device = {file, pixels};
    JDEC decoder;
    JRESULT result = jd_prepare(&decoder, cover_jpeg_input, work, MIA_COVER_JPEG_WORK_SIZE, &device);
    uint8_t scale = 0;
    if (result == JDR_OK) {
        while (scale < 3u && ((decoder.width >> scale) > MIA_COVER_DECODE_STRIDE ||
                              (decoder.height >> scale) > MIA_DISPLAY_HEIGHT)) ++scale;
        *width = (uint16_t)((decoder.width + (1u << scale) - 1u) >> scale);
        *height = (uint16_t)((decoder.height + (1u << scale) - 1u) >> scale);
        if (*width > MIA_COVER_DECODE_STRIDE || *height > MIA_DISPLAY_HEIGHT) result = JDR_PAR;
    }
    if (result == JDR_OK) result = jd_decomp(&decoder, cover_jpeg_output, scale);
    free(work);
    fclose(file);
    return result == JDR_OK;
}

static void draw_cover(const MiaStoragePickerEntry *entry) {
    char path[512];
    if (entry == NULL || !cover_path(entry->path, path, sizeof(path))) return;
    uint16_t source_width = 0;
    uint16_t source_height = 0;
    uint16_t *source = mia_emulator_runtime.display;
    if (source == NULL || !decode_cover(path, source, &source_width, &source_height)) return;
    uint16_t *cover = malloc(MIA_COVER_WIDTH * MIA_COVER_HEIGHT * sizeof(uint16_t));
    if (cover == NULL) return;
    memset(cover, 0, MIA_COVER_WIDTH * MIA_COVER_HEIGHT * sizeof(uint16_t));
    uint32_t draw_width = MIA_COVER_WIDTH;
    uint32_t draw_height = (uint32_t)source_height * draw_width / source_width;
    if (draw_height > MIA_COVER_HEIGHT) {
        draw_height = MIA_COVER_HEIGHT;
        draw_width = (uint32_t)source_width * draw_height / source_height;
    }
    const uint32_t offset_x = (MIA_COVER_WIDTH - draw_width) / 2u;
    const uint32_t offset_y = (MIA_COVER_HEIGHT - draw_height) / 2u;
    for (uint32_t y = 0; y < draw_height; ++y) {
        const uint32_t source_y = y * source_height / draw_height;
        for (uint32_t x = 0; x < draw_width; ++x) {
            const uint32_t source_x = x * source_width / draw_width;
            cover[(offset_y + y) * MIA_COVER_WIDTH + offset_x + x] =
                source[source_y * MIA_COVER_DECODE_STRIDE + source_x];
        }
    }
    (void)display_host_present_rgb565_region(cover, MIA_COVER_X, MIA_COVER_Y,
                                             MIA_COVER_WIDTH, MIA_COVER_HEIGHT,
                                             MIA_COVER_WIDTH * sizeof(uint16_t));
    free(cover);
}
#endif

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

static void format_picker_title(const MiaStorageTarget *target, const char *rom_root,
                                char *title, size_t title_size) {
    char full_title[192];
    snprintf(full_title, sizeof(full_title), "%s-%s %s", target->name,
             picker_text()->title, rom_root);
    copy_display_name(title, title_size, full_title, 50);
}

static void draw_picker(const MiaStorageTarget *target, const char *rom_root, const MiaStoragePickerResult *result, size_t selected, const char *message) {
    const PickerText *text = picker_text();
    mia_host_clear(MIA_HOST_BLACK);
    mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
    char title[192];
    format_picker_title(target, rom_root, title, sizeof(title));
    mia_host_draw_text(4, 2, title, MIA_HOST_BLACK, MIA_HOST_YELLOW);
    if (message != NULL) mia_host_draw_text(8, 34, message, MIA_HOST_RED, MIA_HOST_BLACK);
    size_t first = selected >= MIA_PICKER_VISIBLE ? selected - MIA_PICKER_VISIBLE + 1u : 0u;
    size_t row = 0;
    for (size_t index = first; index < result->count && row < MIA_PICKER_VISIBLE; ++index) {
        if (result->entries[index].kind != MIA_STORAGE_ENTRY_ROM) continue;
        const int32_t y = 42 + (int32_t)row * 16;
        const uint8_t active = index == selected;
        char display_name[128];
        copy_display_name(display_name, sizeof(display_name), result->entries[index].name,
#ifdef MIA_PICKER_COVERS
                          22
#else
                          38
#endif
        );
        mia_host_fill_rect(4, y - 2,
#ifdef MIA_PICKER_COVERS
                           190,
#else
                           312,
#endif
                           14, active ? MIA_HOST_BLUE : MIA_HOST_BLACK);
        mia_host_draw_text(8, y, display_name, active ? MIA_HOST_YELLOW : MIA_HOST_WHITE, active ? MIA_HOST_BLUE : MIA_HOST_BLACK);
        ++row;
    }
#ifdef MIA_PICKER_COVERS
    mia_host_fill_rect(MIA_COVER_X - 2, MIA_COVER_Y - 2, MIA_COVER_WIDTH + 4,
                       MIA_COVER_HEIGHT + 4, MIA_HOST_GRAY);
    mia_host_fill_rect(MIA_COVER_X, MIA_COVER_Y, MIA_COVER_WIDTH, MIA_COVER_HEIGHT,
                       MIA_HOST_BLACK);
    mia_host_draw_text(MIA_COVER_X + 27, MIA_COVER_Y + 84, "NO COVER",
                       MIA_HOST_GRAY, MIA_HOST_BLACK);
#endif
    mia_host_draw_text(8, 222, message == NULL ? text->controls : text->retry_controls, MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_present();
#ifdef MIA_PICKER_COVERS
    if (message == NULL && selected < result->count) draw_cover(&result->entries[selected]);
#endif
}

static void draw_scan_status(const MiaStorageTarget *target, const char *rom_root) {
    const PickerText *text = picker_text();
    char title[192];
    format_picker_title(target, rom_root, title, sizeof(title));
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
    char rom_root[160];
    status = mia_storage_rom_root_path(context, target, rom_root, sizeof(rom_root));
    if (status.code != MIA_STORAGE_OK) {
        snprintf(rom_root, sizeof(rom_root), "%s%s", context->storage_root, target->rom_root);
    }
    for (;;) {
        draw_scan_status(target, rom_root);
        status = mia_storage_picker_list(context, target, &result);
        if (status.code != MIA_STORAGE_OK) {
            char detail[160];
            const char *message = status.message;
            if (status.code == MIA_STORAGE_ERR_MISSING_ROOT && target != NULL && target->rom_root != NULL && status.message != NULL) {
                snprintf(detail, sizeof(detail), "%s: %s", picker_text()->missing_root, target->rom_root);
                message = detail;
            }
            draw_picker(target, rom_root, &(MiaStoragePickerResult){0}, 0, message);
            if (!wait_for_retry()) return mia_storage_error(MIA_STORAGE_ERR_INTERRUPTED, "ROM selection cancelled");
            continue;
        }
        size_t first_rom = result.count;
        for (size_t index = 0; index < result.count; ++index) {
            if (result.entries[index].kind == MIA_STORAGE_ENTRY_ROM) { first_rom = index; break; }
        }
        if (first_rom != result.count) break;
        draw_picker(target, rom_root, &result, 0, picker_text()->no_roms);
        mia_storage_picker_free(&result);
        if (!wait_for_retry()) return mia_storage_error(MIA_STORAGE_ERR_INTERRUPTED, "ROM selection cancelled");
    }
    size_t selected = 0;
    while (selected < result.count && result.entries[selected].kind != MIA_STORAGE_ENTRY_ROM) ++selected;
    draw_picker(target, rom_root, &result, selected, NULL);
    for (;;) {
        mia_host_buttons_poll();
        if (mia_host_button_pressed(MIA_HOST_BUTTON_UP)) selected = next_rom(&result, selected, -1);
        if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN)) selected = next_rom(&result, selected, 1);
        if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) break;
        if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
            mia_storage_picker_free(&result);
            return mia_storage_error(MIA_STORAGE_ERR_INTERRUPTED, "ROM selection cancelled");
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_UP) || mia_host_button_pressed(MIA_HOST_BUTTON_DOWN)) draw_picker(target, rom_root, &result, selected, NULL);
        mia_host_delay_ms(20);
    }
    status = mia_app_picker_select_entry(&result, selected, selection);
    mia_storage_picker_free(&result);
    return status;
}
