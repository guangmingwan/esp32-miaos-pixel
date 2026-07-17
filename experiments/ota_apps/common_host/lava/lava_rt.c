/**
 * @file lava_rt.c
 * @brief Lava Runtime 核心实现
 *
 * 实现运行时上下文管理和 API 挂载
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifndef _WIN32
#include <dirent.h>
#include <limits.h>
#endif

/* Include lava_rt.h - now it will use standard ctype */
#include "lava_rt.h"

#ifdef _WIN32
#include <windows.h>
#endif

/*============================================================================
 * 外部字体数据（来自 font.c）
 *============================================================================*/

extern const unsigned char lav_font[];
extern int utf8_to_gb(char* utf8, char* gb);

static const byte lrt_mes_font[][8] = {
    {0x38, 0x4c, 0xc6, 0xc6, 0xc6, 0x64, 0x38, 0x00},
    {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00},
    {0x7c, 0xc6, 0x0e, 0x3c, 0x78, 0xe0, 0xfe, 0x00},
    {0x7e, 0x0c, 0x18, 0x3c, 0x06, 0xc6, 0x7c, 0x00},
    {0x1c, 0x3c, 0x6c, 0xcc, 0xfe, 0x0c, 0x0c, 0x00},
    {0xfc, 0xc0, 0xfc, 0x06, 0x06, 0xc6, 0x7c, 0x00},
    {0x3c, 0x60, 0xc0, 0xfc, 0xc6, 0xc6, 0x7c, 0x00},
    {0xfe, 0xc6, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x00},
    {0x7c, 0xc6, 0xc6, 0x7c, 0xc6, 0xc6, 0x7c, 0x00},
    {0x7c, 0xc6, 0xc6, 0x7e, 0x06, 0x0c, 0x78, 0x00},
    {0x00, 0x30, 0x30, 0x00, 0x30, 0x30, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00},
    {0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00},
    {0x0c, 0x0c, 0x18, 0x10, 0x00, 0x60, 0x60, 0x00},
    {0x3c, 0x42, 0x99, 0xa1, 0xa1, 0x99, 0x42, 0x3c},
    {0x38, 0x6c, 0xc6, 0xc6, 0xfe, 0xc6, 0xc6, 0x00},
    {0xfc, 0xc6, 0xc6, 0xfc, 0xc6, 0xc6, 0xfc, 0x00},
    {0x3c, 0x66, 0xc0, 0xc0, 0xc0, 0x66, 0x3c, 0x00},
    {0xf8, 0xcc, 0xc6, 0xc6, 0xc6, 0xcc, 0xf8, 0x00},
    {0xfe, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xfe, 0x00},
    {0xfe, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0x00},
    {0x3e, 0x60, 0xc0, 0xce, 0xc6, 0x66, 0x3e, 0x00},
    {0xc6, 0xc6, 0xc6, 0xfe, 0xc6, 0xc6, 0xc6, 0x00},
    {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00},
    {0x1e, 0x06, 0x06, 0x06, 0xc6, 0xc6, 0x7c, 0x00},
    {0xc6, 0xcc, 0xd8, 0xf0, 0xf8, 0xdc, 0xce, 0x00},
    {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7e, 0x00},
    {0xc6, 0xee, 0xfe, 0xfe, 0xd6, 0xc6, 0xc6, 0x00},
    {0xc6, 0xe6, 0xf6, 0xfe, 0xde, 0xce, 0xc6, 0x00},
    {0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00},
    {0xfc, 0xc6, 0xc6, 0xc6, 0xfc, 0xc0, 0xc0, 0x00},
    {0x7c, 0xc6, 0xc6, 0xc6, 0xde, 0xcc, 0x7a, 0x00},
    {0xfc, 0xc6, 0xc6, 0xce, 0xf8, 0xdc, 0xce, 0x00},
    {0x78, 0xcc, 0xc0, 0x7c, 0x06, 0xc6, 0x7c, 0x00},
    {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    {0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00},
    {0xc6, 0xc6, 0xc6, 0xee, 0x7c, 0x38, 0x10, 0x00},
    {0xc6, 0xc6, 0xd6, 0xfe, 0xfe, 0xee, 0xc6, 0x00},
    {0xc6, 0xee, 0x7c, 0x38, 0x7c, 0xee, 0xc6, 0x00},
    {0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00},
    {0xfe, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0xfe, 0x00},
    {0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff}
};

/*============================================================================
 * 内部全局变量
 *============================================================================*/

/** 当前运行时上下文（单例模式） */
static LavaRuntime *g_runtime = NULL;
static void lrt_present_all(LavaRuntime *rt);

/** 帮助面板显示状态 */
static int g_keymap_overlay_visible = 0;

/** 屏幕 VDC */
static VDC g_screen_vdc = {0};

/** 全局变量数据区 */
byte gvar_data[0x10000] = {0};

static int has_high_bit_bytes(const char *str)
{
    const unsigned char *p = (const unsigned char*)str;
    while (*p) {
        if (*p & 0x80) {
            return 1;
        }
        p++;
    }
    return 0;
}

static int is_valid_utf8_string(const char *str)
{
    const unsigned char *p = (const unsigned char*)str;

    while (*p) {
        if (*p < 0x80) {
            p++;
            continue;
        }

        if ((*p & 0xE0) == 0xC0) {
            if (p[1] == 0 || (p[1] & 0xC0) != 0x80) {
                return 0;
            }
            if (*p < 0xC2) {
                return 0;
            }
            p += 2;
            continue;
        }

        if ((*p & 0xF0) == 0xE0) {
            if (p[1] == 0 || p[2] == 0) {
                return 0;
            }
            if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) {
                return 0;
            }
            if (*p == 0xE0 && p[1] < 0xA0) {
                return 0;
            }
            if (*p == 0xED && p[1] >= 0xA0) {
                return 0;
            }
            p += 3;
            continue;
        }

        return 0;
    }

    return 1;
}

static int lrt_overlay_font_index(int c)
{
    if (c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    }

    switch (c) {
    case '.':
        c = 0x3b;
        break;
    case ' ':
        c = 0x3c;
        break;
    case '-':
        c = 0x3d;
        break;
    case '/':
        c = 0x3e;
        break;
    case '!':
        c = 0x3f;
        break;
    case '+':
        c = 0x5b;
        break;
    case '_':
        c = 0x5c;
        break;
    default:
        break;
    }

    if (c < '0' || c > 0x5c) {
        return -1;
    }
    c -= '0';
    if (c < 0 || c >= (int)(sizeof(lrt_mes_font) / sizeof(lrt_mes_font[0]))) {
        return -1;
    }
    return c;
}

static u32 lrt_overlay_mix_half(u32 dst, u32 src)
{
    return 0xFF000000u | (((dst & 0x00FEFEFEu) >> 1) + ((src & 0x00FEFEFEu) >> 1));
}

static void lrt_overlay_fill_rect(u32 *pixels, int width, int height, int pitch_px,
                                  int x, int y, int w, int h, u32 color)
{
    int ix;
    int iy;
    int x0 = x;
    int y0 = y;
    int x1 = x + w;
    int y1 = y + h;

    if (!pixels || w <= 0 || h <= 0) {
        return;
    }
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > width) x1 = width;
    if (y1 > height) y1 = height;
    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    for (iy = y0; iy < y1; iy++) {
        u32 *row = pixels + iy * pitch_px;
        for (ix = x0; ix < x1; ix++) {
            row[ix] = color;
        }
    }
}

static void lrt_overlay_fill_rect_blend(u32 *pixels, int width, int height, int pitch_px,
                                        int x, int y, int w, int h, u32 color)
{
    int ix;
    int iy;
    int x0 = x;
    int y0 = y;
    int x1 = x + w;
    int y1 = y + h;

    if (!pixels || w <= 0 || h <= 0) {
        return;
    }
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > width) x1 = width;
    if (y1 > height) y1 = height;
    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    for (iy = y0; iy < y1; iy++) {
        u32 *row = pixels + iy * pitch_px;
        for (ix = x0; ix < x1; ix++) {
            row[ix] = lrt_overlay_mix_half(row[ix], color);
        }
    }
}

static void lrt_overlay_draw_hline(u32 *pixels, int width, int height, int pitch_px,
                                   int x, int y, int w, u32 color)
{
    lrt_overlay_fill_rect(pixels, width, height, pitch_px, x, y, w, 1, color);
}

static void lrt_overlay_draw_vline(u32 *pixels, int width, int height, int pitch_px,
                                   int x, int y, int h, u32 color)
{
    lrt_overlay_fill_rect(pixels, width, height, pitch_px, x, y, 1, h, color);
}

static void lrt_overlay_draw_char(u32 *pixels, int width, int height, int pitch_px,
                                  int x, int y, int c, u32 color)
{
    int i;
    int j;
    int index = lrt_overlay_font_index(c);

    if (index < 0) {
        return;
    }

    for (i = 0; i < 8; i++) {
        byte row_bits = lrt_mes_font[index][i];
        int py = y + i;
        if (py < 0 || py >= height) {
            continue;
        }
        for (j = 0; j < 8; j++) {
            int px = x + j;
            if (px < 0 || px >= width) {
                continue;
            }
            if (row_bits & (0x80 >> j)) {
                pixels[py * pitch_px + px] = color;
            }
        }
    }
}

