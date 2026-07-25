#include "mia_emulator_runtime.h"
#include "mia_hardware_display.h"
#include "mia_host_abi.h"
#include "display_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MIA_PICKER_COVERS
#include <esp32s3/rom/tjpgd.h>
#endif

#define MIA_PICKER_VISIBLE 10u
#define MIA_PICKER_COVER_DELAY_MS 250u

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
            const uint16_t pixel = (uint16_t)(((uint16_t)(r & 0xf8u) << 8) |
                                              ((uint16_t)(g & 0xfcu) << 3) | (b >> 3));
#ifdef MIA_DISPLAY_RGB565_WIRE_ORDER
            output[x] = __builtin_bswap16(pixel);
#else
            output[x] = pixel;
#endif
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
    uint16_t *source = malloc(MIA_COVER_DECODE_STRIDE * MIA_DISPLAY_HEIGHT * sizeof(uint16_t));
    if (source == NULL) return;
    if (!decode_cover(path, source, &source_width, &source_height)) {
        free(source);
        return;
    }
    uint16_t *cover = malloc(MIA_COVER_WIDTH * MIA_COVER_HEIGHT * sizeof(uint16_t));
    if (cover == NULL) {
        free(source);
        return;
    }
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
    free(source);
}
#endif

typedef struct {
    const char *title;
    const char *scanning;
    const char *loading;
    const char *controls;
    const char *retry_controls;
    const char *missing_root;
    const char *no_roms;
} PickerText;

static const PickerText PICKER_EN = {"ROM Picker", "Scanning ROMs...", "Loading...", "UP/DN Select LT/RT Page A:Run B:Exit M:Menu", "A:Retry B:Exit M:Menu", "ROM root is missing", "No supported ROMs"};
static const PickerText PICKER_ZH = {"ROM选择", "正在扫描ROM...", "正在加载...", "上/下选择 左/右翻页 A:运行 B:退出 M:游戏时退出", "A:重试 B:退出 M:游戏时退出", "ROM目录不存在", "没有支持的ROM"};

static const PickerText *picker_text(void) {
    return mia_host_language() == 1 ? &PICKER_ZH : &PICKER_EN;
}

typedef struct {
    const char *title;
    const char *resume;
    const char *save_state_format;
    const char *load_state_format;
    const char *brightness_format;
    const char *volume_format;
    const char *scale_format;
    const char *scale_fit;
    const char *scale_crop;
    const char *scale_stretch;
    const char *rom_picker;
    const char *exit_launcher;
    const char *controls;
    const char *state_saved_format;
    const char *state_missing_format;
    const char *state_error;
    const char *state_unsupported;
} EmulatorMenuText;

static const EmulatorMenuText MENU_EN = {
    "Game Menu", "Resume", "Save Slot %u", "Load Slot %u", "Brightness %u%%", "Volume %u%%",
    "Scale %s", "Fit", "Crop", "Stretch", "ROM Picker", "Exit Launcher",
    "A:Select B:Resume LT/RT:Slot", "Slot %u saved", "Slot %u is empty", "State operation failed",
    "Save states unsupported"};
static const EmulatorMenuText MENU_ZH = {
    "游戏菜单", "继续游戏", "保存到槽位%u", "读取槽位%u", "亮度 %u%%", "音量 %u%%",
    "缩放 %s", "适应", "裁切", "拉伸", "返回ROM列表", "退出到启动器",
    "A:确认 B:返回 左/右:槽位", "槽位%u已保存", "槽位%u为空", "状态操作失败", "该模拟器暂不支持"};

static const EmulatorMenuText *menu_text(void) {
    return mia_host_language() == 1 ? &MENU_ZH : &MENU_EN;
}

static void draw_centered_text(int32_t y, const char *text, uint8_t fg, uint8_t bg) {
    int32_t x = (mia_host_screen_width() - mia_host_text_width(text)) / 2;
    if (x < 0) x = 0;
    mia_host_draw_text(x, y, text, fg, bg);
}

