#ifndef LAVA_SDL_ESP32_H
#define LAVA_SDL_ESP32_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lava/lava_rt.h"

#define SDL_MAJOR_VERSION 1
#define SDL_MINOR_VERSION 2
#define SDL_VERSION_ATLEAST(x, y, z) 0
#define SDL_OK 0
#define SDL_FAIL -1
#define SDL_SWSURFACE 0
#define SDL_INIT_VIDEO 0x01
#define SDL_INIT_AUDIO 0x02
#define SDL_INIT_TIMER 0x04
#define SDL_INIT_CDROM 0
#define SDL_INIT_JOYSTICK 0
#define SDL_INIT_NOPARACHUTE 0
#define SDL_AUDIO_BITSIZE(x) ((x) & 0xff)
#define SDL_TICKS_PASSED(a, b) ((int32_t)((b) - (a)) <= 0)
#define SDL_FORCE_INLINE static inline __attribute__((always_inline))
#define SDL_SwapLE16(x) (x)
#define SDL_SwapLE32(x) (x)

typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef int32_t Sint32;
typedef uint32_t SDL_AudioDeviceID;
typedef int SDL_ScaleMode;

typedef struct SDL_Rect {
    int x;
    int y;
    int w;
    int h;
} SDL_Rect;

typedef struct SDL_Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} SDL_Color;

typedef struct SDL_Surface {
    int w;
    int h;
    int pitch;
    void *pixels;
    void *format_palette;
} SDL_Surface;

typedef struct SDL_Event { uint32_t type; } SDL_Event;
typedef struct SDL_AudioSpec { int unused; } SDL_AudioSpec;
typedef struct SDL_AudioStream SDL_AudioStream;
typedef struct SDL_CD SDL_CD;
typedef struct SDL_Texture SDL_Texture;

static inline SDL_Surface *SDL_CreateRGBSurface(
    uint32_t flags, int width, int height, int depth,
    uint32_t rmask, uint32_t gmask, uint32_t bmask, uint32_t amask) {
    SDL_Surface *surface;
    size_t bytes_per_pixel;
    (void)flags;
    (void)rmask;
    (void)gmask;
    (void)bmask;
    (void)amask;
    if (width <= 0 || height <= 0 || depth <= 0) return NULL;
    bytes_per_pixel = (size_t)(depth + 7) / 8;
    surface = (SDL_Surface *)calloc(1, sizeof(*surface));
    if (surface == NULL) return NULL;
    surface->pixels = calloc((size_t)width * (size_t)height, bytes_per_pixel);
    if (surface->pixels == NULL) {
        free(surface);
        return NULL;
    }
    surface->w = width;
    surface->h = height;
    surface->pitch = (int)((size_t)width * bytes_per_pixel);
    return surface;
}

static inline void SDL_FreeSurface(SDL_Surface *surface) {
    if (surface == NULL) return;
    free(surface->pixels);
    free(surface);
}

static inline int SDL_BlitSurface(const SDL_Surface *source, const SDL_Rect *source_rect,
                                  SDL_Surface *target, SDL_Rect *target_rect) {
    SDL_Rect src;
    int dx;
    int dy;
    if (source == NULL || target == NULL || source->pixels == NULL || target->pixels == NULL)
        return SDL_FAIL;
    src = source_rect ? *source_rect : (SDL_Rect){0, 0, source->w, source->h};
    dx = target_rect ? target_rect->x : 0;
    dy = target_rect ? target_rect->y : 0;
    for (int row = 0; row < src.h; ++row) {
        int sy = src.y + row;
        int ty = dy + row;
        if (sy < 0 || sy >= source->h || ty < 0 || ty >= target->h) continue;
        int sx = src.x;
        int tx = dx;
        int count = src.w;
        if (sx < 0) { tx -= sx; count += sx; sx = 0; }
        if (tx < 0) { sx -= tx; count += tx; tx = 0; }
        if (sx + count > source->w) count = source->w - sx;
        if (tx + count > target->w) count = target->w - tx;
        if (count > 0) {
            memcpy((uint8_t *)target->pixels + ty * target->pitch + tx,
                   (const uint8_t *)source->pixels + sy * source->pitch + sx,
                   (size_t)count);
        }
    }
    return SDL_OK;
}

static inline int SDL_Init(uint32_t flags) { (void)flags; return SDL_OK; }
static inline void SDL_Quit(void) {}
static inline const char *SDL_GetError(void) { return "ESP32 SDL compatibility error"; }
static inline void SDL_Delay(uint32_t milliseconds) { lrt_delay(milliseconds); }
static inline uint32_t SDL_GetTicks(void) { return lrt_getms(); }

#endif
