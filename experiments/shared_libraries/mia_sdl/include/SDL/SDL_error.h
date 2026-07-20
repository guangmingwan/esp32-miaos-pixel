#ifndef _SDL_ERROR_H
#define _SDL_ERROR_H

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_ERR_OOM       1
#define SDL_ERR_READONLY  2

extern void SDL_SetError(const char *fmt, ...);
extern char *SDL_GetError(void);
extern void SDL_ClearError(void);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_ERROR_H */
