#ifndef _SDL_TIMER_H
#define _SDL_TIMER_H

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern Uint32 SDL_GetTicks(void);
extern void   SDL_Delay(Uint32 ms);

typedef Uint32 (*SDL_TimerCallback)(Uint32 interval);
typedef int    SDL_TimerID;

extern SDL_TimerID SDL_AddTimer(Uint32 interval, SDL_TimerCallback callback, void *param);
extern SDL_bool    SDL_RemoveTimer(SDL_TimerID id);

extern Uint64 SDL_GetPerformanceCounter(void);
extern Uint64 SDL_GetPerformanceFrequency(void);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_TIMER_H */