static void lrt_overlay_draw_string(u32 *pixels, int width, int height, int pitch_px,
                                    int x, int y, const char *text, u32 color)
{
    while (text && *text) {
        lrt_overlay_draw_char(pixels, width, height, pitch_px, x, y, *text, color);
        x += 8;
        text++;
    }
}

static void lrt_overlay_draw_char_scaled(u32 *pixels, int width, int height, int pitch_px,
                                         int x, int y, int c, int scale, u32 color)
{
    int i;
    int j;
    int sy;
    int sx;
    int index = lrt_overlay_font_index(c);

    if (index < 0 || scale <= 0) {
        return;
    }

    for (i = 0; i < 8; i++) {
        byte row_bits = lrt_mes_font[index][i];
        for (j = 0; j < 8; j++) {
            if (row_bits & (0x80 >> j)) {
                int px = x + j * scale;
                int py = y + i * scale;
                for (sy = 0; sy < scale; sy++) {
                    int dy = py + sy;
                    if (dy < 0 || dy >= height) {
                        continue;
                    }
                    for (sx = 0; sx < scale; sx++) {
                        int dx = px + sx;
                        if (dx < 0 || dx >= width) {
                            continue;
                        }
                        pixels[dy * pitch_px + dx] = color;
                    }
                }
            }
        }
    }
}

static void lrt_overlay_draw_string_scaled(u32 *pixels, int width, int height, int pitch_px,
                                           int x, int y, const char *text, int scale, u32 color)
{
    while (text && *text) {
        lrt_overlay_draw_char_scaled(pixels, width, height, pitch_px, x, y, *text, scale, color);
        x += 8 * scale;
        text++;
    }
}

/*============================================================================
 * 初始化与销毁
 *============================================================================*/



LavaRuntime* lrt_create(int width, int height, const byte *font_data)
{
    LavaRuntime *rt = (LavaRuntime*)calloc(1, sizeof(LavaRuntime));
    if (!rt) return NULL;

    /* 屏幕配置 */
    rt->screen_width = width;
    rt->screen_height = height;
    rt->color_depth = 16;  /* 默认 16 位色 */

    /* 分配屏幕 VDC 内存 */
    g_screen_vdc.width = width;
    g_screen_vdc.height = height;
    g_screen_vdc.mem = (u32*)calloc(width * height, sizeof(u32));
    if (!g_screen_vdc.mem) {
        free(rt);
        return NULL;
    }

    rt->index_buf = (byte*)calloc((size_t)width * (size_t)height, sizeof(byte));
    if (!rt->index_buf) {
        free(g_screen_vdc.mem);
        g_screen_vdc.mem = NULL;
        free(rt);
        return NULL;
    }
    rt->owns_index_buf = 1;

    /* 设置裁剪区域 */
    g_screen_vdc.clip.x0 = 0;
    g_screen_vdc.clip.y0 = 0;
    g_screen_vdc.clip.x1 = width - 1;
    g_screen_vdc.clip.y1 = height - 1;

    /* 默认颜色 */
    g_screen_vdc.fgcolor = COLOR_WHITE;
    g_screen_vdc.bgcolor = COLOR_BLACK;

    rt->screen_vdc = &g_screen_vdc;

    /* 字体数据：若为 NULL 则使用内置字体 */
    if (!font_data) {
        font_data = lav_font;  /* 使用 font.c 中的内置字体 */
    }
    rt->font_data = font_data;
    vw_SetFontData(font_data);

    g_runtime = rt;
    g_keymap_overlay_visible = 0;

    /* 初始化图形库 */
    vw_Init();

    lrt_set_screen(0);
    lrt_set_graph_mode(1);

    return rt;
}

LavaRuntime* lrt_get_global(void)
{
    return g_runtime;
}

int lrt_bind_index_buffer(LavaRuntime *rt, byte *pixels, int width, int height)
{
    if (!rt || !pixels || width != rt->screen_width || height != rt->screen_height) {
        return -1;
    }
    if (rt->index_buf == pixels) {
        return 0;
    }
    if (rt->owns_index_buf && rt->index_buf) {
        free(rt->index_buf);
    }
    rt->index_buf = pixels;
    rt->owns_index_buf = 0;
    lrt_present_all(rt);
    return 0;
}

void lrt_destroy(LavaRuntime *rt)
{
    if (!rt) return;

    if (g_screen_vdc.mem) {
        free(g_screen_vdc.mem);
        g_screen_vdc.mem = NULL;
    }

    if (rt->owns_index_buf && rt->index_buf) {
        free(rt->index_buf);
    }
    rt->index_buf = NULL;

    free(rt);
    g_runtime = NULL;
    g_keymap_overlay_visible = 0;
}

/*============================================================================
 * 图形 API
 *============================================================================*/

VDC* lrt_get_screen(LavaRuntime *rt)
{
    if (!rt) return NULL;
    return (VDC*)rt->screen_vdc;
}

static int lrt_mode_mask(const LavaRuntime *rt)
{
    if (!rt) return 0xFF;
    if (rt->graph_mode == 1) return 0x01;
    if (rt->graph_mode == 4) return 0x0F;
    return 0xFF;
}

static int lrt_valid_graph_mode(int mode)
{
    return mode == 1 || mode == 4 || mode == 8;
}

static byte lrt_fg_index(const LavaRuntime *rt)
{
    if (!rt) return 0;
    if (rt->graph_mode == 1) return 1;
    return (byte)(rt->fgcolor & lrt_mode_mask(rt));
}

static byte lrt_bg_index(const LavaRuntime *rt)
{
    if (!rt) return 0;
    if (rt->graph_mode == 1) return 0;
    return (byte)(rt->bgcolor & lrt_mode_mask(rt));
}

static void lrt_update_dc_colors(LavaRuntime *rt)
{
    VDC *dc;

    if (!rt) return;
    dc = (VDC*)rt->screen_vdc;
    if (!dc) return;

    dc->fgcolor = rt->palette[lrt_fg_index(rt)];
    dc->bgcolor = rt->palette[lrt_bg_index(rt)];
    dc->draw_mode = DRAW_COPY;
}

static void lrt_present_pixel(LavaRuntime *rt, int x, int y)
{
    size_t pos;
    VDC *dc;

    if (!rt || !rt->index_buf) return;
    dc = (VDC*)rt->screen_vdc;
    if (!dc || !dc->mem) return;
    if (x < 0 || y < 0 || x >= rt->screen_width || y >= rt->screen_height) return;

    pos = (size_t)y * (size_t)rt->screen_width + (size_t)x;
    dc->mem[pos] = rt->palette[rt->index_buf[pos]];
}

static void lrt_present_all(LavaRuntime *rt)
{
    int x;
    int y;

    if (!rt || !rt->index_buf) return;
    for (y = 0; y < rt->screen_height; y++) {
        for (x = 0; x < rt->screen_width; x++) {
            lrt_present_pixel(rt, x, y);
        }
    }
}

static byte lrt_read_index(const LavaRuntime *rt, int x, int y)
{
    if (!rt || !rt->index_buf) return 0;
    if (x < 0 || y < 0 || x >= rt->screen_width || y >= rt->screen_height) return 0;
    return rt->index_buf[(size_t)y * (size_t)rt->screen_width + (size_t)x];
}

static void lrt_write_index(LavaRuntime *rt, int x, int y, byte value)
{
    size_t pos;

    if (!rt || !rt->index_buf) return;
    if (x < 0 || y < 0 || x >= rt->screen_width || y >= rt->screen_height) return;

    pos = (size_t)y * (size_t)rt->screen_width + (size_t)x;
    rt->index_buf[pos] = (byte)(value & lrt_mode_mask(rt));
    lrt_present_pixel(rt, x, y);
}

static void lrt_fill_index(LavaRuntime *rt, byte value)
{
    size_t count;

    if (!rt || !rt->index_buf) return;
    count = (size_t)rt->screen_width * (size_t)rt->screen_height;
    memset(rt->index_buf, value, count);
    lrt_present_all(rt);
}

static byte lrt_apply_draw_type(LavaRuntime *rt, byte dst, int type)
{
    int mask;

    if (!rt) return 0;
    mask = lrt_mode_mask(rt);

    switch (type) {
    case 0:
        return lrt_bg_index(rt);
    case 2:
        return (byte)((dst ^ mask) & mask);
    default:
        return lrt_fg_index(rt);
    }
}

static byte lrt_apply_lcmd(LavaRuntime *rt, byte dst, byte src, int lcmd)
{
    int op;
    int mask;

    if (!rt) return 0;

    op = lcmd & 7;
    mask = lrt_mode_mask(rt);
    src &= (byte)mask;

    if ((lcmd & 8) || op == 2) {
        src ^= (byte)mask;
    }

    if (rt->graph_mode == 8 && op == 6) {
        return src ? src : dst;
    }

    switch (op) {
    case 3:
        return (byte)((dst | src) & mask);
    case 4:
        return (byte)((dst & src) & mask);
    case 5:
        return (byte)((dst ^ src) & mask);
    default:
        return (byte)(src & mask);
    }
}