static void draw_emulator_menu(const char *target_name, const uint16_t *screenshot,
                               size_t selected, MiaEmulatorMenuNotice notice, uint8_t state_slot,
                               uint8_t brightness, uint8_t volume,
                               MiaDisplayScaleMode scale_mode) {
    const EmulatorMenuText *text = menu_text();
    char save_state[40];
    char load_state[40];
    char brightness_text[40];
    char volume_text[40];
    char scale_text[40];
    snprintf(save_state, sizeof(save_state), text->save_state_format, state_slot);
    snprintf(load_state, sizeof(load_state), text->load_state_format, state_slot);
    snprintf(brightness_text, sizeof(brightness_text), text->brightness_format, brightness);
    snprintf(volume_text, sizeof(volume_text), text->volume_format, volume);
    const char *scale_name = scale_mode == MIA_DISPLAY_SCALE_CROP ? text->scale_crop :
        (scale_mode == MIA_DISPLAY_SCALE_STRETCH ? text->scale_stretch : text->scale_fit);
    snprintf(scale_text, sizeof(scale_text), text->scale_format, scale_name);
    const char *items[] = {text->resume, save_state, load_state, brightness_text,
                           volume_text, scale_text,
                           text->rom_picker, text->exit_launcher};
    const int32_t screen_width = mia_host_screen_width();
    const int32_t screen_height = mia_host_screen_height();
    const int32_t panel_width = screen_width * 60 / 100;
    const int32_t panel_height = screen_height * 80 / 100;
    const int32_t panel_x = (screen_width - panel_width) / 2;
    const int32_t panel_y = (screen_height - panel_height) / 2;
    const int32_t item_x = panel_x + 6;
    const int32_t item_width = panel_width - 12;
    char title[64];
    snprintf(title, sizeof(title), "%s - %s", text->title,
             target_name != NULL ? target_name : "");

    mia_host_clear(255);
    mia_host_fill_rect(panel_x, panel_y, panel_width, panel_height, MIA_HOST_DARK_BLUE);
    draw_centered_text(panel_y + 8, title, MIA_HOST_WHITE, MIA_HOST_DARK_BLUE);
    for (size_t index = 0; index < 8u; ++index) {
        const int32_t y = panel_y + 27 + (int32_t)index * 19;
        const bool is_selected = index == selected;
        const uint8_t foreground = is_selected ? MIA_HOST_BLACK : MIA_HOST_WHITE;
        const uint8_t background = is_selected ? MIA_HOST_WHITE : MIA_HOST_DARK_BLUE;
        if (is_selected) mia_host_fill_rect(item_x, y, item_width, 17, MIA_HOST_WHITE);
        draw_centered_text(mia_host_text_y_centered(y, 17), items[index],
                           foreground, background);
    }
    const char *footer = NULL;
    char state_notice[40];
    uint8_t footer_color = MIA_HOST_GRAY;
    if (notice == MIA_EMULATOR_MENU_NOTICE_STATE_SAVED) {
        snprintf(state_notice, sizeof(state_notice), text->state_saved_format, state_slot);
        footer = state_notice;
        footer_color = MIA_HOST_GREEN;
    } else if (notice == MIA_EMULATOR_MENU_NOTICE_STATE_MISSING) {
        snprintf(state_notice, sizeof(state_notice), text->state_missing_format, state_slot);
        footer = state_notice;
        footer_color = MIA_HOST_YELLOW;
    } else if (notice == MIA_EMULATOR_MENU_NOTICE_STATE_ERROR) {
        footer = text->state_error;
        footer_color = MIA_HOST_RED;
    } else if (notice == MIA_EMULATOR_MENU_NOTICE_STATE_UNSUPPORTED) {
        footer = text->state_unsupported;
        footer_color = MIA_HOST_YELLOW;
    }
    if (footer != NULL) {
        draw_centered_text(panel_y + panel_height - 14, footer, footer_color, MIA_HOST_DARK_BLUE);
    }
    if (screenshot != NULL) {
        (void)display_host_present_rgb565_overlay(
            screenshot, MIA_DISPLAY_WIDTH, MIA_DISPLAY_HEIGHT,
            MIA_DISPLAY_WIDTH * sizeof(uint16_t), 255u, 230u);
    } else {
        mia_host_present();
    }
}

