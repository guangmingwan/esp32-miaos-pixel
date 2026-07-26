#include "mia_host_abi.h"
#include "display_host.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_image.h"
#include <webp/decode.h>

#define SCREEN_W 320
#define SCREEN_H 240
#define VIEW_Y 22
#define VIEW_H 188
#define FOOTER_Y 210
#define ROWS_PER_CHUNK 8
#define MAX_IMAGES 96
#define PAN_STEP 24

typedef struct {
    char name[MIA_HOST_DIRENT_NAME_SIZE];
} ImageEntry;

typedef struct {
    const char *title;
    const char *no_images;
    const char *decode_failed;
    const char *controls;
    const char *slideshow_on;
    const char *slideshow_off;
} ViewerText;

static const ViewerText TEXT_EN = {
    "Image Viewer", "No images", "Cannot decode image",
    "L/R:Page A/B:Zoom D:Pan X/Y:Rotate ST:Slide SEL:Info",
    "Slide:On", "Slide:Off",
};
static const ViewerText TEXT_ZH = {
    "图片查看", "没有图片", "无法解码图片",
    "L/R:翻页 A/B:缩放 方向:移动 X/Y:旋转 ST:幻灯 SEL:状态",
    "幻灯片:开", "幻灯片:关",
};

static ImageEntry images[MAX_IMAGES];
static uint32_t image_count;
static uint32_t selected_image;
static char current_path[256] = "/Pictures";
static uint8_t language;
static uint8_t zoom_mode;
static uint16_t rotation;
static uint8_t slideshow;
static uint8_t overlay_visible;
static uint32_t slideshow_at;
static int32_t pan_x;
static int32_t pan_y;
static uint8_t *current_pixels;
static int current_width;
static int current_height;
static uint8_t current_is_webp;

static const ViewerText *text(void) {
    return language == 1 ? &TEXT_ZH : &TEXT_EN;
}

static uint8_t has_image_extension(const char *name) {
    const char *dot = strrchr(name, '.');
    if (dot == NULL) return 0;
    return strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0 ||
           strcasecmp(dot, ".png") == 0 || strcasecmp(dot, ".bmp") == 0 ||
           strcasecmp(dot, ".webp") == 0;
}

static void make_path(char *dest, size_t size, const char *name) {
    if (strcmp(current_path, "/") == 0) {
        snprintf(dest, size, "/sd/%s", name);
    } else {
        snprintf(dest, size, "/sd%s/%s", current_path, name);
    }
}

static void normalize_direct_path(const char *input, char *dest, size_t size) {
    if (strncmp(input, "/sd/", 4) == 0) {
        snprintf(dest, size, "%s", input);
    } else if (input[0] == '/') {
        snprintf(dest, size, "/sd%s", input);
    } else {
        snprintf(dest, size, "/sd/%s", input);
    }
}

static void set_directory_from_path(const char *path) {
    const char *slash = strrchr(path, '/');
    if (slash == NULL || slash == path + 3) {
        snprintf(current_path, sizeof(current_path), "/");
        return;
    }
    size_t length = (size_t)(slash - path);
    if (length < 4) {
        snprintf(current_path, sizeof(current_path), "/");
        return;
    }
    if (length - 3 >= sizeof(current_path)) length = sizeof(current_path) + 3 - 1;
    memcpy(current_path, path + 3, length - 3);
    current_path[length - 3] = '\0';
}

static void scan_images(void) {
    MiaHostDirEntry entries[MAX_IMAGES];
    int32_t count = mia_host_sd_list_dir(current_path, entries, MAX_IMAGES);
    image_count = 0;
    if (count < 0) return;
    for (int32_t i = 0; i < count && image_count < MAX_IMAGES; ++i) {
        if (!entries[i].is_dir && has_image_extension(entries[i].name)) {
            snprintf(images[image_count].name, sizeof(images[image_count].name), "%s",
                     entries[i].name);
            ++image_count;
        }
    }
    if (selected_image >= image_count) selected_image = image_count == 0 ? 0 : image_count - 1;
}

static uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue) {
    return (uint16_t)(((uint16_t)(red & 0xF8) << 8) |
                      ((uint16_t)(green & 0xFC) << 3) | (blue >> 3));
}

static uint32_t scale_for_image(int width, int height, int rotated_width, int rotated_height) {
    if (zoom_mode == 1) return 65536;
    if (zoom_mode == 2) return 98304;
    if (zoom_mode == 3) return 131072;
    uint32_t horizontal = ((uint32_t)SCREEN_W << 16) / (uint32_t)rotated_width;
    int viewport_height = overlay_visible ? VIEW_H : SCREEN_H;
    uint32_t vertical = ((uint32_t)viewport_height << 16) / (uint32_t)rotated_height;
    (void)width;
    (void)height;
    return horizontal < vertical ? horizontal : vertical;
}

static void clamp_pan(int target_width, int target_height) {
    int viewport_height = overlay_visible ? VIEW_H : SCREEN_H;
    int max_x = target_width > SCREEN_W ? (target_width - SCREEN_W) / 2 : 0;
    int max_y = target_height > viewport_height ? (target_height - viewport_height) / 2 : 0;
    if (pan_x < -max_x) pan_x = -max_x;
    if (pan_x > max_x) pan_x = max_x;
    if (pan_y < -max_y) pan_y = -max_y;
    if (pan_y > max_y) pan_y = max_y;
}

static void source_pixel(const uint8_t *pixels, int width, int height, int rx, int ry,
                         uint16_t angle, uint8_t *red, uint8_t *green, uint8_t *blue) {
    int x = rx;
    int y = ry;
    if (angle == 90) {
        x = ry;
        y = height - 1 - rx;
    } else if (angle == 180) {
        x = width - 1 - rx;
        y = height - 1 - ry;
    } else if (angle == 270) {
        x = width - 1 - ry;
        y = rx;
    }
    const uint8_t *pixel = pixels + ((size_t)y * (size_t)width + (size_t)x) * 3u;
    *red = pixel[0];
    *green = pixel[1];
    *blue = pixel[2];
}

static void render_pixels(const uint8_t *pixels, int width, int height) {
    const int viewport_y = overlay_visible ? VIEW_Y : 0;
    const int viewport_height = overlay_visible ? VIEW_H : SCREEN_H;
    const int rotated_width = rotation == 90 || rotation == 270 ? height : width;
    const int rotated_height = rotation == 90 || rotation == 270 ? width : height;
    const uint32_t scale = scale_for_image(width, height, rotated_width, rotated_height);
    int target_width = (int)(((uint64_t)rotated_width * scale + 65535u) >> 16);
    int target_height = (int)(((uint64_t)rotated_height * scale + 65535u) >> 16);
    if (target_width < 1) target_width = 1;
    if (target_height < 1) target_height = 1;
    clamp_pan(target_width, target_height);
    const int left = (SCREEN_W - target_width) / 2 - pan_x;
    const int top = viewport_y + (viewport_height - target_height) / 2 - pan_y;
    uint16_t rows[ROWS_PER_CHUNK * SCREEN_W];

    for (int y_start = viewport_y; y_start < viewport_y + viewport_height; y_start += ROWS_PER_CHUNK) {
        int rows_count = viewport_y + viewport_height - y_start;
        if (rows_count > ROWS_PER_CHUNK) rows_count = ROWS_PER_CHUNK;
        memset(rows, 0, sizeof(rows));
        for (int row = 0; row < rows_count; ++row) {
            int screen_y = y_start + row;
            if (screen_y < top || screen_y >= top + target_height) continue;
            int ry = (int)(((int64_t)(screen_y - top) << 16) / scale);
            if (ry < 0 || ry >= rotated_height) continue;
            for (int screen_x = 0; screen_x < SCREEN_W; ++screen_x) {
                if (screen_x < left || screen_x >= left + target_width) continue;
                int rx = (int)(((int64_t)(screen_x - left) << 16) / scale);
                if (rx < 0 || rx >= rotated_width) continue;
                uint8_t red, green, blue;
                source_pixel(pixels, width, height, rx, ry, rotation, &red, &green, &blue);
                rows[row * SCREEN_W + screen_x] = rgb565(red, green, blue);
            }
        }
        display_host_present_rgb565_region(rows, 0, y_start, SCREEN_W, rows_count,
                                           SCREEN_W * sizeof(uint16_t));
    }
}