static void lrt_put_dot_xy(LavaRuntime *rt, int x, int y, int type)
{
    byte dst;

    if (!rt) return;
    if (x < 0 || y < 0 || x >= rt->screen_width || y >= rt->screen_height) return;

    dst = lrt_read_index(rt, x, y);
    lrt_write_index(rt, x, y, lrt_apply_draw_type(rt, dst, type));
}

static void lrt_hline_vm(LavaRuntime *rt, int x0, int x1, int y, int type)
{
    int x;
    int t;

    if (!rt) return;
    if (y < 0 || y >= rt->screen_height) return;
    if (x0 > x1) {
        t = x0;
        x0 = x1;
        x1 = t;
    }
    if (x1 < 0 || x0 >= rt->screen_width) return;
    if (x0 < 0) x0 = 0;
    if (x1 >= rt->screen_width) x1 = rt->screen_width - 1;

    for (x = x0; x <= x1; x++) {
        lrt_put_dot_xy(rt, x, y, type);
    }
}

static int lrt_block_prepare(LavaRuntime *rt, int *x0, int *y0, int *x1, int *y1)
{
    int t;

    if (!rt || !x0 || !y0 || !x1 || !y1) return 0;

    if (*y0 > *y1) {
        t = *y0;
        *y0 = *y1;
        *y1 = t;
    }
    if (*x0 > *x1) {
        t = *x0;
        *x0 = *x1;
        *x1 = t;
    }

    if (*x1 < 0 || *x0 >= rt->screen_width) return 0;
    if (*y1 < 0 || *y0 >= rt->screen_height) return 0;

    if (*x0 < 0) *x0 = 0;
    if (*y0 < 0) *y0 = 0;
    if (*x1 >= rt->screen_width) *x1 = rt->screen_width - 1;
    if (*y1 >= rt->screen_height) *y1 = rt->screen_height - 1;
    return 1;
}

static void lrt_draw_block_vm(LavaRuntime *rt, int x0, int y0, int x1, int y1, int type)
{
    int y;

    if (!lrt_block_prepare(rt, &x0, &y0, &x1, &y1)) return;
    for (y = y0; y <= y1; y++) {
        lrt_hline_vm(rt, x0, x1, y, type);
    }
}

static void lrt_draw_rect_vm(LavaRuntime *rt, int x0, int y0, int x1, int y1, int type)
{
    int y;

    if (!lrt_block_prepare(rt, &x0, &y0, &x1, &y1)) return;

    lrt_hline_vm(rt, x0, x1, y0, type);
    lrt_hline_vm(rt, x0, x1, y1, type);
    for (y = y0; y <= y1; y++) {
        lrt_put_dot_xy(rt, x0, y, type);
        lrt_put_dot_xy(rt, x1, y, type);
    }
}

static void lrt_put_dot4(LavaRuntime *rt, int x0, int y0, int x, int y, int fill, int type, int *circle_buf)
{
    if (!rt) return;

    if (fill) {
        if (y0 - y >= 0 && y0 - y < rt->screen_height && x > circle_buf[y0 - y]) {
            circle_buf[y0 - y] = x;
        }
        if (y0 + y >= 0 && y0 + y < rt->screen_height && x > circle_buf[y0 + y]) {
            circle_buf[y0 + y] = x;
        }
        return;
    }

    if (x == 0) {
        lrt_put_dot_xy(rt, x0, y0 + y, type);
        lrt_put_dot_xy(rt, x0, y0 - y, type);
    } else if (y == 0) {
        lrt_put_dot_xy(rt, x0 + x, y0, type);
        lrt_put_dot_xy(rt, x0 - x, y0, type);
    } else {
        lrt_put_dot_xy(rt, x0 + x, y0 + y, type);
        lrt_put_dot_xy(rt, x0 - x, y0 + y, type);
        lrt_put_dot_xy(rt, x0 + x, y0 - y, type);
        lrt_put_dot_xy(rt, x0 - x, y0 - y, type);
    }
}

static void lrt_draw_ellipse_vm(LavaRuntime *rt, int x0, int y0, int r1, int r2, int fill, int type)
{
    int *circle_buf = NULL;
    int i;
    int j;
    int fxy;
    int fx;
    int fy;
    int incx;
    int incy;
    int temp_x;
    int temp_y;
    int delta_x;
    int delta_y;
    int distant_a;
    int distant_b;
    int circle_r;
    int dot_start;

    if (!rt || r1 < 0 || r2 < 0) return;

    if (fill) {
        circle_buf = (int*)malloc((size_t)rt->screen_height * sizeof(int));
        if (!circle_buf) return;
        for (i = 0; i < rt->screen_height; i++) {
            circle_buf[i] = -1;
        }
    }

    distant_a = r1;
    distant_b = r2;
    dot_start = 0;

    if (distant_a == 0 && distant_b == 0) {
        lrt_put_dot_xy(rt, x0, y0, type);
        free(circle_buf);
        return;
    }

    circle_r = (distant_a > distant_b) ? distant_a : distant_b;
    incx = -1;
    incy = 1;
    fy = 1;
    fx = 1 - 2 * circle_r;
    fxy = 0;
    delta_x = 0;
    delta_y = 0;
    temp_x = distant_a;
    temp_y = 0;

    lrt_put_dot4(rt, x0, y0, temp_x, temp_y, fill, type, circle_buf);

    do {
        if (fxy >= 0) {
            delta_x += distant_a;
            if (delta_x >= circle_r) {
                temp_x += incx;
                delta_x -= circle_r;
                if (temp_x + 1 != distant_a) {
                    lrt_put_dot4(rt, x0, y0, temp_x, temp_y, fill, type, circle_buf);
                }
            }
            fxy -= abs(fx);
            fx += 2;
            if (fx < 0 || fx >= 3) {
                continue;
            }
            incy = -incy;
            fy = -fy + 2;
            fxy = -fxy;
        } else {
            delta_y += distant_b;
            if (delta_y >= circle_r) {
                delta_y -= circle_r;
                temp_y += incy;
                if ((temp_y == 1 || temp_y == 2) && dot_start == 0) {
                    lrt_put_dot4(rt, x0, y0, distant_a, temp_y, fill, type, circle_buf);
                } else {
                    dot_start = 1;
                    lrt_put_dot4(rt, x0, y0, temp_x, temp_y, fill, type, circle_buf);
                }
            }
            fxy += abs(fy);
            fy += 2;
            if (fy < 0 || fy > 2) {
                continue;
            }
            incx = -incx;
            fx = -fx + 2;
            fxy = -fxy;
        }
    } while (temp_x);

    if (fill && circle_buf) {
        for (i = 0; i < rt->screen_height; i++) {
            if (circle_buf[i] >= 0) {
                for (j = 0; j < circle_buf[i] * 2 + 1; j++) {
                    lrt_put_dot_xy(rt, x0 - circle_buf[i] + j, i, type);
                }
            }
        }
    }

    free(circle_buf);
}

static unsigned short lrt_next_gb_char(const char **ps)
{
    const unsigned char *p;
    unsigned char c;
    unsigned char c2;

    if (!ps || !*ps) return 0;

    p = (const unsigned char*)*ps;
    c = *p++;
    if (c == 0) return 0;

    if (c < 0x80) {
        (*ps)++;
        return c;
    }

    c2 = *p;
    if (c2 == 0) {
        (*ps)++;
        return '?';
    }

    *ps += 2;
    return (unsigned short)((c << 8) | c2);
}

static int lrt_glyph_bit(const byte *buf, int width, int row, int col)
{
    if (!buf || row < 0 || col < 0 || col >= width) return 0;

    if (width <= 8) {
        return (buf[row] & (0x80 >> col)) != 0;
    }

    if (width == 12) {
        if (col < 8) {
            return (buf[row * 2] & (0x80 >> col)) != 0;
        }
        return (buf[row * 2 + 1] & (0x80 >> (col - 8))) != 0;
    }

    if (col < 8) {
        return (buf[row * 2] & (0x80 >> col)) != 0;
    }
    return (buf[row * 2 + 1] & (0x80 >> (col - 8))) != 0;
}

static void lrt_draw_glyph(LavaRuntime *rt, int x, int y, const byte *buf, int width, int height, int mode)
{
    int row;
    int col;
    int lcmd;
    int mirror;
    byte fg;
    byte bg;

    if (!rt || !buf || width <= 0 || height <= 0) return;

    lcmd = mode & 0x0F;
    mirror = mode & 0x20;
    fg = lrt_fg_index(rt);
    bg = lrt_bg_index(rt);

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            int src_col = mirror ? (width - 1 - col) : col;
            byte src;
            byte dst;

            if (rt->graph_mode == 1) {
                src = lrt_glyph_bit(buf, width, row, src_col) ? 1 : 0;
            } else {
                src = lrt_glyph_bit(buf, width, row, src_col) ? fg : bg;
            }

            dst = lrt_read_index(rt, x + col, y + row);
            lrt_write_index(rt, x + col, y + row, lrt_apply_lcmd(rt, dst, src, lcmd));
        }
    }
}

