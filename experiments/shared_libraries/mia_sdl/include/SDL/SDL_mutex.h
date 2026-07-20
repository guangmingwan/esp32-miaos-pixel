#ifndef _SDL_MUTEX_H
#define _SDL_MUTEX_H

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SDL_mutex;
typedef struct SDL_mutex SDL_mutex;

extern SDL_mutex *SDL_CreateMutex(void);
extern int        SDL_LockMutex(SDL_mutex *mutex);
extern int        SDL_TryLockMutex(SDL_mutex *mutex);
extern int        SDL_UnlockMutex(SDL_mutex *mutex);
extern void       SDL_DestroyMutex(SDL_mutex *mutex);

#define SDL_mutexP(m)   SDL_LockMutex(m)
#define SDL_mutexV(m)   SDL_UnlockMutex(m)

struct SDL_semaphore;
typedef struct SDL_semaphore SDL_sem;

extern SDL_sem *SDL_CreateSemaphore(Uint32 initial_value);
extern void     SDL_DestroySemaphore(SDL_sem *sem);
extern int      SDL_SemWait(SDL_sem *sem);
extern int      SDL_SemTryWait(SDL_sem *sem);
extern int      SDL_SemWaitTimeout(SDL_sem *sem, Uint32 ms);
extern int      SDL_SemPost(SDL_sem *sem);
extern Uint32   SDL_SemValue(SDL_sem *sem);

struct SDL_cond;
typedef struct SDL_cond SDL_cond;

extern SDL_cond *SDL_CreateCond(void);
extern void      SDL_DestroyCond(SDL_cond *cond);
extern int       SDL_CondSignal(SDL_cond *cond);
extern int       SDL_CondBroadcast(SDL_cond *cond);
extern int       SDL_CondWait(SDL_cond *cond, SDL_mutex *mutex);
extern int       SDL_CondWaitTimeout(SDL_cond *cond, SDL_mutex *mutex, Uint32 ms);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_MUTEX_H */
