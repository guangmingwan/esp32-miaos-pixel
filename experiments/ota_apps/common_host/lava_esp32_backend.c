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

static int64_t s_last_button_scan_us;

/* ==================== Display bridge ==================== */

static int32_t present_indexed_frame_sync(const uint8_t *pixels, uint32_t width,
                                          uint32_t height, uint32_t pitch_bytes,
                                          const uint8_t *palette_rgba)
{
    uint16_t palette_rgb565[256];
    int out_w = display_host_width();
    int out_h = display_host_height();
    if (!pixels || !palette_rgba || width == 0 || height == 0 || pitch_bytes < width ||
        out_w <= 0 || out_h <= 0)
        return MIA_HOST_RESULT_INVALID_ARGUMENT;

    int copy_w = (int)width < out_w ? (int)width : out_w;
    int copy_h = (int)height < out_h ? (int)height : out_h;
    int offset_x = (out_w - copy_w) / 2;
    int offset_y = (out_h - copy_h) / 2;

    for (int i = 0; i < 256; i++)
    {
        const uint8_t *color = palette_rgba + i * 4;
        uint8_t r = color[0];
        uint8_t g = color[1];
        uint8_t b = color[2];
        palette_rgb565[i] =
            (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    }

    return display_host_present_indexed8_region(
        pixels, palette_rgb565, offset_x, offset_y, (uint32_t)copy_w,
        (uint32_t)copy_h, pitch_bytes);
}

#ifdef LAVA_DISPLAY_DUAL_CORE

#define LAVA_DISPLAY_FRAME_SLOTS 2
#define LAVA_DISPLAY_SLOT_FREE 0
#define LAVA_DISPLAY_SLOT_FILLING 1
#define LAVA_DISPLAY_SLOT_PENDING 2
#define LAVA_DISPLAY_SLOT_ACTIVE 3

typedef struct
{
    uint8_t *pixels;
    uint8_t palette_rgba[256 * 4];
    uint32_t width;
    uint32_t height;
    uint8_t state;
} LavaDisplayFrameSlot;

static LavaDisplayFrameSlot s_display_slots[LAVA_DISPLAY_FRAME_SLOTS];
static portMUX_TYPE s_display_mux = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t s_display_task;
static size_t s_display_slot_capacity;
static uint8_t s_display_async_ready;
static uint8_t s_display_async_failed;

static void lava_display_task(void *argument)
{
    (void)argument;
    printf("[LAVA][DISPLAY] worker core=%d\n", xPortGetCoreID());
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        for (;;)
        {
            int slot_index = -1;
            portENTER_CRITICAL(&s_display_mux);
            for (int i = 0; i < LAVA_DISPLAY_FRAME_SLOTS; i++)
            {
                if (s_display_slots[i].state == LAVA_DISPLAY_SLOT_PENDING)
                {
                    s_display_slots[i].state = LAVA_DISPLAY_SLOT_ACTIVE;
                    slot_index = i;
                    break;
                }
            }
            portEXIT_CRITICAL(&s_display_mux);
            if (slot_index < 0)
                break;

            LavaDisplayFrameSlot *slot = &s_display_slots[slot_index];
            (void)present_indexed_frame_sync(
                slot->pixels, slot->width, slot->height, slot->width,
                slot->palette_rgba);

            portENTER_CRITICAL(&s_display_mux);
            slot->state = LAVA_DISPLAY_SLOT_FREE;
            portEXIT_CRITICAL(&s_display_mux);
        }
    }
}

