#ifndef _SDL_MOUSE_H
#define _SDL_MOUSE_H

#include "SDL_types.h"
#include "SDL_pixels.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WMcursor WMcursor;
typedef struct SDL_Cursor {
	SDL_Rect    area;
	Sint16      hot_x, hot_y;
	Uint8      *data;
	Uint8      *mask;
	Uint8      *save[2];
	WMcursor   *wm_cursor;
} SDL_Cursor;

extern Uint8  SDL_GetMouseState(int *x, int *y);
extern Uint8  SDL_GetRelativeMouseState(int *x, int *y);
extern void   SDL_WarpMouse(Uint16 x, Uint16 y);
extern SDL_Cursor *SDL_CreateCursor(Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y);
extern void   SDL_SetCursor(SDL_Cursor *cursor);
extern SDL_Cursor *SDL_GetCursor(void);
extern void   SDL_FreeCursor(SDL_Cursor *cursor);
extern int    SDL_ShowCursor(int toggle);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_MOUSE_H */
