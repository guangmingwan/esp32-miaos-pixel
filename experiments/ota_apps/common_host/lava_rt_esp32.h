/*
 * lava_rt_esp32.h - ESP32 native backend for LavaX runtime.
 *
 * Replaces the SDL2-based lava_rt_native.h. Provides the same LavaX API
 * macro surface but routes display/input/timing to the ESP32 display_host
 * + button + esp_timer backends instead of an SDL window/event loop.
 *
 * Include this BEFORE lavax_native_begin.h so that the standard-library
 * includes and lrt_* declarations use real C types; only the game source
 * region (after begin.h) gets the LavaX 16-bit int mapping.
 */
#ifndef LAVA_RT_ESP32_H
#define LAVA_RT_ESP32_H

/* Auto-detect native compilation */
#if defined(__GNUC__) && !defined(LAVA_NATIVE_COMPILED)
#define LAVA_NATIVE_COMPILED 1
#endif

/* Rename the game's main() to user_main() so app_main can call it.
 * Must happen before any game source is seen. */
#ifdef main
#undef main
#endif
#define main user_main

/* LavaX source directives -> no-ops under GCC */
#if defined(__GNUC__)
#define width(x)
#define height(x)
#define color(x)
#define pen(x)
#define autoscreen
#define watch
#define secret
#define loadall
#define bigram
#define encoding(x)
#endif

/* Standard headers (included before type mapping) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

/* LavaX runtime types + lrt_* declarations */
#include "lava/lava_rt.h"

int32_t lrt_present_indexed_frame(const uint8_t *pixels, uint32_t width,
                                  uint32_t height, uint32_t pitch_bytes,
                                  const uint8_t *palette_rgba);

/* ==================== LavaX type aliases used by game shim ==================== */
typedef uint8_t  lava_char;
typedef int16_t  lava_int;
typedef int32_t  lava_long;
typedef float    lava_float;

/* Pointer-access macros (mirror lava_rt_native.h) */
#define PTR(type, a)  (*((type *)(a)))
#define IPTR(type, a) (*((type *)(a)))
#ifdef LPTR
#undef LPTR
#endif
#define LPTR(type, a) (*((type *)(a)))

/* ==================== Graphics API ==================== */
#define SetScreen(mode)        lrt_set_screen(mode)
#define ClearScreen()          lrt_clear_screen()
#define Refresh()              lrt_refresh()

#define Point(x, y, type)      lrt_point(x, y, type)
#define Line(x0, y0, x1, y1, t)        lrt_line(x0, y0, x1, y1, t)
#define Box(x0, y0, x1, y1, f, t)      lrt_box(x0, y0, x1, y1, f, t)
#define Circle(x, y, r, f, t)          lrt_circle(x, y, r, f, t)
#define Ellipse(x, y, a, b, f, t)      lrt_ellipse(x, y, a, b, f, t)
#define Block(x0, y0, x1, y2, t)       lrt_box(x0, y0, x1, y2, 1, t)
#define Rectangle(x1, y1, x2, y2, c)   lrt_box(x1, y1, x2, y2, 0, c)

#define Locate(y, x)           lrt_locate(y, x)
#define TextOut(x, y, str, m)  lrt_textout(x, y, (const char *)(str), m)

#define WriteBlock(x, y, w, h, m, data) lrt_write_block(x, y, w, h, m, (const byte *)(data))
#define GetBlock(x, y, w, h, m, data)   lrt_get_block(x, y, w, h, m, (byte *)(data))
#define GetPoint(x, y)          lrt_get_point(x, y)

#define SetGraphMode(mode)     lrt_set_graph_mode(mode)
#define SetFgColor(color)      lrt_set_fgcolor_global(color)
#define SetBgColor(color)      lrt_set_bgcolor_global(color)
#define SetPalette(s, c, data) lrt_set_palette_vm(s, c, (const unsigned char *)(data))
#define Fade(bright)           lrt_fade(bright)

/* ==================== I/O API ==================== */
#define putchar(c)  lrt_putchar(c)
#define getchar()   lrt_getchar()
#define inkey()     lrt_inkey()
#define Inkey()     lrt_inkey()
#define checkkey(k) lrt_checkkey(k)
#define CheckKey(k) lrt_checkkey(k)
#define releasekey(k) lrt_releasekey(k)
#define ReleaseKey(k) lrt_releasekey(k)

/* ==================== Time API ==================== */
#define delay(ms)   lrt_delay(ms)
#define Delay(ms)   lrt_delay(ms)
#define getms()     lrt_getms()
#define Getms()     lrt_getms()
#define gettime(t)  lrt_gettime(t)
#define GetTime(t)  lrt_gettime((byte *)(t))
#define settime(y,m,d,h,mi,s) lrt_settime(y,m,d,h,mi,s)
#define SetTime(y,m,d,h,mi,s) lrt_settime(y,m,d,h,mi,s)

/* ==================== Random API ==================== */
#define random()    lrt_random()
#define srandom(s)  lrt_srandom(s)

/* ==================== Math helpers ==================== */
#ifndef abs
#define abs(x) ((x) < 0 ? -(x) : (x))
#endif
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

/* ==================== Globals provided by lava_esp32_backend.c ==================== */
extern volatile unsigned char lav_key;
extern char ExePath[260];
extern int g_lava_shutdown_requested;
#endif /* LAVA_RT_ESP32_H */