static int lava_display_async_init(size_t frame_bytes)
{
    if (s_display_async_ready)
        return frame_bytes <= s_display_slot_capacity;
    if (s_display_async_failed)
        return 0;

    for (int i = 0; i < LAVA_DISPLAY_FRAME_SLOTS; i++)
    {
        s_display_slots[i].pixels = heap_caps_malloc(
            frame_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (s_display_slots[i].pixels == NULL)
            s_display_slots[i].pixels = heap_caps_malloc(frame_bytes, MALLOC_CAP_8BIT);
        if (s_display_slots[i].pixels == NULL)
        {
            for (int j = 0; j < i; j++)
            {
                heap_caps_free(s_display_slots[j].pixels);
                s_display_slots[j].pixels = NULL;
            }
            s_display_async_failed = 1;
            return 0;
        }
        s_display_slots[i].state = LAVA_DISPLAY_SLOT_FREE;
    }

    s_display_slot_capacity = frame_bytes;
    if (xTaskCreatePinnedToCore(lava_display_task, "lava_display", 4096, NULL, 4,
                                &s_display_task, 0) != pdPASS)
    {
        for (int i = 0; i < LAVA_DISPLAY_FRAME_SLOTS; i++)
        {
            heap_caps_free(s_display_slots[i].pixels);
            s_display_slots[i].pixels = NULL;
        }
        s_display_async_failed = 1;
        return 0;
    }

    s_display_async_ready = 1;
    printf("[LAVA][DISPLAY] core0 async slots=%d frame_bytes=%u\n",
           LAVA_DISPLAY_FRAME_SLOTS, (unsigned int)frame_bytes);
    return 1;
}

#endif

/* Runtime palette pixel: R | (G<<8) | (B<<16) | 0xFF000000. */
int32_t lrt_present_indexed_frame(const uint8_t *pixels, uint32_t width,
                                  uint32_t height, uint32_t pitch_bytes,
                                  const uint8_t *palette_rgba)
{
    if (!pixels || !palette_rgba || width == 0 || height == 0 ||
        pitch_bytes < width || height > SIZE_MAX / width)
        return MIA_HOST_RESULT_INVALID_ARGUMENT;

#ifdef LAVA_DISPLAY_DUAL_CORE
    const size_t frame_bytes = (size_t)width * height;
    if (s_display_async_ready && frame_bytes > s_display_slot_capacity)
        return MIA_HOST_RESULT_INVALID_ARGUMENT;
    if (lava_display_async_init(frame_bytes))
    {
        int slot_index = -1;
        portENTER_CRITICAL(&s_display_mux);
        for (int i = 0; i < LAVA_DISPLAY_FRAME_SLOTS; i++)
        {
            if (s_display_slots[i].state == LAVA_DISPLAY_SLOT_PENDING)
            {
                slot_index = i;
                break;
            }
        }
        if (slot_index < 0)
        {
            for (int i = 0; i < LAVA_DISPLAY_FRAME_SLOTS; i++)
            {
                if (s_display_slots[i].state == LAVA_DISPLAY_SLOT_FREE)
                {
                    slot_index = i;
                    break;
                }
            }
        }
        if (slot_index >= 0)
            s_display_slots[slot_index].state = LAVA_DISPLAY_SLOT_FILLING;
        portEXIT_CRITICAL(&s_display_mux);

        if (slot_index < 0)
            return MIA_HOST_RESULT_OK;

        LavaDisplayFrameSlot *slot = &s_display_slots[slot_index];
        for (uint32_t row = 0; row < height; row++)
            memcpy(slot->pixels + (size_t)row * width,
                   pixels + (size_t)row * pitch_bytes, width);
        memcpy(slot->palette_rgba, palette_rgba, sizeof(slot->palette_rgba));
        slot->width = width;
        slot->height = height;

        portENTER_CRITICAL(&s_display_mux);
        slot->state = LAVA_DISPLAY_SLOT_PENDING;
        portEXIT_CRITICAL(&s_display_mux);
        xTaskNotifyGive(s_display_task);
        return MIA_HOST_RESULT_OK;
    }
#endif

    return present_indexed_frame_sync(pixels, width, height, pitch_bytes, palette_rgba);
}

void lrt_refresh(void)
{
    LavaRuntime *rt = lrt_get_global();
    if (!rt || !rt->index_buf || rt->screen_width <= 0 || rt->screen_height <= 0)
        return;

    (void)lrt_present_indexed_frame(
        rt->index_buf, (uint32_t)rt->screen_width, (uint32_t)rt->screen_height,
        (uint32_t)rt->screen_width, (const uint8_t *)rt->palette);
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
    int64_t now = esp_timer_get_time();
    if (s_last_button_scan_us == 0 || now - s_last_button_scan_us >= 1000)
    {
        mia_host_buttons_poll();
        s_last_button_scan_us = now;
    }

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
        push_lava_key(LAVA_KEY_B);
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
