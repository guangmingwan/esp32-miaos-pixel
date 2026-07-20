/*
 * sdl_wrapper.c — forwarding layer that turns SDLPAL's direct SDL_* calls
 * into indirect dispatch through the MiaSdlApi function table provided by
 * libmia_sdl_v1.so. app_main.c loads the .so and calls mia_sdl_set_api()
 * before entering SDLPAL's main().
 */
#include "SDL/SDL.h"
#include "mia_sdl_api.h"

#include <stdarg.h>

const MiaSdlApi *g_sdl_api = NULL;

void mia_sdl_set_api(const MiaSdlApi *api) { g_sdl_api = api; }

/* init / quit */
int   SDL_Init(Uint32 f)                                          { return g_sdl_api->init(f); }
void  SDL_Quit(void)                                              { g_sdl_api->quit(); }
int   SDL_InitSubSystem(Uint32 f)                                { return g_sdl_api->init_subsystem(f); }
void  SDL_QuitSubSystem(Uint32 f)                                { g_sdl_api->quit_subsystem(f); }
Uint32 SDL_WasInit(Uint32 f)                                     { return g_sdl_api->was_init(f); }
const SDL_version *SDL_Linked_Version(void)                      { return g_sdl_api->linked_version(); }

/* error */
void  SDL_SetError(const char *fmt, ...)                          { va_list a; va_start(a,fmt); g_sdl_api->set_error(fmt,a); va_end(a); }
char *SDL_GetError(void)                                         { return g_sdl_api->get_error(); }
void  SDL_ClearError(void)                                       { g_sdl_api->clear_error(); }

/* timer */
Uint32 SDL_GetTicks(void)                                        { return g_sdl_api->get_ticks(); }
void   SDL_Delay(Uint32 ms)                                      { g_sdl_api->delay(ms); }
Uint64 SDL_GetPerformanceCounter(void)                           { return g_sdl_api->get_performance_counter(); }
Uint64 SDL_GetPerformanceFrequency(void)                         { return g_sdl_api->get_performance_frequency(); }

/* video */
const SDL_VideoInfo *SDL_GetVideoInfo(void)                      { return g_sdl_api->get_video_info(); }
SDL_Rect **SDL_ListModes(SDL_PixelFormat *f, Uint32 fl)          { return g_sdl_api->list_modes(f, fl); }
int    SDL_VideoModeOK(int w, int h, int bpp, Uint32 fl)         { return g_sdl_api->video_mode_ok(w, h, bpp, fl); }
SDL_Surface *SDL_SetVideoMode(int w, int h, int bpp, Uint32 fl)  { return g_sdl_api->set_video_mode(w, h, bpp, fl); }
void   SDL_UpdateRect(SDL_Surface *s, Sint32 x, Sint32 y, Uint32 w, Uint32 h) { g_sdl_api->update_rect(s,x,y,w,h); }
void   SDL_UpdateRects(SDL_Surface *s, int n, SDL_Rect *r)       { g_sdl_api->update_rects(s,n,r); }
int    SDL_Flip(SDL_Surface *s)                                  { return g_sdl_api->flip(s); }
int    SDL_SetGamma(float r, float g, float b)                   { return g_sdl_api->set_gamma(r,g,b); }
int    SDL_SetGammaRamp(const Uint16 *r, const Uint16 *g, const Uint16 *b) { return g_sdl_api->set_gamma_ramp(r,g,b); }
int    SDL_GetGammaRamp(Uint16 *r, Uint16 *g, Uint16 *b)        { return g_sdl_api->get_gamma_ramp(r,g,b); }
void   SDL_WM_SetCaption(const char *t, const char *i)           { g_sdl_api->wm_set_caption(t,i); }
void   SDL_WM_GetCaption(char **t, char **i)                    { g_sdl_api->wm_get_caption(t,i); }
void   SDL_WM_SetIcon(SDL_Surface *ic, Uint8 *m)                { g_sdl_api->wm_set_icon(ic,m); }
int    SDL_WM_IconifyWindow(void)                               { return g_sdl_api->wm_iconify_window(); }
int    SDL_WM_ToggleFullScreen(SDL_Surface *s)                  { return g_sdl_api->wm_toggle_full_screen(s); }
int    SDL_ShowCursor(int t)                                    { return g_sdl_api->show_cursor(t); }

