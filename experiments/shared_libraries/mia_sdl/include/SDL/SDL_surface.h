#ifndef _SDL_SURFACE_H
#define _SDL_SURFACE_H

#include "SDL_types.h"
#include "SDL_pixels.h"
#include "SDL_rwops.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Surface flags */
#define SDL_SWSURFACE       0x00000000
#define SDL_HWSURFACE       0x00000001
#define SDL_ASYNCBLIT       0x00000004
#define SDL_ANYFORMAT       0x10000000
#define SDL_HWPALETTE       0x20000000
#define SDL_DOUBLEBUF       0x40000000
#define SDL_FULLSCREEN      0x80000000
#define SDL_OPENGL          0x00000002
#define SDL_OPENGLBLIT      0x0000000A
#define SDL_RESIZABLE       0x00000010
#define SDL_NOFRAME         0x00000020
#define SDL_HWACCEL         0x00000100
#define SDL_SRCCOLORKEY     0x00001000
#define SDL_RLEACCELOK      0x00002000
#define SDL_RLEACCEL        0x00004000
#define SDL_SRCALPHA        0x00010000
#define SDL_PREALLOC        0x01000000

#define SDL_MUSTLOCK(surface)                                          \
	((surface) && (((surface)->flags & (SDL_HWSURFACE | SDL_ASYNCBLIT | \
	                                     SDL_RLEACCEL)) != 0))

typedef int (*SDL_blit)(struct SDL_Surface *src, SDL_Rect *srcrect,
                        struct SDL_Surface *dst, SDL_Rect *dstrect);

typedef struct SDL_BlitMap {
	struct SDL_Surface *dst;
	int                 ident;
	SDL_blit            blit;
	void               *data;
	SDL_PixelFormat    *src_palette;
	SDL_PixelFormat    *dst_palette;
	Uint32              info;
} SDL_BlitMap;

typedef struct SDL_Surface {
	Uint32           flags;
	SDL_PixelFormat *format;
	int              w, h;
	Uint16           pitch;
	void            *pixels;
	int              offset;
	SDL_Rect         clip_rect;
	SDL_BlitMap     *map;
	unsigned int     format_version;
	int              refcount;
	void            *hwdata;
	int              locked;
	void            *unused1;
} SDL_Surface;

extern SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth,
                                         Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
extern SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth,
                                             int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
extern SDL_Surface *SDL_DisplayFormat(SDL_Surface *surface);
extern SDL_Surface *SDL_DisplayFormatAlpha(SDL_Surface *surface);
extern void         SDL_FreeSurface(SDL_Surface *surface);
extern int          SDL_LockSurface(SDL_Surface *surface);
extern void         SDL_UnlockSurface(SDL_Surface *surface);
extern int          SDL_SetPalette(SDL_Surface *surface, int flags, const SDL_Color *colors,
                                   int firstcolor, int ncolors);
extern int          SDL_SetColors(SDL_Surface *surface, const SDL_Color *colors,
                                  int firstcolor, int ncolors);
extern int          SDL_SetColorKey(SDL_Surface *surface, Uint32 flag, Uint32 key);
extern int          SDL_SetAlpha(SDL_Surface *surface, Uint32 flag, Uint8 alpha);
extern int          SDL_SetSurfacePalette(SDL_Surface *surface, SDL_Palette *palette);
extern int          SDL_SetSurfaceColorMod(SDL_Surface *surface, Uint8 r, Uint8 g, Uint8 b);
extern int          SDL_FillRect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color);
extern int          SDL_BlitSurface(SDL_Surface *src, const SDL_Rect *srcrect,
                                    SDL_Surface *dst, SDL_Rect *dstrect);
extern int          SDL_BlitScaled(SDL_Surface *src, const SDL_Rect *srcrect,
                                   SDL_Surface *dst, SDL_Rect *dstrect);
extern int          SDL_SoftStretch(SDL_Surface *src, const SDL_Rect *srcrect,
                                    SDL_Surface *dst, const SDL_Rect *dstrect);
extern int          SDL_UpperBlit(SDL_Surface *src, const SDL_Rect *srcrect,
                                  SDL_Surface *dst, SDL_Rect *dstrect);
extern int          SDL_LowerBlit(SDL_Surface *src, SDL_Rect *srcrect,
                                  SDL_Surface *dst, SDL_Rect *dstrect);
extern SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, const SDL_PixelFormat *fmt, Uint32 flags);
extern SDL_Surface *SDL_LoadBMP_RW(SDL_RWops *src, int freesrc);
extern int          SDL_SaveBMP_RW(SDL_Surface *surface, SDL_RWops *dst, int freedst);

#define SDL_LoadBMP(file)   SDL_LoadBMP_RW(SDL_RWFromFile(file, "rb"), 1)
#define SDL_SaveBMP(surface, file) SDL_SaveBMP_RW(surface, SDL_RWFromFile(file, "wb"), 1)

#define SDL_LOGPAL  0x01
#define SDL_PHYSPAL 0x02

#ifdef __cplusplus
}
#endif
#endif /* _SDL_SURFACE_H */