void lrt_set_palette(LavaRuntime *rt, int index, int r, int g, int b)
{
    if (!rt || index < 0 || index >= 256) return;
    rt->palette[index] = RGB(r, g, b);
    lrt_update_dc_colors(rt);
}

void lrt_set_fgcolor(LavaRuntime *rt, int color)
{
    int mask;

    if (!rt) return;
    mask = lrt_mode_mask(rt);
    if (color < 0) color = 0;
    rt->fgcolor = (word)(color & mask);
    lrt_update_dc_colors(rt);
}

void lrt_set_bgcolor(LavaRuntime *rt, int color)
{
    int mask;

    if (!rt) return;
    mask = lrt_mode_mask(rt);
    if (color < 0) color = 0;
    rt->bgcolor = (word)(color & mask);
    lrt_update_dc_colors(rt);
}

/* 全局版本（原生模式使用） */
void lrt_set_fgcolor_global(int color)
{
    lrt_set_fgcolor(g_runtime, color);
}

void lrt_set_bgcolor_global(int color)
{
    lrt_set_bgcolor(g_runtime, color);
}

void lrt_set_palette_block(int start, int count, unsigned char *data)
{
    int i;

    if (!g_runtime || !data || count <= 0) return;
    if (start < 0) start = 0;
    if (start >= 256) return;
    if (start + count > 256) count = 256 - start;

    for (i = 0; i < count; i++) {
        g_runtime->palette[start + i] = RGB(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
    }

    lrt_update_dc_colors(g_runtime);
    lrt_present_all(g_runtime);
}

int lrt_set_palette_vm(int start, int count, const unsigned char *data)
{
    int i;

    if (!g_runtime || !data || count <= 0) return 0;
    if (start < 0) start = 0;
    if (start >= 256) return 0;
    if (start + count > 256) count = 256 - start;

    for (i = 0; i < count; i++) {
        const unsigned char *src = data + i * 4;
        g_runtime->palette[start + i] = RGB(src[2], src[1], src[0]);
    }

    lrt_update_dc_colors(g_runtime);
    lrt_begin_draw();
    lrt_present_all(g_runtime);
    lrt_end_draw();
    return count;
}

/*============================================================================
 * LAVA 兼容 API
 *============================================================================*/

__attribute__((weak)) void lrt_begin_draw(void)
{
}

__attribute__((weak)) void lrt_end_draw(void)
{
}

void lrt_point(int x, int y, int type)
{
    if (!g_runtime) return;
    lrt_begin_draw();
    lrt_put_dot_xy(g_runtime, x, y, type);
    lrt_end_draw();
}

void lrt_line(int x0, int y0, int x1, int y1, int type)
{
    int delta_x;
    int delta_y;
    int distance;
    int tt;
    int xerr;
    int yerr;
    int incy;
    int t;
    int x;
    int y;

    if (!g_runtime) return;

    lrt_begin_draw();

    if (x0 == x1) {
        if (y0 > y1) {
            t = y0;
            y0 = y1;
            y1 = t;
        }
        for (y = y0; y <= y1; y++) {
            lrt_put_dot_xy(g_runtime, x0, y, type);
        }
        lrt_end_draw();
        return;
    }

    if (y0 == y1) {
        lrt_hline_vm(g_runtime, x0, x1, y0, type);
        lrt_end_draw();
        return;
    }

    if (x1 < x0) {
        t = x0;
        x0 = x1;
        x1 = t;
        t = y0;
        y0 = y1;
        y1 = t;
    }

    delta_x = x1 - x0;
    delta_y = abs(y1 - y0);
    incy = (y1 > y0) ? 1 : -1;
    distance = (delta_x > delta_y) ? delta_x : delta_y;
    tt = 0;
    xerr = 0;
    yerr = 0;
    x = x0;
    y = y0;

    for (;;) {
        lrt_put_dot_xy(g_runtime, x, y, type);
        xerr += delta_x;
        yerr += delta_y;
        if (xerr >= distance) {
            xerr -= distance;
            x++;
        }
        if (yerr >= distance) {
            yerr -= distance;
            y += incy;
        }
        tt++;
        if (distance < tt) break;
    }

    lrt_end_draw();
}

void lrt_box(int x0, int y0, int x1, int y1, int fill, int type)
{
    if (!g_runtime) return;
    lrt_begin_draw();
    if (fill) lrt_draw_block_vm(g_runtime, x0, y0, x1, y1, type);
    else lrt_draw_rect_vm(g_runtime, x0, y0, x1, y1, type);
    lrt_end_draw();
}

void lrt_circle(int x, int y, int r, int fill, int type)
{
    if (!g_runtime) return;
    lrt_begin_draw();
    lrt_draw_ellipse_vm(g_runtime, x, y, r, r, fill, type);
    lrt_end_draw();
}

void lrt_ellipse(int x, int y, int a, int b, int fill, int type)
{
    if (!g_runtime) return;
    lrt_begin_draw();
    lrt_draw_ellipse_vm(g_runtime, x, y, a, b, fill, type);
    lrt_end_draw();
}

void lrt_textout(int x, int y, const char *str, int mode)
{
    char *gb_buf = NULL;
    const char *draw_str = str;
    int font = (mode & 0x80) ? FONT_MEDIUM : FONT_SMALL;
    int height = (font == FONT_SMALL) ? 12 : 16;
    int no_buf = mode & 0x40;

    if (!g_runtime || !str) return;

    if (has_high_bit_bytes(str) && is_valid_utf8_string(str)) {
        size_t len = strlen(str);
        gb_buf = (char*)malloc(len + 1);
        if (gb_buf && utf8_to_gb((char*)str, gb_buf)) {
            draw_str = gb_buf;
        } else {
            free(gb_buf);
            gb_buf = NULL;
        }
    }

    lrt_begin_draw();
    while (*draw_str) {
        byte glyph[32];
        unsigned short ch = lrt_next_gb_char(&draw_str);
        int width = lrt_get_font_bitmap_ex(g_runtime, ch, font, glyph, (int)sizeof(glyph));

        if (width <= 0) continue;
        if (x >= g_runtime->screen_width) break;

        lrt_draw_glyph(g_runtime, x, y, glyph, width, height, mode);
        x += width;
    }
    lrt_end_draw();

    free(gb_buf);

    if (no_buf) {
        lrt_refresh();
    }
}

void lrt_clear_screen(void)
{
    byte fill;

    if (!g_runtime) return;
    fill = (g_runtime->graph_mode == 1) ? 0 : lrt_bg_index(g_runtime);
    lrt_begin_draw();
    lrt_fill_index(g_runtime, fill);
    lrt_end_draw();
}

void lrt_set_screen(int mode)
{
    int width;
    int height;

    if (!g_runtime) return;

    g_runtime->screen_mode = (mode != 0);
    width = g_runtime->screen_width;
    height = g_runtime->screen_height;

    if (g_runtime->screen_mode) {
        g_runtime->text_cols = (byte)(((width - 2) / 6) & 0xFE);
        g_runtime->text_rows = (byte)((height - 1) / 13);
    } else {
        g_runtime->text_cols = (byte)(width / 8);
        g_runtime->text_rows = (byte)(height / 16);
    }
    g_runtime->cursor_x = 0;
    g_runtime->cursor_y = 0;
}

void lrt_locate(int y, int x)
{
    if (!g_runtime) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= g_runtime->text_cols) x = g_runtime->text_cols ? g_runtime->text_cols - 1 : 0;
    if (y >= g_runtime->text_rows) y = g_runtime->text_rows ? g_runtime->text_rows - 1 : 0;
    g_runtime->cursor_x = (byte)x;
    g_runtime->cursor_y = (byte)y;
}

u32 lrt_get_pixel(int x, int y)
{
    if (!g_runtime) return 0;
    if (x < 0 || y < 0 || x >= g_runtime->screen_width || y >= g_runtime->screen_height) return 0;
    return g_runtime->palette[lrt_read_index(g_runtime, x, y)];
}

int lrt_get_point(int x, int y)
{
    if (!g_runtime) return 0;
    if (x < 0 || y < 0 || x >= g_runtime->screen_width || y >= g_runtime->screen_height) return 0;
    return lrt_read_index(g_runtime, x, y);
}

/* 默认空实现，使用弱符号允许用户空间覆盖 */
__attribute__((weak)) void lrt_refresh(void)
{
    /* 刷新显示需要 SDL 或平台层实现 */
    /* 这里只是占位符，实际实现由上层提供 */
}

/*============================================================================
 * 字体 API
 *============================================================================*/

