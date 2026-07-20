/*
 * lavapal sdl_compat.h — bridge header consumed by SDLPAL source files.
 *
 * SDLPAL pulls in "sdl_compat.h" from common.h. We redirect to the MiaOS
 * SDL 1.2 compatibility headers that are shared with libmia_sdl_v1.so.
 */
#ifndef LAVAPAL_SDL_COMPAT_H
#define LAVAPAL_SDL_COMPAT_H

#include "SDL/SDL.h"

/* SDLPAL expects these to exist even when SDL 1.2 lacks them. */
#ifndef SDL_RLEACCEL
#define SDL_RLEACCEL 0x00004000
#endif
#ifndef SDL_PHYSPAL
#define SDL_PHYSPAL 0x02
#endif
#ifndef SDL_LOGPAL
#define SDL_LOGPAL 0x01
#endif

/* SDLPAL tests SDL_PATCHLEVEL on SDL 1.2 for "at least 1.2.10" behaviour. */
#if !defined(PAL_HAS_GLSL)
#define PAL_HAS_GLSL 0
#endif
#if !defined(PAL_HAS_JOYSTICKS)
#define PAL_HAS_JOYSTICKS 0
#endif
#if !defined(PAL_HAS_SDLCD)
#define PAL_HAS_SDLCD 0
#endif
#if !defined(PAL_HAS_NATIVEMIDI)
#define PAL_HAS_NATIVEMIDI 0
#endif
#if !defined(PAL_HAS_CONFIG_PAGE)
#define PAL_HAS_CONFIG_PAGE 0
#endif
#if !defined(SDL_CDROM_DISABLED)
#define SDL_CDROM_DISABLED 1
#endif

#endif /* LAVAPAL_SDL_COMPAT_H */
