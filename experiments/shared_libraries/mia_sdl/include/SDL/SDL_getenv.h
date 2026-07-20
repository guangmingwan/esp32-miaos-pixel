#ifndef _SDL_GETENV_H
#define _SDL_GETENV_H

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_getenv  getenv
#define SDL_putenv  putenv
#define SDL_setenv  setenv

#ifdef __cplusplus
}
#endif
#endif /* _SDL_GETENV_H */
