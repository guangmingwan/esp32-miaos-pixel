#ifndef _SDL_BYTEORDER_H
#define _SDL_BYTEORDER_H

/* ESP32-S3 (Xtensa LX7) is little-endian. */
#define SDL_LIL_ENDIAN  1234
#define SDL_BIG_ENDIAN 4321
#define SDL_BYTEORDER   SDL_LIL_ENDIAN

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_Swap16(X)   (Uint16)((((Uint16)(X)) << 8) | (((Uint16)(X)) >> 8))
#define SDL_Swap32(X)   (Uint32)((((Uint32)(X)) << 24) | ((((Uint32)(X)) << 8) & 0x00FF0000) | ((((Uint32)(X)) >> 8) & 0x0000FF00) | (((Uint32)(X)) >> 24))

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define SDL_SwapLE16(X) (X)
#define SDL_SwapLE32(X) (X)
#define SDL_SwapBE16(X) SDL_Swap16(X)
#define SDL_SwapBE32(X) SDL_Swap32(X)
#else
#define SDL_SwapLE16(X) SDL_Swap16(X)
#define SDL_SwapLE32(X) SDL_Swap32(X)
#define SDL_SwapBE16(X) (X)
#define SDL_SwapBE32(X) (X)
#endif

#ifdef __cplusplus
}
#endif
#endif /* _SDL_BYTEORDER_H */