int lrt_get_font_bitmap_ex(LavaRuntime *rt, int ch, int font, byte *buf, int buf_size)
{
    const byte *font_data;
    const byte *src;
    int bytes;

    if (!buf || buf_size <= 0) return 0;

    font_data = rt && rt->font_data ? rt->font_data : lav_font;

    if (ch >= 0 && ch < 0x80) {
        if (font == FONT_SMALL) {
            bytes = 12;
            if (buf_size < bytes) return 0;
            src = font_data + ch * 12;
            memcpy(buf, src, bytes);
            return 6;
        }

        bytes = 16;
        if (buf_size < bytes) return 0;
        src = font_data + 0x600 + ch * 16;
        memcpy(buf, src, bytes);
        return 8;
    }

    {
        unsigned char c1 = (unsigned char)(ch >> 8);
        unsigned char c2 = (unsigned char)ch;
        int index;

        if (c1 < 0xa1 || c1 > 0xfe || c2 < 0xa1 || c2 > 0xfe) return 0;

        if (c1 < 0xb0) index = (c1 - 0xa1) * 94 + (c2 - 0xa1);
        else index = (c1 - 0xa7) * 94 + (c2 - 0xa1);

        if (font == FONT_SMALL) {
            bytes = 24;
            if (buf_size < bytes) return 0;
            src = font_data + 0xe00 + index * 24;
            memcpy(buf, src, bytes);
            return 12;
        }

        bytes = 32;
        if (buf_size < bytes) return 0;

        src = font_data + 0x2d7d0 + index * 32;
        memcpy(buf, src, bytes);
        return 16;
    }
}

int lrt_get_font_bitmap(LavaRuntime *rt, int ch, byte *buf, int buf_size)
{
    return lrt_get_font_bitmap_ex(rt, ch, FONT_MEDIUM, buf, buf_size);
}



/*============================================================================
 * 扩展图形 API（新增）
 *============================================================================*/

static byte lrt_block_src_pixel(const LavaRuntime *rt, const byte *data, int w, int x, int y)
{
    int row_stride;

    if (!rt || !data || x < 0 || y < 0 || x >= w) return 0;

    if (rt->graph_mode == 1) {
        row_stride = (w + 7) >> 3;
        return (byte)((data[y * row_stride + (x >> 3)] >> (7 - (x & 7))) & 1);
    }

    if (rt->graph_mode == 4) {
        row_stride = (w + 1) >> 1;
        if (x & 1) {
            return (byte)(data[y * row_stride + (x >> 1)] & 0x0F);
        }
        return (byte)(data[y * row_stride + (x >> 1)] >> 4);
    }

    return data[y * w + x];
}

int lrt_set_graph_mode(int mode)
{
    static const byte lv5[] = {0, 64, 128, 192, 255};
    static const byte lv9[] = {0, 32, 64, 96, 128, 160, 192, 224, 255};
    int old_mode;
    int i;

    if (!g_runtime) return 0;

    old_mode = g_runtime->graph_mode;

    if (mode == 0) {
        return old_mode;
    }
    if (!lrt_valid_graph_mode(mode)) {
        return 0;
    }
    if (mode == old_mode) {
        return old_mode;
    }

    lrt_begin_draw();

    for (i = 0; i < 256; i++) {
        g_runtime->palette[i] = COLOR_BLACK;
    }

    if (mode == 1) {
        g_runtime->palette[0] = RGB(255, 255, 255);
        g_runtime->palette[1] = RGB(0, 0, 0);
        g_runtime->palette[2] = RGB(127, 127, 127);
        g_runtime->palette[254] = COLOR_BLACK;
        g_runtime->palette[255] = COLOR_BLACK;
        g_runtime->fgcolor = 1;
        g_runtime->bgcolor = 0;
    } else if (mode == 4) {
        g_runtime->palette[254] = COLOR_BLACK;
        g_runtime->palette[255] = COLOR_BLACK;
        for (i = 0; i < 16; i++) {
            int c = (15 - i) * 0x11;
            g_runtime->palette[i] = RGB(c, c, c);
        }
        g_runtime->fgcolor = 15;
        g_runtime->bgcolor = 0;
    } else {
        g_runtime->palette[0] = RGB(0, 0, 0);
        g_runtime->palette[254] = RGB(0, 0, 0);
        g_runtime->palette[255] = RGB(255, 255, 255);
        for (i = 0; i < 225; i++) {
            g_runtime->palette[16 + i] = RGB(lv5[i % 5], lv9[i / 25], lv5[(i / 5) % 5]);
        }
        g_runtime->fgcolor = 255;
        g_runtime->bgcolor = 0;
    }

    g_runtime->graph_mode = (word)mode;
    lrt_update_dc_colors(g_runtime);
    lrt_fill_index(g_runtime, (mode == 1) ? 0 : lrt_bg_index(g_runtime));

    lrt_end_draw();
    return old_mode;
}

void lrt_write_block(int x, int y, int w, int h, int mode, const byte *data)
{
    int dx;
    int dy;
    int lcmd;
    int mirror;
    int no_buf;

    if (!g_runtime || !data || w <= 0 || h <= 0) return;

    lcmd = mode & 0x0F;
    mirror = mode & 0x20;
    no_buf = mode & 0x40;

    if (g_runtime->graph_mode == 1 && (w & 7) != 0) mirror = 0;
    if (g_runtime->graph_mode == 4 && (w & 1) != 0) mirror = 0;

    lrt_begin_draw();
    for (dy = 0; dy < h; dy++) {
        for (dx = 0; dx < w; dx++) {
            int src_x = mirror ? (w - 1 - dx) : dx;
            byte src = lrt_block_src_pixel(g_runtime, data, w, src_x, dy);
            byte dst = lrt_read_index(g_runtime, x + dx, y + dy);
            lrt_write_index(g_runtime, x + dx, y + dy, lrt_apply_lcmd(g_runtime, dst, src, lcmd));
        }
    }
    lrt_end_draw();

    if (no_buf) {
        lrt_refresh();
    }
}

void lrt_get_block(int x, int y, int w, int h, int mode, byte *data)
{
    int dx;
    int dy;
    int row_bytes;
    int start_x;

    (void)mode;

    if (!g_runtime || !data || w <= 0 || h <= 0) return;

    if (g_runtime->graph_mode == 1) {
        row_bytes = w >> 3;
        start_x = x & ~7;
    } else if (g_runtime->graph_mode == 4) {
        row_bytes = (w & ~7) >> 1;
        start_x = x & ~7;
    } else {
        row_bytes = w;
        start_x = x;
    }

    if (row_bytes <= 0) return;
    memset(data, 0, (size_t)row_bytes * (size_t)h);

    if (g_runtime->graph_mode == 1) {
        for (dy = 0; dy < h; dy++) {
            for (dx = 0; dx < row_bytes; dx++) {
                byte t = 0;
                int bit;
                for (bit = 0; bit < 8; bit++) {
                    if (lrt_read_index(g_runtime, start_x + dx * 8 + bit, y + dy)) {
                        t |= (byte)(0x80 >> bit);
                    }
                }
                data[dy * row_bytes + dx] = t;
            }
        }
    } else if (g_runtime->graph_mode == 4) {
        for (dy = 0; dy < h; dy++) {
            for (dx = 0; dx < row_bytes; dx++) {
                byte hi = lrt_read_index(g_runtime, start_x + dx * 2, y + dy) & 0x0F;
                byte lo = lrt_read_index(g_runtime, start_x + dx * 2 + 1, y + dy) & 0x0F;
                data[dy * row_bytes + dx] = (byte)((hi << 4) | lo);
            }
        }
    } else {
        for (dy = 0; dy < h; dy++) {
            for (dx = 0; dx < row_bytes; dx++) {
                data[dy * row_bytes + dx] = lrt_read_index(g_runtime, start_x + dx, y + dy);
            }
        }
    }
}

void lrt_Color16Init(void)
{
    (void)lrt_set_graph_mode(4);
}

void lrt_Color256Init(void)
{
    (void)lrt_set_graph_mode(8);
}

void lrt_fade(int bright)
{
    size_t count;
    size_t i;
    byte fade_level;

    if (!g_runtime || !g_runtime->index_buf) return;
    if (g_runtime->graph_mode == 1) return;

    fade_level = (byte)((bright & 0x0F) ^ 0x0F);
    count = (size_t)g_runtime->screen_width * (size_t)g_runtime->screen_height;

    lrt_begin_draw();
    for (i = 0; i < count; i++) {
        if (g_runtime->index_buf[i] < fade_level) {
            g_runtime->index_buf[i] = fade_level;
        }
    }
    lrt_present_all(g_runtime);
    lrt_end_draw();
    lrt_refresh();
}

void lrt_set_keymap_overlay_visible(int visible)
{
    g_keymap_overlay_visible = visible ? 1 : 0;
}

int lrt_toggle_keymap_overlay(void)
{
    g_keymap_overlay_visible = !g_keymap_overlay_visible;
    return g_keymap_overlay_visible;
}

int lrt_is_keymap_overlay_visible(void)
{
    return g_keymap_overlay_visible;
}

