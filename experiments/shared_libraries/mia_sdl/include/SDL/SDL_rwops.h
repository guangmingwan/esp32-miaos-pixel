#ifndef _SDL_RWOPS_H
#define _SDL_RWOPS_H

#include "SDL_types.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_RWops {
	Sint32 (*seek)(struct SDL_RWops *context, Sint32 offset, int whence);
	Sint32 (*read)(struct SDL_RWops *context, void *ptr, Sint32 size, Sint32 maxnum);
	Sint32 (*write)(struct SDL_RWops *context, const void *ptr, Sint32 size, Sint32 num);
	int    (*close)(struct SDL_RWops *context);
	Uint32  type;
	union {
		struct { FILE *fp; } stdio;
		struct { Uint8 *base; Uint8 *here; Uint8 *stop; } mem;
		struct { void *data1; } unknown;
	} hidden;
} SDL_RWops;

#define RW_SEEK_SET 0
#define RW_SEEK_CUR 1
#define RW_SEEK_END 2

#define SDL_RWseek(ctx, offset, whence) (ctx)->seek(ctx, offset, whence)
#define SDL_RWtell(ctx)                 (ctx)->seek(ctx, 0, RW_SEEK_CUR)
#define SDL_RWread(ctx, ptr, size, n)   (ctx)->read(ctx, ptr, size, n)
#define SDL_RWwrite(ctx, ptr, size, n)  (ctx)->write(ctx, ptr, size, n)
#define SDL_RWclose(ctx)                (ctx)->close(ctx)

extern SDL_RWops *SDL_RWFromFile(const char *file, const char *mode);
extern SDL_RWops *SDL_RWFromFP(FILE *fp, SDL_bool autoclose);
extern SDL_RWops *SDL_RWFromMem(void *data, int size);
extern SDL_RWops *SDL_RWFromConstMem(const void *data, int size);
extern SDL_RWops *SDL_AllocRW(void);
extern void       SDL_FreeRW(SDL_RWops *area);

#define SDL_RWsize(ctx) ((ctx)->seek(ctx, 0, RW_SEEK_END) - (ctx)->seek(ctx, 0, RW_SEEK_SET))

#ifdef __cplusplus
}
#endif
#endif /* _SDL_RWOPS_H */