/* pixels */
Uint32 SDL_MapRGB(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b) { return g_sdl_api->map_rgb(f,r,g,b); }
Uint32 SDL_MapRGBA(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b, Uint8 a) { return g_sdl_api->map_rgba(f,r,g,b,a); }
void   SDL_GetRGB(Uint32 p, const SDL_PixelFormat *f, Uint8 *r, Uint8 *g, Uint8 *b) { g_sdl_api->get_rgb(p,f,r,g,b); }
void   SDL_GetRGBA(Uint32 p, const SDL_PixelFormat *f, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a) { g_sdl_api->get_rgba(p,f,r,g,b,a); }
SDL_Palette *SDL_AllocPalette(int n)                            { return g_sdl_api->alloc_palette(n); }
void   SDL_FreePalette(SDL_Palette *p)                          { g_sdl_api->free_palette(p); }
int    SDL_SetPaletteColors(SDL_Palette *p, const SDL_Color *c, int f, int n) { return g_sdl_api->set_palette_colors(p,c,f,n); }

/* surface */
SDL_Surface *SDL_CreateRGBSurface(Uint32 fl, int w, int h, int d, Uint32 R, Uint32 G, Uint32 B, Uint32 A) { return g_sdl_api->create_rgb_surface(fl,w,h,d,R,G,B,A); }
SDL_Surface *SDL_CreateRGBSurfaceFrom(void *px, int w, int h, int d, int p, Uint32 R, Uint32 G, Uint32 B, Uint32 A) { return g_sdl_api->create_rgb_surface_from(px,w,h,d,p,R,G,B,A); }
SDL_Surface *SDL_DisplayFormat(SDL_Surface *s)                 { return g_sdl_api->display_format(s); }
SDL_Surface *SDL_DisplayFormatAlpha(SDL_Surface *s)            { return g_sdl_api->display_format_alpha(s); }
void   SDL_FreeSurface(SDL_Surface *s)                         { g_sdl_api->free_surface(s); }
int    SDL_LockSurface(SDL_Surface *s)                         { return g_sdl_api->lock_surface(s); }
void   SDL_UnlockSurface(SDL_Surface *s)                       { g_sdl_api->unlock_surface(s); }
int    SDL_SetPalette(SDL_Surface *s, int fl, const SDL_Color *c, int f, int n) { return g_sdl_api->set_palette(s,fl,c,f,n); }
int    SDL_SetColors(SDL_Surface *s, const SDL_Color *c, int f, int n) { return g_sdl_api->set_colors(s,c,f,n); }
int    SDL_SetColorKey(SDL_Surface *s, Uint32 fl, Uint32 k)    { return g_sdl_api->set_color_key(s,fl,k); }
int    SDL_SetAlpha(SDL_Surface *s, Uint32 fl, Uint8 a)        { return g_sdl_api->set_alpha(s,fl,a); }
int    SDL_SetSurfacePalette(SDL_Surface *s, SDL_Palette *p)   { return g_sdl_api->set_surface_palette(s,p); }
int    SDL_SetSurfaceColorMod(SDL_Surface *s, Uint8 r, Uint8 g, Uint8 b) { return g_sdl_api->set_surface_color_mod(s,r,g,b); }
int    SDL_FillRect(SDL_Surface *d, const SDL_Rect *r, Uint32 c) { return g_sdl_api->fill_rect(d,r,c); }
int    SDL_BlitSurface(SDL_Surface *s, const SDL_Rect *sr, SDL_Surface *d, SDL_Rect *dr) { return g_sdl_api->blit_surface(s,sr,d,dr); }
int    SDL_BlitScaled(SDL_Surface *s, const SDL_Rect *sr, SDL_Surface *d, SDL_Rect *dr) { return g_sdl_api->blit_scaled(s,sr,d,dr); }
int    SDL_SoftStretch(SDL_Surface *s, const SDL_Rect *sr, SDL_Surface *d, const SDL_Rect *dr) { return g_sdl_api->soft_stretch(s,sr,d,dr); }
int    SDL_UpperBlit(SDL_Surface *s, const SDL_Rect *sr, SDL_Surface *d, SDL_Rect *dr) { return g_sdl_api->upper_blit(s,sr,d,dr); }
int    SDL_LowerBlit(SDL_Surface *s, SDL_Rect *sr, SDL_Surface *d, SDL_Rect *dr) { return g_sdl_api->lower_blit(s,sr,d,dr); }
SDL_Surface *SDL_ConvertSurface(SDL_Surface *s, const SDL_PixelFormat *f, Uint32 fl) { return g_sdl_api->convert_surface(s,f,fl); }
SDL_Surface *SDL_LoadBMP_RW(SDL_RWops *src, int fs)            { return g_sdl_api->load_bmp_rw(src,fs); }
int    SDL_SaveBMP_RW(SDL_Surface *s, SDL_RWops *d, int fd)    { return g_sdl_api->save_bmp_rw(s,d,fd); }

