/*
 * SDL_main compatibility stub. SDLPAL pulls in <SDL3/SDL_main.h> only on
 * Emscripten, so this file only needs to be includable.
 */
#ifndef _SDL_MAIN_H
#define _SDL_MAIN_H

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_MAIN_HANDLED

extern int SDL_RegisterApp(const char *name, Uint32 style, void *hInst);
extern void SDL_UnregisterApp(void);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_MAIN_H */
