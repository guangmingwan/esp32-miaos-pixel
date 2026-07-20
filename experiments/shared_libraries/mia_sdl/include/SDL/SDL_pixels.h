#ifndef _SDL_PIXELS_H
#define _SDL_PIXELS_H

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_Rect {
	Sint16 x, y;
	Uint16 w, h;
} SDL_Rect;

typedef struct SDL_Color {
	Uint8 r, g, b;
	Uint8 unused;
} SDL_Color;

typedef struct SDL_Palette {
	int        ncolors;
	SDL_Color *colors;
	Uint32     version;
	int        refcount;
} SDL_Palette;

typedef struct SDL_PixelFormat {
	SDL_Palette *palette;
	Uint8  BitsPerPixel;
	Uint8  BytesPerPixel;
	Uint8  Rloss, Gloss, Bloss, Aloss;
	Uint8  Rshift, Gshift, Bshift, Ashift;
	Uint32 Rmask, Gmask, Bmask, Amask;
	Uint32 colorkey;
	Uint8  alpha;
} SDL_PixelFormat;

extern Uint32   SDL_MapRGB(const SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b);
extern Uint32   SDL_MapRGBA(const SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
extern void     SDL_GetRGB(Uint32 pixel, const SDL_PixelFormat *fmt, Uint8 *r, Uint8 *g, Uint8 *b);
extern void     SDL_GetRGBA(Uint32 pixel, const SDL_PixelFormat *fmt, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a);

extern SDL_Palette *SDL_AllocPalette(int ncolors);
extern void         SDL_FreePalette(SDL_Palette *palette);
extern int          SDL_SetPaletteColors(SDL_Palette *palette, const SDL_Color *colors, int firstcolor, int ncolors);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_PIXELS_H */