void lrt_draw_keymap_overlay_u32(u32 *pixels, int width, int height, int pitch)
{
    static const char *const keymap[][4] = {
        {"Arrows", "D-Pad", "Tab/PgUp", "L1"},
        {"L/Space", "A", "BS/PgDn", "R1"},
        {"K/LCtrl", "B", "F1", "KeyMap"},
        {"P/LShift", "X", "Select", "Menu"},
        {"O/LAlt", "Y", "6/Enter", "Start"},
        {"5/RCtrl", "SEL", "Alt+Ent", "Full"},
        {"SEL+START", "Exit", "ESC", "Exit"},
        {NULL, NULL, NULL, NULL}
    };
    static const char *title = "Key Mapping";
    static const char *footer = "F1 to hide";
    static const char *version = "LavaX Native";
    int pitch_px;
    int scale;
    int scaled_w;
    int scaled_h;
    int origin_x;
    int origin_y;
    int box_w = 235;
    int box_h = 140;
    int box_x;
    int box_y;
    int x;
    int y;
    int i;
    u32 bg_color = 0x90102020u;
    u32 border_color = 0xFFFFFFFFu;
    u32 key_color = 0xFFFFFF00u;
    u32 action_color = 0xFFFFFFFFu;
    u32 sep_color = 0xFF00FF00u;
    u32 footer_color = 0xFF7FFFFFu;
    u32 version_color = 0xFF7B7B7Bu;

    if (!pixels || width <= 0 || height <= 0 || pitch <= 0) {
        return;
    }

    pitch_px = pitch / (int)sizeof(u32);
    if (pitch_px <= 0) {
        return;
    }

    memset(pixels, 0, (size_t)pitch_px * (size_t)height * sizeof(u32));

    scale = width / 320;
    if (height / 240 < scale) {
        scale = height / 240;
    }
    if (scale < 1) {
        scale = 1;
    }

    scaled_w = 320 * scale;
    scaled_h = 240 * scale;
    origin_x = (width - scaled_w) / 2;
    origin_y = (height - scaled_h) / 2;
    box_x = origin_x + ((320 - box_w) / 2) * scale;
    box_y = origin_y + ((240 - box_h) / 2) * scale;

    lrt_overlay_fill_rect(pixels, width, height, pitch_px,
                          box_x, box_y, box_w * scale, box_h * scale, bg_color);

    for (x = 0; x < box_w; x++) {
        lrt_overlay_fill_rect(pixels, width, height, pitch_px,
                              box_x + x * scale, box_y, scale, scale, border_color);
        lrt_overlay_fill_rect(pixels, width, height, pitch_px,
                              box_x + x * scale, box_y + (box_h - 1) * scale, scale, scale, border_color);
    }
    for (y = 0; y < box_h; y++) {
        lrt_overlay_fill_rect(pixels, width, height, pitch_px,
                              box_x, box_y + y * scale, scale, scale, border_color);
        lrt_overlay_fill_rect(pixels, width, height, pitch_px,
                              box_x + (box_w - 1) * scale, box_y + y * scale, scale, scale, border_color);
    }

    lrt_overlay_draw_string_scaled(
        pixels, width, height, pitch_px,
        box_x + ((box_w - ((int)strlen(title) * 8)) / 2) * scale,
        box_y + 8 * scale,
        title, scale, border_color);

    lrt_overlay_fill_rect(pixels, width, height, pitch_px,
                          box_x + 2 * scale, box_y + 18 * scale,
                          (box_w - 4) * scale, scale, sep_color);

    lrt_overlay_fill_rect(pixels, width, height, pitch_px,
                          box_x + (box_w / 2) * scale, box_y + 19 * scale,
                          scale, (box_h - 46) * scale, sep_color);

    y = box_y + 23 * scale;
    for (i = 0; keymap[i][0]; i++) {
        lrt_overlay_draw_string_scaled(pixels, width, height, pitch_px,
                                       box_x + 6 * scale, y, keymap[i][0], scale, key_color);
        if (keymap[i][1] && keymap[i][1][0]) {
            lrt_overlay_draw_string_scaled(pixels, width, height, pitch_px,
                                           box_x + 72 * scale, y, keymap[i][1], scale, action_color);
        }
        if (keymap[i][2] && keymap[i][2][0]) {
            lrt_overlay_draw_string_scaled(pixels, width, height, pitch_px,
                                           box_x + (box_w / 2 + 6) * scale, y, keymap[i][2], scale, key_color);
        }
        if (keymap[i][3] && keymap[i][3][0]) {
            lrt_overlay_draw_string_scaled(pixels, width, height, pitch_px,
                                           box_x + (box_w / 2 + 72) * scale, y, keymap[i][3], scale, action_color);
        }
        y += 12 * scale;
    }

    lrt_overlay_fill_rect(pixels, width, height, pitch_px,
                          box_x + 2 * scale, box_y + (box_h - 28) * scale,
                          (box_w - 4) * scale, scale, sep_color);

    lrt_overlay_draw_string_scaled(
        pixels, width, height, pitch_px,
        box_x + ((box_w - ((int)strlen(footer) * 8)) / 2) * scale,
        box_y + (box_h - 20) * scale,
        footer, scale, footer_color);

    lrt_overlay_draw_string_scaled(
        pixels, width, height, pitch_px,
        box_x + ((box_w - ((int)strlen(version) * 8)) / 2) * scale,
        box_y + (box_h - 8) * scale,
        version, scale, version_color);
}

/**
 * lrt_init_data - 从 LAV 数据段初始化 gvar_data
 * @param data: LAV 数据段字节（不含头部前16字节）
 * @param len:  数据段长度
 *
 * 遍历数据段，处理 TK_PRESET (0x41) 指令将数据写入 gvar_data。
 * TK_PRESET 格式: opcode(1) + addr(2) + len(2) + data(len)
 */
void lrt_init_data(const unsigned char *data, int len)
{
    extern byte gvar_data[];
    int i = 0;
    while (i + 4 < len) {
        unsigned char op = data[i];
        if (op == 0x41) { /* TK_PRESET */
            int addr = data[i+1] | (data[i+2] << 8);
            int plen = data[i+3] | (data[i+4] << 8);
            /* addr 是 LavaX 地址（如 0x2000），需要转换为 gvar_data 偏移 */
            int offset = addr - 0x2000;  /* VAR_START_ADDR */
            if (plen >= 0 && offset >= 0 && offset + plen <= 0x10000 && i + 5 + plen <= len) {
                memcpy(gvar_data + offset, data + i + 5, plen);
            }
            i += 5 + plen;
        } else {
            break;
        }
    }
}

/**
 * lrt_stack_printf - native 后端 printf 包装
 *
 * 调用约定：rdi=n (参数个数), rsi=调用前rsp (指向参数区域)
 * 注意：call 指令会在栈上 push 返回地址，所以实际栈顶是返回地址。
 * rsi 指向返回地址下方，即第一个参数。
 * 但在函数内部，rsp 比 rsi 少 8（返回地址）。
 *
 * 栈布局（从 rsi 向高地址）:
 *   rsi+0       第 1 个参数 (arg1)
 *   rsi+8       第 2 个参数 (arg2)
 *   ...
 *   rsi+8*(n-2) 第 n-1 个参数
 *   rsi+8*(n-1) 格式字符串指针
 */
int lrt_stack_printf(long long n, long long *stack)
{
    if (n <= 0) return 0;
    const char *fmt = (const char *)stack[n - 1];
    int argc = (int)(n - 1);

    /* 解码 LavaX 编码地址为主机地址 */
    extern byte gvar_data[];
    long long args[6];
    for (int i = 0; i < argc && i < 6; i++) {
        long long val = stack[argc - 1 - i];
        unsigned long long uval = (unsigned long long)val;
        unsigned long long tag = uval >> 48;

        if (tag > 0 && tag <= 127) {
            long long offset = (long long)(uval << 16) >> 16;
            if (offset < 0) {
                /* 局部变量：保持原值 */
            } else {
                val = (long long)(size_t)(gvar_data + offset);
            }
        } else if (uval >= 0x2000 && uval < 0x10000) {
            /* LavaX 全局地址 */
            long long offset = uval - 0x2000;
            if (offset >= 0 && offset < 0x10000) {
                val = (long long)(size_t)(gvar_data + offset);
            }
        }
        args[i] = val;
    }

    switch (argc) {
        case 0: return printf(fmt);
        case 1: return printf(fmt, args[0]);
        case 2: return printf(fmt, args[0], args[1]);
        case 3: return printf(fmt, args[0], args[1], args[2]);
        case 4: return printf(fmt, args[0], args[1], args[2], args[3]);
        default: return printf(fmt, args[0], args[1], args[2], args[3], args[4]);
    }
}

/*============================================================================
 * 标准 I/O API
 *============================================================================*/

/* 前置声明 */
__attribute__((weak)) void lrt_poll_keys(void);
__attribute__((weak)) int lrt_platform_key_down(int key);

/* 键盘状态 - 保持与 LavaX 虚拟机模型一致 */
static int lrt_key_buffer[128] = {0};
static int lrt_key_head = 0;
static int lrt_key_tail = 0;

