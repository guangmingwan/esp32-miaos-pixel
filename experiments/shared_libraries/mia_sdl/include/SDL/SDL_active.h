#ifndef _SDL_ACTIVE_H
#define _SDL_ACTIVE_H

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_APPMOUSEFOCUS 0x01
#define SDL_APPINPUTFOCUS 0x02
#define SDL_APPACTIVE     0x04

extern Uint8 SDL_GetAppState(void);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_ACTIVE_H */
