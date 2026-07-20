#ifndef _SDL_VIDEO_H
#define _SDL_VIDEO_H

#include "SDL_types.h"
#include "SDL_pixels.h"
#include "SDL_surface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_VideoInfo {
	Uint32          hw_available:1;
	Uint32          wm_available:1;
	Uint32          UnusedBits1:6;
	Uint32          UnusedBits2:1;
	Uint32          blit_hw:1;
	Uint32          blit_hw_CC:1;
	Uint32          blit_hw_A:1;
	Uint32          blit_sw:1;
	Uint32          blit_sw_CC:1;
	Uint32          blit_sw_A:1;
	Uint32          blit_fill:1;
	Uint32          UnusedBits3:16;
	Uint32          video_mem;
	SDL_PixelFormat *vfmt;
} SDL_VideoInfo;

typedef enum {
	SDL_GRAB_QUERY = -1,
	SDL_GRAB_OFF = 0,
	SDL_GRAB_ON = 1,
	SDL_GRAB_FULLSCREEN  = 2
} SDL_GrabMode;

extern const SDL_VideoInfo *SDL_GetVideoInfo(void);
extern SDL_Rect    **SDL_ListModes(SDL_PixelFormat *format, Uint32 flags);
extern int           SDL_VideoModeOK(int width, int height, int bpp, Uint32 flags);
extern SDL_Surface  *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags);
extern void          SDL_UpdateRect(SDL_Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h);
extern void          SDL_UpdateRects(SDL_Surface *screen, int numrects, SDL_Rect *rects);
extern int           SDL_Flip(SDL_Surface *screen);
extern int           SDL_SetGamma(float redgamma, float greengamma, float bluegamma);
extern int           SDL_SetGammaRamp(const Uint16 *red, const Uint16 *green, const Uint16 *blue);
extern int           SDL_GetGammaRamp(Uint16 *red, Uint16 *green, Uint16 *blue);
extern void          SDL_WM_SetCaption(const char *title, const char *icon);
extern void          SDL_WM_GetCaption(char **title, char **icon);
extern void          SDL_WM_SetIcon(SDL_Surface *icon, Uint8 *mask);
extern int           SDL_WM_IconifyWindow(void);
extern int           SDL_WM_ToggleFullScreen(SDL_Surface *surface);
extern int           SDL_ShowCursor(int toggle);
extern SDL_GrabMode  SDL_WM_GrabInput(SDL_GrabMode mode);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_VIDEO_H */