/* 外部声明 lav_key（由 bridge 代码定义） */
extern volatile unsigned char lav_key;

int lrt_putchar(int c)
{
    /* 默认实现输出到 stdout，上层可覆盖 */
    return putchar(c);
}

int lrt_getchar(void)
{
    /* 阻塞直到有按键 */
    while (lrt_key_head == lrt_key_tail && lav_key < 128) {
        lrt_poll_keys();
        lrt_delay(1);
    }
    if (lav_key >= 128) {
        int key = lav_key & 0x7F;
        lav_key = 0;
        return key;
    }
    int key = lrt_key_buffer[lrt_key_tail];
    lrt_key_tail = (lrt_key_tail + 1) % 128;
    return key;
}

int lrt_inkey(void)
{
    /* 非阻塞获取按键，返回 0 表示无按键 */
    lrt_poll_keys();

    /* 检查 lav_key（来自 SDL 事件循环） */
    if (lav_key >= 128) {
        int key = lav_key & 0x7F;
        lav_key = 0; // 清除按键状态
        return key;
    }

    /* 检查键盘缓冲区 */
    if (lrt_key_head == lrt_key_tail) {
        return 0;
    }
    int key = lrt_key_buffer[lrt_key_tail];
    lrt_key_tail = (lrt_key_tail + 1) % 128;
    return key;
}

int lrt_checkkey(int key)
{
    /* 检查特定按键是否按下 */
    lrt_poll_keys();
    if (lrt_platform_key_down(key)) {
        return 1;
    }
    int i = lrt_key_tail;
    while (i != lrt_key_head) {
        if (lrt_key_buffer[i] == key) {
            return 1;
        }
        i = (i + 1) % 128;
    }
    return 0;
}

void lrt_releasekey(int key)
{
    /* 从缓冲区移除指定按键 */
    int prev = lrt_key_tail;
    int curr = lrt_key_tail;
    while (curr != lrt_key_head) {
        if (lrt_key_buffer[curr] != key) {
            lrt_key_buffer[prev] = lrt_key_buffer[curr];
            prev = (prev + 1) % 128;
        }
        curr = (curr + 1) % 128;
    }
    lrt_key_head = prev;
}

/* 上层可以调用这个函数来推送按键事件 */
__attribute__((weak)) void lrt_push_key(int key)
{
    int next_head = (lrt_key_head + 1) % 128;
    if (next_head != lrt_key_tail) {
        lrt_key_buffer[lrt_key_head] = key;
        lrt_key_head = next_head;
    }
}

/* 上层需要实现这个来从系统获取按键事件 */
__attribute__((weak)) void lrt_poll_keys(void)
{
    /* 默认宿主未提供输入轮询时保持空实现。 */
}

__attribute__((weak)) int lrt_platform_key_down(int key)
{
    (void)key;
    return 0;
}

/*============================================================================
 * 时间 API
 *============================================================================*/

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#endif

void lrt_delay(unsigned int ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    while (ms > 0) {
        unsigned int slice = ms > 5 ? 5 : ms;
        lrt_poll_keys();
        usleep(slice * 1000);
        ms -= slice;
    }
#endif
}

unsigned int lrt_getms(void)
{
#ifdef _WIN32
    return GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

void lrt_gettime(byte *t)
{
    /* t[0] = year低8位, t[1] = year高8位, t[2] = month, t[3] = day, t[4] = hour, t[5] = minute, t[6] = second, t[7] = week */
    time_t now = time(NULL);
    struct tm *tm;
#ifdef _WIN32
    tm = localtime(&now);
#else
    struct tm tm_buf;
    tm = localtime_r(&now, &tm_buf);
#endif
    int year = tm->tm_year + 1900;
    t[0] = (byte)(year & 0xFF);      /* 年低8位 */
    t[1] = (byte)(year >> 8);       /* 年高8位 */
    t[2] = (byte)(tm->tm_mon + 1);  /* 月 */
    t[3] = (byte)tm->tm_mday;       /* 日 */
    t[4] = (byte)tm->tm_hour;        /* 时 */
    t[5] = (byte)tm->tm_min;         /* 分 */
    t[6] = (byte)tm->tm_sec;         /* 秒 */
    t[7] = (byte)tm->tm_wday;         /* 星期 */
}

void lrt_settime(uint8_t y, uint8_t m, uint8_t d, uint8_t h, uint8_t mi, uint8_t s)
{
    /* 大多数系统不允许用户空间设置时间，仅保留接口 */
    (void)y; (void)m; (void)d; (void)h; (void)mi; (void)s;
}

/*============================================================================
 * 随机数 API
 *============================================================================*/

int lrt_random(void)
{
    return rand();
}

void lrt_srandom(unsigned int seed)
{
    srand(seed);
}

/*============================================================================
 * 字符串 API
 *============================================================================*/

/* 辅助函数：将 LavaX 地址转换为主机指针 */
static inline char* addr_to_ptr(addr a)
{
    extern byte gvar_data[];
    if (a >= 0x2000 && a < 0x10000) {
        return (char*)(gvar_data + (a - 0x2000));
    }
    return (char*)(size_t)a;
}

lava_long lrt_strlen(addr str)
{
    return (lava_long)strlen(addr_to_ptr(str));
}

lava_long lrt_strcmp(addr str1, addr str2)
{
    return (lava_long)strcmp(addr_to_ptr(str1), addr_to_ptr(str2));
}

addr lrt_strcpy(addr dst, addr src)
{
    strcpy(addr_to_ptr(dst), addr_to_ptr(src));
    return dst;
}

addr lrt_strcat(addr dst, addr src)
{
    strcat(addr_to_ptr(dst), addr_to_ptr(src));
    return dst;
}

addr lrt_strchr(addr str, int c)
{
    char *result = strchr(addr_to_ptr(str), c);
    if (!result) return 0;
    /* 如果结果在 gvar_data 内部，返回 LavaX 地址 */
    extern byte gvar_data[];
    if (result >= (char*)gvar_data && result < (char*)gvar_data + 0x10000) {
        return (addr)(0x2000 + (result - (char*)gvar_data));
    }
    return (addr)(size_t)result;
}

addr lrt_strstr(addr haystack, addr needle)
{
    char *result = strstr(addr_to_ptr(haystack), addr_to_ptr(needle));
    if (!result) return 0;
    /* 如果结果在 gvar_data 内部，返回 LavaX 地址 */
    extern byte gvar_data[];
    if (result >= (char*)gvar_data && result < (char*)gvar_data + 0x10000) {
        return (addr)(0x2000 + (result - (char*)gvar_data));
    }
    return (addr)(size_t)result;
}

/*============================================================================
 * 字符判断 API
 *============================================================================*/

/* myctype.h already provides all the necessary macros and functions,
 * we just implement them with the correct function names */

int lrt_isalnum(int c)
{
    return isalnum(c);
}

int lrt_isalpha(int c)
{
    return isalpha(c);
}

int lrt_iscntrl(int c)
{
    return iscntrl(c);
}

int lrt_isdigit(int c)
{
    return isdigit(c);
}

int lrt_isgraph(int c)
{
    return isgraph(c);
}

int lrt_islower(int c)
{
    return islower(c);
}

int lrt_isprint(int c)
{
    return isprint(c);
}

int lrt_ispunct(int c)
{
    return ispunct(c);
}

int lrt_isspace(int c)
{
    return isspace(c);
}

int lrt_isupper(int c)
{
    return isupper(c);
}

int lrt_isxdigit(int c)
{
    return isxdigit(c);
}

int lrt_tolower(int c)
{
    return tolower(c);
}

int lrt_toupper(int c)
{
    return toupper(c);
}

/*============================================================================
 * 文件 API
 *============================================================================*/

/* 我们需要将 FILE* 封装为 LavaX 地址 */
#define MAX_OPEN_FILES 16
static FILE *lrt_open_files[MAX_OPEN_FILES] = {NULL};

static addr fptr_to_addr(FILE *fp)
{
    int i;
    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (lrt_open_files[i] == NULL) {
            lrt_open_files[i] = fp;
            /* 使用 0x10000 + slot 作为地址，避免与 gvar_data 冲突 */
            return (addr)(0x10000 + i);
        }
    }
    return 0; /* 超出最大打开文件数 */
}

static FILE* addr_to_fptr(addr a)
{
    int slot = a - 0x10000;
    if (slot >= 0 && slot < MAX_OPEN_FILES) {
        return lrt_open_files[slot];
    }
    return NULL;
}

static void free_fptr_slot(addr a)
{
    int slot = a - 0x10000;
    if (slot >= 0 && slot < MAX_OPEN_FILES) {
        lrt_open_files[slot] = NULL;
    }
}