static void draw_overlay(const char *name, const char *status) {
    char header[64];
    char state[80];
    const ViewerText *labels = text();
    snprintf(header, sizeof(header), "%s  %u/%u", name, selected_image + 1, image_count);
    snprintf(state, sizeof(state), "%s  %s  Zoom:%s", status,
             rotation == 0 ? "0" : rotation == 90 ? "90" : rotation == 180 ? "180" : "270",
             zoom_mode == 0 ? "Fit" : zoom_mode == 1 ? "100%" : zoom_mode == 2 ? "150%" : "200%");
    mia_host_clear(255);
    mia_host_fill_rect(0, 0, SCREEN_W, VIEW_Y, MIA_HOST_BLUE);
    mia_host_draw_text(4, 7, header, MIA_HOST_YELLOW, MIA_HOST_BLUE);
    mia_host_fill_rect(0, FOOTER_Y, SCREEN_W, SCREEN_H - FOOTER_Y, MIA_HOST_BLACK);
    mia_host_draw_text(4, FOOTER_Y + 1, labels->controls, MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_draw_text(4, FOOTER_Y + 16, state, MIA_HOST_GRAY, MIA_HOST_BLACK);
    display_host_present_rgb565_overlay(NULL, SCREEN_W, SCREEN_H, SCREEN_W * sizeof(uint16_t),
                                        255, 255);
}

static void clear_overlay(void) {
    mia_host_clear(255);
    display_host_present_rgb565_overlay(NULL, SCREEN_W, SCREEN_H, SCREEN_W * sizeof(uint16_t),
                                        255, 255);
}

static uint8_t *load_image(const char *path, int *width, int *height, int *channels,
                           uint8_t *is_webp) {
    const char *dot = strrchr(path, '.');
    *is_webp = dot != NULL && strcasecmp(dot, ".webp") == 0;
    FILE *file = fopen(path, "rb");
    if (file == NULL) return NULL;
    uint8_t header[12];
    size_t header_size = fread(header, 1, sizeof(header), file);
    if (header_size == sizeof(header) && memcmp(header, "RIFF", 4) == 0 &&
        memcmp(header + 8, "WEBP", 4) == 0) {
        *is_webp = 1;
    }
    if (!*is_webp) {
        fclose(file);
        return stbi_load(path, width, height, channels, 3);
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        if (file != NULL) fclose(file);
        return NULL;
    }
    long length = ftell(file);
    if (length <= 0 || length > 8 * 1024 * 1024 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    uint8_t *compressed = malloc((size_t)length);
    if (compressed == NULL || fread(compressed, 1, (size_t)length, file) != (size_t)length) {
        free(compressed);
        fclose(file);
        return NULL;
    }
    fclose(file);
    uint8_t *decoded = WebPDecodeRGB(compressed, (size_t)length, width, height);
    free(compressed);
    if (decoded != NULL) *channels = 3;
    return decoded;
}

static void free_image(uint8_t *pixels, uint8_t is_webp) {
    if (pixels == NULL) return;
    if (is_webp) {
        WebPFree(pixels);
    } else {
        stbi_image_free(pixels);
    }
}

static void unload_current(void) {
    free_image(current_pixels, current_is_webp);
    current_pixels = NULL;
    current_width = 0;
    current_height = 0;
    current_is_webp = 0;
}

static void show_current(void) {
    if (image_count == 0) {
        display_host_fill_screen_rgb565(0);
        if (overlay_visible) draw_overlay(text()->no_images, text()->no_images);
        else clear_overlay();
        return;
    }
    if (current_pixels == NULL) {
        char path[320];
        int channels = 0;
        make_path(path, sizeof(path), images[selected_image].name);
        current_pixels = load_image(path, &current_width, &current_height, &channels,
                                    &current_is_webp);
    }
    display_host_fill_screen_rgb565(0);
    if (current_pixels == NULL || current_width <= 0 || current_height <= 0 ||
        current_width > 4096 || current_height > 4096) {
        unload_current();
        if (overlay_visible) draw_overlay(images[selected_image].name, text()->decode_failed);
        else clear_overlay();
        return;
    }
    render_pixels(current_pixels, current_width, current_height);
    if (overlay_visible) {
        draw_overlay(images[selected_image].name,
                     slideshow ? text()->slideshow_on : text()->slideshow_off);
    } else {
        clear_overlay();
    }
}

static void move_image(int delta) {
    if (image_count == 0) return;
    int next = (int)selected_image + delta;
    if (next < 0) next = (int)image_count - 1;
    if (next >= (int)image_count) next = 0;
    selected_image = (uint32_t)next;
    pan_x = 0;
    pan_y = 0;
    unload_current();
    show_current();
}

static void pan_image(int dx, int dy) {
    if (current_pixels == NULL || zoom_mode == 0) return;
    pan_x += dx;
    pan_y += dy;
    show_current();
}

int image_viewer_main_impl(int argc, char *argv[]) {
    language = mia_host_language();
    display_host_font_set(mia_host_font_get());
    selected_image = 0;
    zoom_mode = 0;
    rotation = 0;
    slideshow = 0;
    overlay_visible = 1;
    pan_x = 0;
    pan_y = 0;
    current_pixels = NULL;
    current_width = 0;
    current_height = 0;
    current_is_webp = 0;
    if (argc > 1 && argv[1] != NULL && argv[1][0] != '\0') {
        char direct_path[320];
        normalize_direct_path(argv[1], direct_path, sizeof(direct_path));
        set_directory_from_path(direct_path);
    }
    scan_images();
    if (argc > 1 && argv[1] != NULL && image_count > 0) {
        const char *base = strrchr(argv[1], '/');
        base = base == NULL ? argv[1] : base + 1;
        for (uint32_t i = 0; i < image_count; ++i) {
            if (strcmp(images[i].name, base) == 0) {
                selected_image = i;
                break;
            }
        }
    }
    show_current();
    while (1) {
        mia_host_buttons_poll();
        if (mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
            mia_host_button_down(MIA_HOST_BUTTON_START)) break;
        if (mia_host_button_pressed(MIA_HOST_BUTTON_L)) move_image(-1);
        if (mia_host_button_pressed(MIA_HOST_BUTTON_R)) move_image(1);
        if (mia_host_button_pressed(MIA_HOST_BUTTON_A) && zoom_mode < 3) {
            ++zoom_mode;
            show_current();
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_B) && zoom_mode > 0) {
            --zoom_mode;
            if (zoom_mode == 0) {
                pan_x = 0;
                pan_y = 0;
            }
            show_current();
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_LEFT)) pan_image(-PAN_STEP, 0);
        if (mia_host_button_pressed(MIA_HOST_BUTTON_RIGHT)) pan_image(PAN_STEP, 0);
        if (mia_host_button_pressed(MIA_HOST_BUTTON_UP)) pan_image(0, -PAN_STEP);
        if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN)) pan_image(0, PAN_STEP);
        if (mia_host_button_pressed(MIA_HOST_BUTTON_X)) {
            rotation = rotation == 0 ? 270 : rotation - 90;
            pan_x = 0;
            pan_y = 0;
            show_current();
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_Y)) {
            rotation = rotation == 270 ? 0 : rotation + 90;
            pan_x = 0;
            pan_y = 0;
            show_current();
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_START)) {
            slideshow = slideshow ? 0 : 1;
            slideshow_at = mia_host_millis();
            show_current();
        }
        if (mia_host_button_pressed(MIA_HOST_BUTTON_SELECT)) {
            overlay_visible = overlay_visible ? 0 : 1;
            show_current();
        }
        if (slideshow && image_count > 0 && mia_host_millis() - slideshow_at >= 3000) {
            slideshow_at = mia_host_millis();
            move_image(1);
        }
        mia_host_delay_ms(20);
    }
    unload_current();
    display_host_fill_screen_rgb565(0);
    return 0;
}
