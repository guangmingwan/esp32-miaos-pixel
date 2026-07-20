/*
 * MiaOS SDL 1.2 compatibility layer — umbrella header.
 *
 * This is a minimal SDL 1.2-compatible surface for running SDLPAL on
 * ESP32-S3. It provides the SDL types, function declarations, and macros
 * that SDLPAL references. The implementation lives in libmia_sdl_v1.so
 * and is wired up by the host firmware through MiaSdlApi.
 *
 * Include this header as either <SDL/SDL.h> or "SDL/SDL.h".
 */
#ifndef _SDL_H
#define _SDL_H

#include "SDL_main.h"      /* empty stub (see below) */
#include "SDL_version.h"
#include "SDL_stdinc.h"
#include "SDL_byteorder.h"
#include "SDL_getenv.h"
#include "SDL_error.h"
#include "SDL_types.h"
#include "SDL_timer.h"
#include "SDL_rwops.h"
#include "SDL_mutex.h"
#include "SDL_thread.h"
#include "SDL_pixels.h"
#include "SDL_surface.h"
#include "SDL_video.h"
#include "SDL_events.h"
#include "SDL_keyboard.h"
#include "SDL_mouse.h"
#include "SDL_joystick.h"
#include "SDL_cdrom.h"
#include "SDL_audio.h"
#include "SDL_active.h"
#include "SDL_syswm.h"
#include "SDL_cpuinfo.h"
#include "SDL_loadso.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_INIT_TIMER      0x00000001
#define SDL_INIT_AUDIO      0x00000010
#define SDL_INIT_VIDEO      0x00000020
#define SDL_INIT_CDROM      0x00000100
#define SDL_INIT_JOYSTICK   0x00000200
#define SDL_INIT_NOPARACHUTE 0x00100000
#define SDL_INIT_EVENTTHREAD 0x01000000
#define SDL_INIT_EVERYTHING  0x0000FFFF

#define SDL_OK   0
#define SDL_FAIL -1
#define SDL_ENABLE  1
#define SDL_DISABLE 0
#define SDL_IGNORE  0
#define SDL_QUERY   (-1)

#define SDL_TICKS_PASSED(A, B) ((Sint32)((B) - (A)) <= 0)

extern int  SDL_Init(Uint32 flags);
extern int  SDL_InitSubSystem(Uint32 flags);
extern void SDL_QuitSubSystem(Uint32 flags);
extern Uint32 SDL_WasInit(Uint32 flags);
extern void SDL_Quit(void);
extern const SDL_version *SDL_Linked_Version(void);
extern const char *SDL_GetPlatform(void);

#ifdef __cplusplus
}
#endif

#endif /* _SDL_H */
