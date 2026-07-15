/*
 * lava_esp32_backend.c - ESP32 platform backend for the LavaX runtime.
 *
 * Provides strong definitions for the weak hooks declared in lava_rt.c:
 *   - lrt_refresh()      : convert VDC BGRA frame -> RGB565 -> display_host
 *   - lrt_poll_keys()    : sample ESP32 buttons -> lav_key / key buffer
 *   - lrt_begin_draw/end : no-op (single-threaded, no lock needed)
 *
 * Also defines the runtime globals shared by ESP32 Lava applications.
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"

#include "lava/lava_rt.h"
#include "display_host.h"
#include "mia_host_abi.h"

/* ==================== Globals ==================== */

volatile unsigned char lav_key = 0;
char ExePath[260] = "/";
extern int g_lava_shutdown_requested;

static uint16_t *s_frame_buf;
static size_t s_frame_pixels;

/* ==================== Display bridge ==================== */

/* VDC pixel: RGB(r,g,b) = R | (G<<8) | (B<<16) | 0xFF000000  (see graph/vdc.h) */
void lrt_refresh(void)
{
    LavaRuntime *rt = lrt_get_global();
    if (!rt) return;

    VDC *vdc = lrt_get_screen(rt);
    if (!vdc || !vdc->mem || rt->screen_width <= 0 || rt->screen_height <= 0)
        return;

    int w = rt->screen_width;
    int h = rt->screen_height;
    int out_w = display_host_width();
    int out_h = display_host_height();
    const uint32_t *src = vdc->mem;

    if (out_w <= 0 || out_h <= 0) return;
    size_t required = (size_t)out_w * (size_t)out_h;
    if (required > s_frame_pixels)
    {
        free(s_frame_buf);
        s_frame_buf = heap_caps_malloc(required * sizeof(uint16_t),
                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (s_frame_buf == NULL)
        {
            s_frame_pixels = 0;
            return;
        }
        s_frame_pixels = required;
    }

    memset(s_frame_buf, 0, required * sizeof(uint16_t));
    int copy_w = w < out_w ? w : out_w;
    int copy_h = h < out_h ? h : out_h;
    int offset_x = (out_w - copy_w) / 2;
    int offset_y = (out_h - copy_h) / 2;

    for (int y = 0; y < copy_h; y++)
    {
        const uint32_t *srow = src + (size_t)y * w;
        uint16_t *drow = s_frame_buf + (size_t)(y + offset_y) * out_w + offset_x;
        for (int x = 0; x < copy_w; x++)
        {
            uint32_t p = srow[x];
            uint8_t r = p & 0xFF;
            uint8_t g = (p >> 8) & 0xFF;
            uint8_t b = (p >> 16) & 0xFF;
            drow[x] = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        }
    }
    display_host_present_rgb565(s_frame_buf, (uint32_t)out_w, (uint32_t)out_h,
                                (uint32_t)out_w * sizeof(uint16_t));
}

/* ==================== Input bridge ==================== */

/* LavaX key codes (from lava_rt_native.h key_code table).
 * Index maps to the logical button set exposed by the ESP32 app context. */
#define LAVA_KEY_UP    0x14
#define LAVA_KEY_DOWN  0x15
#define LAVA_KEY_LEFT  0x17
#define LAVA_KEY_RIGHT 0x16
#define LAVA_KEY_A     0x0d   /* confirm */
#define LAVA_KEY_B     0x1b   /* cancel */
#define LAVA_KEY_X     0x1f   /* last magic */
#define LAVA_KEY_Y     0x19   /* best magic */
#define LAVA_KEY_L     0x0e
#define LAVA_KEY_R     0x1d
#define LAVA_KEY_START 0x1c
#define LAVA_KEY_SELECT 0x13

/* Edge detection state */
static uint8_t s_prev_down[14] = {0};

static void push_lava_key(unsigned char key)
{
    if (lav_key < 128)
        lav_key = key | 0x80;
}

void lrt_poll_keys(void)
{
    mia_host_buttons_poll();

    struct { uint8_t btn; unsigned char key; } map[] = {
        { MIA_HOST_BUTTON_UP,     LAVA_KEY_UP },
        { MIA_HOST_BUTTON_DOWN,   LAVA_KEY_DOWN },
        { MIA_HOST_BUTTON_LEFT,   LAVA_KEY_LEFT },
        { MIA_HOST_BUTTON_RIGHT,  LAVA_KEY_RIGHT },
        { MIA_HOST_BUTTON_A,      LAVA_KEY_A },
        { MIA_HOST_BUTTON_B,      LAVA_KEY_B },
        { MIA_HOST_BUTTON_X,      LAVA_KEY_X },
        { MIA_HOST_BUTTON_Y,      LAVA_KEY_Y },
        { MIA_HOST_BUTTON_L,      LAVA_KEY_L },
        { MIA_HOST_BUTTON_R,      LAVA_KEY_R },
        { MIA_HOST_BUTTON_START,  LAVA_KEY_START },
        { MIA_HOST_BUTTON_SELECT, LAVA_KEY_SELECT },
    };

    for (int i = 0; i < (int)(sizeof(map) / sizeof(map[0])); i++)
    {
        uint8_t down = mia_host_button_down(map[i].btn);
        if (down && !s_prev_down[map[i].btn])
            push_lava_key(map[i].key);
        s_prev_down[map[i].btn] = down;
    }

    /* SELECT + START -> request shutdown (return to launcher) */
    if (mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
        mia_host_button_down(MIA_HOST_BUTTON_START))
    {
        g_lava_shutdown_requested = 1;
    }
}

int lrt_platform_key_down(int key)
{
    switch (key)
    {
    case LAVA_KEY_UP: return mia_host_button_down(MIA_HOST_BUTTON_UP);
    case LAVA_KEY_DOWN: return mia_host_button_down(MIA_HOST_BUTTON_DOWN);
    case LAVA_KEY_LEFT: return mia_host_button_down(MIA_HOST_BUTTON_LEFT);
    case LAVA_KEY_RIGHT: return mia_host_button_down(MIA_HOST_BUTTON_RIGHT);
    case LAVA_KEY_A: return mia_host_button_down(MIA_HOST_BUTTON_A);
    case LAVA_KEY_B: return mia_host_button_down(MIA_HOST_BUTTON_B);
    case LAVA_KEY_X: return mia_host_button_down(MIA_HOST_BUTTON_X);
    case LAVA_KEY_Y: return mia_host_button_down(MIA_HOST_BUTTON_Y);
    default: return 0;
    }
}

/* ==================== Lock hooks (no-op on single-thread ESP32) ==================== */
void lrt_begin_draw(void) {}
void lrt_end_draw(void) {}

/* ==================== ExePath / config helpers ==================== */

void GetExePath(void)
{
    /* On ESP32 the "game directory" is the SD-card app folder.  The caller
     * (app_main) sets ExePath before launching the game. */
    if (ExePath[0] == 0)
        strncpy(ExePath, "/", sizeof(ExePath) - 1);
}

/* native_fopen: delegate to standard fopen (ESP-IDF VFS resolves the path). */
FILE *native_fopen(const char *path, const char *mode)
{
    char full_path[512];

    if (path == NULL || mode == NULL)
        return NULL;
    if (path[0] == '/')
        return fopen(path, mode);
    if (snprintf(full_path, sizeof(full_path), "%s/%s", ExePath, path) >=
        (int)sizeof(full_path))
        return NULL;
    return fopen(full_path, mode);
}