addr lrt_fopen(addr path, addr mode)
{
    /* 处理路径和模式参数 - 支持两种情况：
     * 1. 字节码模式：path和mode是32位地址（0x2000-0xFFFF）
     * 2. 原生模式：path和mode是实际的char*指针（32位或64位）
     */
    char *path_str, *mode_str;

    /* 检查是否是有效的64位指针（通过检查地址范围） */
    if (path > 0x10000) {
        /* 原生模式：path是64位指针的低32位，直接转换为char* */
        path_str = (char*)(uintptr_t)path;
        mode_str = (char*)(uintptr_t)mode;
    } else {
        /* 字节码模式：path是32位地址，需要转换为指针 */
        path_str = addr_to_ptr(path);
        mode_str = addr_to_ptr(mode);
    }

    FILE *fp = fopen(path_str, mode_str);
    if (!fp) return 0;

    /* 无论什么模式，都返回文件描述符（int），因为游戏代码期望int类型的文件句柄 */
    return (addr)fileno(fp);
}

FILE* lrt_fopen_native(const char *path, const char *mode)
{
    FILE *fp = fopen(path, mode);
    if (!fp) return 0;
    return fp;
}

int lrt_fopen_native_int(const char *path, const char *mode)
{
    FILE *fp = fopen(path, mode);
    if (!fp) return 0;
    return fileno(fp);
}

int lrt_fclose(addr fp)
{
    /* fp是文件描述符（int），直接使用close关闭 */
    return close((int)fp);
}

lava_long lrt_fread(addr buf, lava_long size, lava_long nmemb, addr fp)
{
    /* fp是文件描述符（int），使用fdopen转换为FILE*进行读取 */
    FILE *f = fdopen((int)fp, "rb");
    if (!f) return 0;
    lava_long result = (lava_long)fread(addr_to_ptr(buf), (size_t)size, (size_t)nmemb, f);
    fclose(f);
    return result;
}

lava_long lrt_fwrite(addr buf, lava_long size, lava_long nmemb, addr fp)
{
    /* fp是文件描述符（int），使用fdopen转换为FILE*进行写入 */
    FILE *f = fdopen((int)fp, "wb");
    if (!f) return 0;
    lava_long result = (lava_long)fwrite(addr_to_ptr(buf), (size_t)size, (size_t)nmemb, f);
    fclose(f);
    return result;
}

int lrt_fseek(addr fp, lava_long offset, int whence)
{
    /* fp是文件描述符（int），使用lseek进行定位 */
    return lseek((int)fp, (long)offset, whence);
}

lava_long lrt_ftell(addr fp)
{
    /* fp是文件描述符（int），使用lseek获取当前位置 */
    return (lava_long)lseek((int)fp, 0, SEEK_CUR);
}

int lrt_feof(addr fp)
{
    /* fp是文件描述符（int），使用read检查是否到达文件末尾 */
    char buf[1];
    int n = read((int)fp, buf, 1);
    if (n == 0) return 1;
    lseek((int)fp, -1, SEEK_CUR);
    return 0;
}

void lrt_rewind(addr fp)
{
    /* fp是文件描述符（int），使用lseek重置到文件开头 */
    lseek((int)fp, 0, SEEK_SET);
}

int lrt_fgetc(addr fp)
{
    /* fp是文件描述符（int），使用read读取单个字符 */
    char c;
    int n = read((int)fp, &c, 1);
    if (n == 1) return (unsigned char)c;
    return EOF;
}

int lrt_fputc(int c, addr fp)
{
    /* fp是文件描述符（int），使用write写入单个字符 */
    char ch = (char)c;
    int n = write((int)fp, &ch, 1);
    if (n == 1) return c;
    return EOF;
}

/*============================================================================
 * 配置文件读取 API
 *============================================================================*/

#if defined(__GNUC__)
#define LAVA_HOST_WEAK __attribute__((weak))
#else
#define LAVA_HOST_WEAK
#endif

LAVA_HOST_WEAK char ExePath[260];

/* 跳过空格 */
char *skip_space(char *p) {
    while(*p == ' ' || *p == '\t') p++;
    return p;
}

/* 获取字符串 */
char *get_string(char *p,char *buf,int blen) {
    int len = 0;
    while(*p && *p != ' ' && *p != '\t' && *p != '=' && *p != '\n' && *p != '\r') {
        if(len < blen-1) buf[len++] = *p;
        p++;
    }
    buf[len] = 0;
    return p;
}

/* 跳到下一行 */
char *to_next_line(char *p) {
    while(*p && *p != '\n') p++;
    if(*p == '\n') p++;
    return p;
}

/* 配置项存储 */
static config_t configs[64];
static int num_configs = 0;

/* 设置配置项 */
void set_config(char *name,char *val) {
    int i;
    for(i=0; i<num_configs; i++) {
        if(strcmp(configs[i].name, name) == 0) {
            strcpy(configs[i].val, val);
            return;
        }
    }
    if(num_configs < 64) {
        strcpy(configs[num_configs].name, name);
        strcpy(configs[num_configs].val, val);
        num_configs++;
    }
}

/* 获取配置项 */
int ConfigKey(char *name, char *val) {
    int i;
    for(i=0; i<num_configs; i++) {
        if(strcmp(configs[i].name, name) == 0) {
            strcpy(val, configs[i].val);
            return 1;
        }
    }
    return 0;
}

/* 读取配置文件 - 1:1 复刻 lavaVM */
void ReadConfig(char *fname)
{
    FILE *fp;
    int len;
    char buf[MAX_CONFIG_SIZE];
    char name[64];
    char val[260];
    char *p;

    fp = native_fopen(fname, "rb");
    if (!fp) {
        return;
    }
    len = fread(buf, 1, MAX_CONFIG_SIZE-1, fp);
    fclose(fp);
    buf[len] = 0;

    p = buf;
    for(;;) {
        if (!p[0]) {
            break;
        }
        p = skip_space(p);
        p = get_string(p, name, sizeof(name));
        if (name[0]) {
            p = skip_space(p);
            if (*p == '=') {
                p = skip_space(p+1);
                p = get_string(p, val, sizeof(val));
                if (val[0]) {
                    set_config(name, val);
                }
            }
        }
        p = to_next_line(p);
    }
}

/*============================================================================
 * 路径处理 API
 *============================================================================*/

#ifndef _WIN32
static int findDir(char *out, char *path, char *dir, int lev, int max_lev) {
    struct stat st;
    char buf[PATH_MAX];
    DIR *d;
    struct dirent *ent;

    sprintf(buf, "%s/%s", path, dir);
    if (!stat(buf, &st) && (st.st_mode & S_IFDIR)) {
        strcpy(out, path);
        return 1;
    }
    if (lev < max_lev) {
        d = opendir(path);
        if (d) {
            for (;;) {
                ent = readdir(d);
                if (!ent) {
                    break;
                }
                if (ent->d_name[0] == '.') {
                    continue;
                }
                sprintf(buf, "%s/%s", path, ent->d_name);
                if (!stat(buf, &st) && (st.st_mode & S_IFDIR) &&
                    findDir(out, buf, "LavaXOS", lev + 1, max_lev)) {
                    closedir(d);
                    return 1;
                }
            }
            closedir(d);
        }
    }
    return 0;
}
#endif /* _WIN32 */

LAVA_HOST_WEAK FILE* native_fopen(const char* path, const char* mode)
{
    char full_path[512];
    char* p;
    char* q;

    if (path[0] == '/') {
        snprintf(full_path, sizeof(full_path), "%s%s", ExePath, path);
    } else if (path[0] && path[1] == ':') {
        if (path[2] == '/' || path[2] == '\\') {
            snprintf(full_path, sizeof(full_path), "%s/%s", ExePath, path + 3);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", ExePath, path);
        }
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", ExePath, path);
    }

    p = full_path;
    q = full_path;
    while (*p) {
        if (*p == '/' && *(p+1) == '.') {
            if (*(p+2) == '.' && (*(p+3) == '/' || *(p+3) == '\0')) {
                if (q > full_path) {
                    q--;
                    while (q > full_path && *q != '/') {
                        q--;
                    }
                    if (q == full_path) {
                        q++;
                    }
                }
                p += 3;
            } else if (*(p+2) == '/' || *(p+2) == '\0') {
                p += 2;
            } else {
                *q++ = *p++;
            }
        } else {
            *q++ = *p++;
        }
    }
    *q = '\0';

    if (strncmp(full_path, ExePath, strlen(ExePath)) != 0) {
        fprintf(stderr, "[Lava bridge] Sandbox: Path traversal detected: %s -> %s\n", path, full_path);
        return NULL;
    }

    return fopen(full_path, mode);
}

LAVA_HOST_WEAK void GetExePath() {

    /* Windows: 从 GetModuleFileName 获取并提取目录部分 */
    char *p, *p2 = NULL;
    #ifdef _WIN32
    GetModuleFileName(NULL, ExePath, 260);
    p = ExePath;
    while (*p) {
        if (*p == '\\')
            p2 = p; // 记录最后一个反斜杠位置
        p++;
    }
    *(p2 + 1) = 0; // 截断到目录名
    return;
    #elif defined(__linux__)
    if (getcwd(ExePath, sizeof(ExePath)) != NULL) {
        printf("Current working directory: %s\n", ExePath);
    } else {
        perror("getcwd");
    }
    #endif
}
