#ifndef _SDL_JOYSTICK_H
#define _SDL_JOYSTICK_H

#include "SDL_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SDL_Joystick;
typedef struct SDL_Joystick SDL_Joystick;
typedef Sint32 SDL_JoystickID;

#define SDL_HAT_CENTERED    0x00
#define SDL_HAT_UP          0x01
#define SDL_HAT_RIGHT       0x02
#define SDL_HAT_DOWN        0x04
#define SDL_HAT_LEFT        0x08
#define SDL_HAT_RIGHTUP     (SDL_HAT_RIGHT | SDL_HAT_UP)
#define SDL_HAT_RIGHTDOWN   (SDL_HAT_RIGHT | SDL_HAT_DOWN)
#define SDL_HAT_LEFTUP      (SDL_HAT_LEFT  | SDL_HAT_UP)
#define SDL_HAT_LEFTDOWN    (SDL_HAT_LEFT  | SDL_HAT_DOWN)

extern int   SDL_NumJoysticks(void);
extern const char *SDL_JoystickName(int device_index);
extern const char *SDL_JoystickNameForIndex(int device_index);
extern SDL_Joystick *SDL_JoystickOpen(int device_index);
extern void SDL_JoystickClose(SDL_Joystick *joystick);
extern int   SDL_JoystickEventState(int state);
extern int   SDL_JoystickGetAxis(SDL_Joystick *joystick, int axis);
extern Uint8 SDL_JoystickGetHat(SDL_Joystick *joystick, int hat);
extern int   SDL_JoystickGetBall(SDL_Joystick *joystick, int ball, int *dx, int *dy);
extern Uint8 SDL_JoystickGetButton(SDL_Joystick *joystick, int button);
extern int   SDL_JoystickNumAxes(SDL_Joystick *joystick);
extern int   SDL_JoystickNumBalls(SDL_Joystick *joystick);
extern int   SDL_JoystickNumHats(SDL_Joystick *joystick);
extern int   SDL_JoystickNumButtons(SDL_Joystick *joystick);
extern void  SDL_JoystickUpdate(void);
extern int   SDL_JoystickGetAxisInitialState(SDL_Joystick *joystick, int axis, Sint16 *state);

/* SDL 2.0+ names retained for SDLPAL conditional code. */
extern int            SDL_GetNumJoysticks(void);
extern const char    *SDL_GetJoystickNameForID(SDL_JoystickID instance_id);
extern const char    *SDL_GetJoystickName(SDL_Joystick *joystick);
extern SDL_JoystickID SDL_GetJoysticks(void);
extern SDL_Joystick  *SDL_OpenJoystick(SDL_JoystickID instance_id);
extern int            SDL_GetNumJoystickAxes(SDL_Joystick *joystick);
extern int            SDL_GetNumJoystickBalls(SDL_Joystick *joystick);
extern int            SDL_GetNumJoystickHats(SDL_Joystick *joystick);
extern int            SDL_GetNumJoystickButtons(SDL_Joystick *joystick);

#ifdef __cplusplus
}
#endif
#endif /* _SDL_JOYSTICK_H */
