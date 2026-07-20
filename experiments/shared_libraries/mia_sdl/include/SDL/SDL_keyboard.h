#ifndef _SDL_KEYBOARD_H
#define _SDL_KEYBOARD_H

#include "SDL_types.h"
#include "SDL_keysym.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_keysym SDL_keysym;

#define SDL_DEFAULT_REPEAT_DELAY  500
#define SDL_DEFAULT_REPEAT_INTERVAL 30

#define SDL_RELEASED 0
#define SDL_PRESSED  1

#define SDL_QUERY   -1
#define SDL_IGNORE   0
#define SDL_DISABLE  0
#define SDL_ENABLE   1

extern Uint8 *SDL_GetKeyState(int *numkeys);
extern Uint8 *SDL_GetKeyState_NonConst(int *numkeys);
extern SDLMod SDL_GetModState(void);
extern void   SDL_SetModState(SDLMod modstate);
extern char  *SDL_GetKeyName(SDLKey key);
extern void   SDL_GetKeyboardState_NonConst(int *numkeys, Uint8 **keys);
extern int    SDL_EnableUNICODE(int enable);
extern int    SDL_EnableKeyRepeat(int delay, int interval);
extern void   SDL_GetKeyRepeat(int *delay, int *interval);

/* SDL 2.0+ naming used inside SDLPAL conditionals. */
extern const Uint8 *SDL_GetKeyboardState(int *numkeys);
extern int           SDL_GetScancodeFromKey(SDLKey key);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_KEYBOARD_H */
