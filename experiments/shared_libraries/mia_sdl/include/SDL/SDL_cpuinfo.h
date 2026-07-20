#ifndef _SDL_CPUINFO_H
#define _SDL_CPUINFO_H

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SDL_bool SDL_HasRDTSC(void);
extern SDL_bool SDL_HasMMX(void);
extern SDL_bool SDL_Has3DNow(void);
extern SDL_bool SDL_HasSSE(void);
extern SDL_bool SDL_HasSSE2(void);
extern SDL_bool SDL_HasAltiVec(void);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_CPUINFO_H */
