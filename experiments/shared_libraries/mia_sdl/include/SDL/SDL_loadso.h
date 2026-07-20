#ifndef _SDL_LOADSO_H
#define _SDL_LOADSO_H

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void *SDL_LoadObject(const char *sofile);
extern void *SDL_LoadFunction(void *handle, const char *name);
extern void  SDL_UnloadObject(void *handle);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_LOADSO_H */
