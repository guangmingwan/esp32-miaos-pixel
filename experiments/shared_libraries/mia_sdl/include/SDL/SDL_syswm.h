#ifndef _SDL_SYSWM_H
#define _SDL_SYSWM_H

#include "SDL_types.h"
#include "SDL_version.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_SysWMmsg {
	SDL_version version;
	int         data;
} SDL_SysWMmsg;

typedef struct SDL_SysWMinfo {
	SDL_version version;
	int         data;
} SDL_SysWMinfo;

extern int SDL_GetWMInfo(SDL_SysWMinfo *info);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_SYSWM_H */