static void wait_menu_input_release(void) {
    for (unsigned guard = 0; guard < 500u; ++guard) {
        mia_host_buttons_poll();
        bool any_down = false;
        for (uint8_t key = MIA_HOST_BUTTON_BOOT; key <= MIA_HOST_BUTTON_RIGHT; ++key) {
            any_down = any_down || mia_host_button_down(key);
        }
        if (!any_down) return;
        mia_host_delay_ms(10);
    }
}

MiaEmulatorMenuAction mia_emulator_menu_run(const char *target_name,
                                            uint16_t *screenshot,
                                            MiaEmulatorMenuNotice notice,
                                            uint8_t *state_slot,
                                            MiaDisplayScaleMode *scale_mode) {
    size_t selected = 0;
    if (state_slot == NULL || scale_mode == NULL) return MIA_EMULATOR_MENU_RESUME;
    *state_slot %= 10u;
    uint8_t brightness = mia_host_brightness_get();
    uint8_t volume = mia_host_volume_get();
    wait_menu_input_release();
    if (screenshot != NULL) {
        (void)display_host_capture_rgb565(screenshot, MIA_DISPLAY_WIDTH, MIA_DISPLAY_HEIGHT,
                                          MIA_DISPLAY_WIDTH * sizeof(uint16_t));
    }
    draw_emulator_menu(target_name, screenshot, selected, notice, *state_slot,
                       brightness, volume, *scale_mode);
    for (;;) {
        mia_host_buttons_poll();
        size_t next = selected;
        if (mia_host_button_pressed(MIA_HOST_BUTTON_UP)) next = selected == 0u ? 7u : selected - 1u;
        if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN)) next = (selected + 1u) % 8u;
        const bool left = mia_host_button_pressed(MIA_HOST_BUTTON_LEFT);
        const bool right = mia_host_button_pressed(MIA_HOST_BUTTON_RIGHT);
        if ((selected == MIA_EMULATOR_MENU_SAVE_STATE ||
             selected == MIA_EMULATOR_MENU_LOAD_STATE) &&
            left) {
            *state_slot = *state_slot == 0u ? 9u : *state_slot - 1u;
        }
        if ((selected == MIA_EMULATOR_MENU_SAVE_STATE ||
             selected == MIA_EMULATOR_MENU_LOAD_STATE) &&
            right) {
            *state_slot = (*state_slot + 1u) % 10u;
        }
        if (selected == 3u && (left || right)) {
            brightness = left ? (brightness <= 10u ? 10u : brightness - 10u) :
                                (brightness >= 100u ? 100u : brightness + 10u);
            (void)mia_host_brightness_set(brightness);
        }
        if (selected == 4u && (left || right)) {
            volume = left ? (volume < 5u ? 0u : volume - 5u) :
                            (volume >= 100u ? 100u : volume + 5u);
            (void)mia_host_volume_set(volume);
        }
        if (selected == 5u && (left || right)) {
            if (left) *scale_mode = *scale_mode == MIA_DISPLAY_SCALE_FIT ?
                MIA_DISPLAY_SCALE_STRETCH : (MiaDisplayScaleMode)(*scale_mode - 1);
            if (right) *scale_mode = *scale_mode == MIA_DISPLAY_SCALE_STRETCH ?
                MIA_DISPLAY_SCALE_FIT : (MiaDisplayScaleMode)(*scale_mode + 1);
            display_host_scale_mode_set((uint8_t)*scale_mode);
        }
        const bool redraw = next != selected || left || right;
        if (next != selected) {
            selected = next;
        }
        if (redraw) {
            draw_emulator_menu(target_name, screenshot, selected, notice, *state_slot,
                               brightness, volume, *scale_mode);
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
            wait_menu_input_release();
            return MIA_EMULATOR_MENU_RESUME;
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
            if (selected >= 3u && selected <= 5u) continue;
            wait_menu_input_release();
            if (selected == 0u) return MIA_EMULATOR_MENU_RESUME;
            if (selected == 1u) return MIA_EMULATOR_MENU_SAVE_STATE;
            if (selected == 2u) return MIA_EMULATOR_MENU_LOAD_STATE;
            if (selected == 6u) return MIA_EMULATOR_MENU_ROM_PICKER;
            return MIA_EMULATOR_MENU_EXIT_LAUNCHER;
        }
        mia_host_delay_ms(20);
    }
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
    copy_display_name(title, title_size, full_title, 38);
}

