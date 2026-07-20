#ifndef _SDL_VERSION_H
#define _SDL_VERSION_H

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_MAJOR_VERSION 1
#define SDL_MINOR_VERSION 2
#define SDL_PATCHLEVEL   15

#define SDL_VERSIONNUM(X, Y, Z)  ((X)*1000 + (Y)*100 + (Z))
#define SDL_COMPILEDVERSION     SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL)
#define SDL_VERSION_ATLEAST(X, Y, Z) (SDL_COMPILEDVERSION >= SDL_VERSIONNUM(X, Y, Z))

typedef struct SDL_version {
	Uint8 major;
	Uint8 minor;
	Uint8 patch;
} SDL_version;

#define SDL_VERSION(X)                          \
	do {                                        \
		(X)->major = SDL_MAJOR_VERSION;         \
		(X)->minor = SDL_MINOR_VERSION;         \
		(X)->patch = SDL_PATCHLEVEL;            \
	} while (0)

extern void SDL_GetVersion(SDL_version *ver);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_VERSION_H */
