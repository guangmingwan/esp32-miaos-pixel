/*
 * MiaOS SDL 1.2 compatibility layer — basic types.
 * Minimal SDL 1.2-compatible definitions for SDLPAL on ESP32-S3.
 */
#ifndef _SDL_TYPES_H
#define _SDL_TYPES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	SDL_FALSE = 0,
	SDL_TRUE  = 1
} SDL_bool;

typedef uint8_t  Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;
typedef int8_t   Sint8;
typedef int16_t  Sint16;
typedef int32_t  Sint32;
typedef int64_t  Sint64;

/* Calling-convention macro; empty on Xtensa GCC. */
#ifndef SDLCALL
#define SDLCALL
#endif
#ifndef SDL_INLINE
#define SDL_INLINE __inline__
#endif

#define SDL_DECLARE_GLOBAL
#define DUMMYENUM_SDL1234 1

#ifdef __cplusplus
}
#endif
#endif /* _SDL_TYPES_H */