static void draw_picker(const MiaStorageTarget *target, const char *rom_root, const MiaStoragePickerResult *result, size_t selected, size_t rom_count, const char *message) {
    const PickerText *text = picker_text();
    mia_host_clear(MIA_HOST_BLACK);
    mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
    char title[192];
    format_picker_title(target, rom_root, title, sizeof(title));
    const int32_t title_y = mia_host_text_y_centered(0, 20);
    mia_host_draw_text(4, title_y, title, MIA_HOST_BLACK, MIA_HOST_YELLOW);
    char count_text[24];
    snprintf(count_text, sizeof(count_text), "ROM:%u", (unsigned)rom_count);
    const int32_t count_x = mia_host_screen_width() - (int32_t)strlen(count_text) * 8 - 4;
    mia_host_fill_rect(count_x - 4, 0, mia_host_screen_width() - count_x + 4, 20,
                       MIA_HOST_YELLOW);
    mia_host_draw_text(count_x, title_y, count_text, MIA_HOST_BLACK, MIA_HOST_YELLOW);
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
        mia_host_fill_rect(4, y - 4,
#ifdef MIA_PICKER_COVERS
                           190,
#else
                           312,
#endif
                           16, active ? MIA_HOST_BLUE : MIA_HOST_BLACK);
        mia_host_draw_text(8, y - 4, display_name, active ? MIA_HOST_YELLOW : MIA_HOST_WHITE, active ? MIA_HOST_BLUE : MIA_HOST_BLACK);
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
}

static void draw_scan_status(const MiaStorageTarget *target, const char *rom_root) {
    const PickerText *text = picker_text();
    char title[192];
    format_picker_title(target, rom_root, title, sizeof(title));
    mia_host_clear(MIA_HOST_BLACK);
    mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
    mia_host_draw_text(4, mia_host_text_y_centered(0, 20), title, MIA_HOST_BLACK,
                       MIA_HOST_YELLOW);
    mia_host_draw_text(72, 108, text->scanning, MIA_HOST_CYAN, MIA_HOST_BLACK);
    mia_host_present();
}