/* events */
void   SDL_PumpEvents(void)                                     { g_sdl_api->pump_events(); }
int    SDL_PollEvent(SDL_Event *e)                              { return g_sdl_api->poll_event(e); }
int    SDL_WaitEvent(SDL_Event *e)                              { return g_sdl_api->wait_event(e); }
int    SDL_PushEvent(SDL_Event *e)                              { return g_sdl_api->push_event(e); }
int    SDL_PeepEvents(SDL_Event *e, int n, int a, Uint32 m)    { return g_sdl_api->peep_events(e,n,a,m); }
void   SDL_SetEventFilter(SDL_EventFilter f)                   { g_sdl_api->set_event_filter(f); }
SDL_EventFilter SDL_GetEventFilter(void)                       { return g_sdl_api->get_event_filter(); }
Uint8  SDL_EventState(Uint8 t, int s)                          { return g_sdl_api->event_state(t,s); }

/* keyboard */
Uint8 *SDL_GetKeyState(int *n)                                  { return g_sdl_api->get_key_state(n); }
SDLMod SDL_GetModState(void)                                    { return g_sdl_api->get_mod_state(); }
void   SDL_SetModState(SDLMod m)                                { g_sdl_api->set_mod_state(m); }
char  *SDL_GetKeyName(SDLKey k)                                 { return g_sdl_api->get_key_name(k); }
int    SDL_EnableUNICODE(int e)                                 { return g_sdl_api->enable_unicode(e); }
int    SDL_EnableKeyRepeat(int d, int i)                       { return g_sdl_api->enable_key_repeat(d,i); }
const Uint8 *SDL_GetKeyboardState(int *n)                      { return g_sdl_api->get_keyboard_state(n); }
int    SDL_GetScancodeFromKey(SDLKey k)                        { return g_sdl_api->get_scancode_from_key(k); }

/* mouse */
Uint8  SDL_GetMouseState(int *x, int *y)                       { return g_sdl_api->get_mouse_state(x,y); }
Uint8  SDL_GetRelativeMouseState(int *x, int *y)               { return g_sdl_api->get_relative_mouse_state(x,y); }
void   SDL_WarpMouse(Uint16 x, Uint16 y)                       { g_sdl_api->warp_mouse(x,y); }
SDL_Cursor *SDL_CreateCursor(Uint8 *d, Uint8 *m, int w, int h, int hx, int hy) { return g_sdl_api->create_cursor(d,m,w,h,hx,hy); }
void   SDL_SetCursor(SDL_Cursor *c)                            { g_sdl_api->set_cursor(c); }
SDL_Cursor *SDL_GetCursor(void)                                { return g_sdl_api->get_cursor(); }
void   SDL_FreeCursor(SDL_Cursor *c)                           { g_sdl_api->free_cursor(c); }

/* joystick */
int    SDL_NumJoysticks(void)                                   { return g_sdl_api->num_joysticks(); }
const char *SDL_JoystickName(int i)                             { return g_sdl_api->joystick_name(i); }
const char *SDL_JoystickNameForIndex(int i)                    { return g_sdl_api->joystick_name_for_index(i); }
SDL_Joystick *SDL_JoystickOpen(int i)                          { return g_sdl_api->joystick_open(i); }
void   SDL_JoystickClose(SDL_Joystick *j)                      { g_sdl_api->joystick_close(j); }
int    SDL_JoystickEventState(int s)                           { return g_sdl_api->joystick_event_state(s); }
int    SDL_JoystickGetAxis(SDL_Joystick *j, int a)             { return g_sdl_api->joystick_get_axis(j,a); }
Uint8  SDL_JoystickGetHat(SDL_Joystick *j, int h)              { return g_sdl_api->joystick_get_hat(j,h); }
int    SDL_JoystickGetBall(SDL_Joystick *j, int b, int *dx, int *dy) { return g_sdl_api->joystick_get_ball(j,b,dx,dy); }
Uint8  SDL_JoystickGetButton(SDL_Joystick *j, int b)           { return g_sdl_api->joystick_get_button(j,b); }
int    SDL_JoystickNumAxes(SDL_Joystick *j)                    { return g_sdl_api->joystick_num_axes(j); }
int    SDL_JoystickNumBalls(SDL_Joystick *j)                   { return g_sdl_api->joystick_num_balls(j); }
int    SDL_JoystickNumHats(SDL_Joystick *j)                    { return g_sdl_api->joystick_num_hats(j); }
int    SDL_JoystickNumButtons(SDL_Joystick *j)                 { return g_sdl_api->joystick_num_buttons(j); }
void   SDL_JoystickUpdate(void)                                { g_sdl_api->joystick_update(); }

