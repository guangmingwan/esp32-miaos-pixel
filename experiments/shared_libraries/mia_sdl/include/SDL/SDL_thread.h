#ifndef _SDL_THREAD_H
#define _SDL_THREAD_H

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_Thread SDL_Thread;
typedef unsigned long SDL_threadID;
typedef int (*SDL_ThreadFunction)(void *data);

extern SDL_Thread *SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *data);
extern SDL_threadID SDL_GetThreadID(SDL_Thread *thread);
extern const char  *SDL_GetThreadName(SDL_Thread *thread);
extern void         SDL_WaitThread(SDL_Thread *thread, int *status);
extern SDL_threadID SDL_ThreadID(void);
extern void         SDL_DetachThread(SDL_Thread *thread);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_THREAD_H */
