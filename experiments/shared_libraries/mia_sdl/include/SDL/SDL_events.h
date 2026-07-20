#ifndef _SDL_EVENTS_H
#define _SDL_EVENTS_H

#include "SDL_types.h"
#include "SDL_keyboard.h"
#include "SDL_mouse.h"
#include "SDL_joystick.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_ALLEVENTS 0xFFFFFFFF

typedef enum {
	SDL_NOEVENT         = 0,
	SDL_ACTIVEEVENT     = 1,
	SDL_KEYDOWN         = 2,
	SDL_KEYUP           = 3,
	SDL_MOUSEMOTION     = 4,
	SDL_MOUSEBUTTONDOWN = 5,
	SDL_MOUSEBUTTONUP   = 6,
	SDL_JOYAXISMOTION   = 7,
	SDL_JOYBALLMOTION   = 8,
	SDL_JOYHATMOTION    = 9,
	SDL_JOYBUTTONDOWN   = 10,
	SDL_JOYBUTTONUP     = 11,
	SDL_QUIT            = 12,
	SDL_SYSWMEVENT      = 13,
	SDL_EVENT_RESERVED1 = 14,
	SDL_EVENT_RESERVED2 = 15,
	SDL_EVENT_RESERVED3 = 16,
	SDL_VIDEORESIZE     = 17,
	SDL_VIDEOEXPOSE     = 18,
	SDL_EVENT_RESERVED4 = 19,
	SDL_EVENT_RESERVED5 = 20,
	SDL_EVENT_RESERVED6 = 21,
	SDL_EVENT_RESERVED7 = 22,
	SDL_USEREVENT       = 24,
	SDL_NUMEVENTS       = 32
} SDL_EventType;

typedef struct SDL_ActiveEvent {
	Uint8 type;
	Uint8 gain;
	Uint8 state;
} SDL_ActiveEvent;

typedef struct SDL_KeyboardEvent {
	Uint8      type;
	Uint8      which;
	Uint8      state;
	SDL_keysym keysym;
} SDL_KeyboardEvent;

typedef struct SDL_MouseMotionEvent {
	Uint8  type;
	Uint8  which;
	Uint8  state;
	Uint16 x, y;
	Sint16 xrel, yrel;
} SDL_MouseMotionEvent;

typedef struct SDL_MouseButtonEvent {
	Uint8  type;
	Uint8  which;
	Uint8  button;
	Uint8  state;
	Uint16 x, y;
} SDL_MouseButtonEvent;

typedef struct SDL_JoyAxisEvent {
	Uint8  type;
	Uint8  which;
	Uint8  axis;
	Sint16 value;
} SDL_JoyAxisEvent;

typedef struct SDL_JoyButtonEvent {
	Uint8 type;
	Uint8 which;
	Uint8 button;
	Uint8 state;
} SDL_JoyButtonEvent;

typedef struct SDL_QuitEvent {
	Uint8  type;
	int    code;
} SDL_QuitEvent;

typedef struct SDL_UserEvent {
	Uint8  type;
	int    code;
	void  *data1;
	void  *data2;
} SDL_UserEvent;

typedef struct SDL_SysWMmsg SDL_SysWMmsg;
typedef struct SDL_SysWMEvent {
	Uint8        type;
	SDL_SysWMmsg *msg;
} SDL_SysWMEvent;

typedef struct SDL_ResizeEvent {
	Uint8 type;
	int   w, h;
} SDL_ResizeEvent;

typedef union SDL_Event {
	Uint8                  type;
	SDL_ActiveEvent        active;
	SDL_KeyboardEvent      key;
	SDL_MouseMotionEvent   motion;
	SDL_MouseButtonEvent   button;
	SDL_JoyAxisEvent       jaxis;
	SDL_JoyButtonEvent     jbutton;
	SDL_QuitEvent          quit;
	SDL_UserEvent          user;
	SDL_SysWMEvent         syswm;
	SDL_ResizeEvent        resize;
} SDL_Event;

extern void   SDL_PumpEvents(void);
extern int    SDL_PollEvent(SDL_Event *event);
extern int    SDL_WaitEvent(SDL_Event *event);
extern int    SDL_PushEvent(SDL_Event *event);
extern int    SDL_PeepEvents(SDL_Event *events, int numevents, int action, Uint32 mask);
#define SDL_ADDEVENT    0
#define SDL_PEEKEVENT   1
#define SDL_GETEVENT    2

typedef int (*SDL_EventFilter)(const SDL_Event *event);
extern void   SDL_SetEventFilter(SDL_EventFilter filter);
extern SDL_EventFilter SDL_GetEventFilter(void);
extern void   SDL_FilterEvents(SDL_EventFilter filter, void *userdata);
extern Uint8  SDL_EventState(Uint8 type, int state);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_EVENTS_H */