/* audio */
int    SDL_AudioInit(const char *n)                             { return g_sdl_api->audio_init(n); }
void   SDL_AudioQuit(void)                                      { g_sdl_api->audio_quit(); }
int    SDL_OpenAudio(SDL_AudioSpec *d, SDL_AudioSpec *o)       { return g_sdl_api->open_audio(d,o); }
void   SDL_CloseAudio(void)                                     { g_sdl_api->close_audio(); }
void   SDL_PauseAudio(int p)                                    { g_sdl_api->pause_audio(p); }
void   SDL_LockAudio(void)                                      { g_sdl_api->lock_audio(); }
void   SDL_UnlockAudio(void)                                    { g_sdl_api->unlock_audio(); }
int    SDL_BuildAudioCVT(SDL_AudioCVT *c, SDL_AudioFormat sf, Uint8 sc, int sr, SDL_AudioFormat df, Uint8 dc, int dr) { return g_sdl_api->build_audio_cvt(c,sf,sc,sr,df,dc,dr); }
int    SDL_ConvertAudio(SDL_AudioCVT *c)                       { return g_sdl_api->convert_audio(c); }
void   SDL_MixAudio(Uint8 *d, const Uint8 *s, Uint32 l, int v) { g_sdl_api->mix_audio(d,s,l,v); }

/* rwops */
SDL_RWops *SDL_RWFromFile(const char *f, const char *m)        { return g_sdl_api->rw_from_file(f,m); }
SDL_RWops *SDL_RWFromMem(void *d, int s)                       { return g_sdl_api->rw_from_mem(d,s); }
SDL_RWops *SDL_RWFromConstMem(const void *d, int s)            { return g_sdl_api->rw_from_const_mem(d,s); }
SDL_RWops *SDL_AllocRW(void)                                   { return g_sdl_api->alloc_rw(); }
void       SDL_FreeRW(SDL_RWops *rw)                           { g_sdl_api->free_rw(rw); }

/* mutex */
SDL_mutex *SDL_CreateMutex(void)                               { return g_sdl_api->create_mutex(); }
int    SDL_LockMutex(SDL_mutex *m)                             { return g_sdl_api->lock_mutex(m); }
int    SDL_TryLockMutex(SDL_mutex *m)                          { return g_sdl_api->try_lock_mutex(m); }
int    SDL_UnlockMutex(SDL_mutex *m)                           { return g_sdl_api->unlock_mutex(m); }
void   SDL_DestroyMutex(SDL_mutex *m)                          { g_sdl_api->destroy_mutex(m); }
SDL_sem *SDL_CreateSemaphore(Uint32 v)                        { return g_sdl_api->create_semaphore(v); }
void   SDL_DestroySemaphore(SDL_sem *s)                        { g_sdl_api->destroy_semaphore(s); }
int    SDL_SemWait(SDL_sem *s)                                { return g_sdl_api->sem_wait(s); }
int    SDL_SemTryWait(SDL_sem *s)                             { return g_sdl_api->sem_try_wait(s); }
int    SDL_SemWaitTimeout(SDL_sem *s, Uint32 ms)              { return g_sdl_api->sem_wait_timeout(s,ms); }
int    SDL_SemPost(SDL_sem *s)                                { return g_sdl_api->sem_post(s); }
Uint32 SDL_SemValue(SDL_sem *s)                               { return g_sdl_api->sem_value(s); }
SDL_cond *SDL_CreateCond(void)                                { return g_sdl_api->create_cond(); }
void   SDL_DestroyCond(SDL_cond *c)                           { g_sdl_api->destroy_cond(c); }
int    SDL_CondSignal(SDL_cond *c)                            { return g_sdl_api->cond_signal(c); }
int    SDL_CondBroadcast(SDL_cond *c)                         { return g_sdl_api->cond_broadcast(c); }
int    SDL_CondWait(SDL_cond *c, SDL_mutex *m)               { return g_sdl_api->cond_wait(c,m); }
int    SDL_CondWaitTimeout(SDL_cond *c, SDL_mutex *m, Uint32 ms) { return g_sdl_api->cond_wait_timeout(c,m,ms); }

/* thread */
SDL_Thread *SDL_CreateThread(SDL_ThreadFunction fn, const char *n, void *d) { return g_sdl_api->create_thread(fn,n,d); }
void   SDL_WaitThread(SDL_Thread *t, int *s)                   { g_sdl_api->wait_thread(t,s); }
unsigned long SDL_ThreadID(void)                              { return g_sdl_api->thread_id(); }

/* active / syswm / loadso */
Uint8  SDL_GetAppState(void)                                   { return g_sdl_api->get_app_state(); }
int    SDL_GetWMInfo(SDL_SysWMinfo *i)                         { return g_sdl_api->get_wm_info(i); }
void  *SDL_LoadObject(const char *f)                           { return g_sdl_api->load_object(f); }
void  *SDL_LoadFunction(void *h, const char *n)                { return g_sdl_api->load_function(h,n); }
void   SDL_UnloadObject(void *h)                               { g_sdl_api->unload_object(h); }