static void draw_loading_status(unsigned phase) {
    static const int8_t offsets[8][2] = {
        {0, -18}, {13, -13}, {18, 0}, {13, 13},
        {0, 18}, {-13, 13}, {-18, 0}, {-13, -13},
    };
    const PickerText *text = picker_text();
    const int32_t center_x = mia_host_screen_width() / 2;
    mia_host_fill_rect(center_x - 96, 76, 192, 88, MIA_HOST_GRAY);
    mia_host_fill_rect(center_x - 94, 78, 188, 84, MIA_HOST_BLACK);
    for (unsigned index = 0; index < 8u; ++index) {
        const uint8_t color = index == phase % 8u ? MIA_HOST_CYAN : MIA_HOST_DARK_BLUE;
        mia_host_fill_rect(center_x + offsets[index][0] - 3,
                           109 + offsets[index][1] - 3, 6, 6, color);
    }
    const int32_t loading_x = center_x - (int32_t)strlen(text->loading) * 4;
    mia_host_draw_text(loading_x, 140, text->loading, MIA_HOST_WHITE, MIA_HOST_BLACK);
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

static size_t move_roms(const MiaStoragePickerResult *result, size_t start, int direction,
                        size_t count) {
    size_t selected = start;
    for (size_t step = 0; step < count; ++step) {
        const size_t next = next_rom(result, selected, direction);
        if (next == selected) break;
        selected = next;
    }
    return selected;
}

MiaStorageStatus mia_emulator_picker_run(const MiaStorageContext *context, const MiaStorageTarget *target, MiaAppPickerSelection *selection) {
    MiaStoragePickerResult result;
    MiaStorageStatus status;
    size_t rom_count = 0;
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
            draw_picker(target, rom_root, &(MiaStoragePickerResult){0}, 0, 0, message);
            if (!wait_for_retry()) return mia_storage_error(MIA_STORAGE_ERR_INTERRUPTED, "ROM selection cancelled");
            continue;
        }
        size_t first_rom = result.count;
        rom_count = 0;
        for (size_t index = 0; index < result.count; ++index) {
            if (result.entries[index].kind == MIA_STORAGE_ENTRY_ROM) {
                if (first_rom == result.count) first_rom = index;
                ++rom_count;
            }
        }
        if (first_rom != result.count) break;
        draw_picker(target, rom_root, &result, 0, 0, picker_text()->no_roms);
        mia_storage_picker_free(&result);
        if (!wait_for_retry()) return mia_storage_error(MIA_STORAGE_ERR_INTERRUPTED, "ROM selection cancelled");
    }
    size_t selected = 0;
    while (selected < result.count && result.entries[selected].kind != MIA_STORAGE_ENTRY_ROM) ++selected;
    draw_picker(target, rom_root, &result, selected, rom_count, NULL);
#ifdef MIA_PICKER_COVERS
    bool cover_pending = true;
    uint32_t last_navigation_ms = mia_host_millis();
#endif
    for (;;) {
        mia_host_buttons_poll();
        const bool up = mia_host_button_pressed(MIA_HOST_BUTTON_UP);
        const bool down = mia_host_button_pressed(MIA_HOST_BUTTON_DOWN);
        const bool left = mia_host_button_pressed(MIA_HOST_BUTTON_LEFT);
        const bool right = mia_host_button_pressed(MIA_HOST_BUTTON_RIGHT);
        const bool navigation = up || down || left || right;
        const size_t previous = selected;
        if (up) selected = next_rom(&result, selected, -1);
        if (down) selected = next_rom(&result, selected, 1);
        if (left) selected = move_roms(&result, selected, -1, MIA_PICKER_VISIBLE);
        if (right) selected = move_roms(&result, selected, 1, MIA_PICKER_VISIBLE);
        if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
            draw_loading_status(0);
            break;
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
            mia_storage_picker_free(&result);
            return mia_storage_error(MIA_STORAGE_ERR_INTERRUPTED, "ROM selection cancelled");
        }
        if (selected != previous) draw_picker(target, rom_root, &result, selected, rom_count, NULL);
#ifdef MIA_PICKER_COVERS
        if (navigation) {
            last_navigation_ms = mia_host_millis();
            cover_pending = true;
        } else if (cover_pending && mia_host_millis() - last_navigation_ms >= MIA_PICKER_COVER_DELAY_MS) {
            draw_cover(&result.entries[selected]);
            cover_pending = false;
        }
#endif
        mia_host_delay_ms(20);
    }
    status = mia_app_picker_select_entry(&result, selected, selection);
    mia_storage_picker_free(&result);
    return status;
}
