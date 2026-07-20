/*
 * MiaSdlApi — ABI contract between host firmware and libmia_sdl_v1.so.
 *
 * Host firmware (e.g. lavapal.bin) dlopens libmia_sdl_v1.so and obtains a
 * const MiaSdlApi* via mia_sdl_get_api(). A thin wrapper layer inside the
 * firmware forwards each SDL_* call to the corresponding function pointer
 * so SDLPAL source code can call SDL_* by name without modification.
 *
 * Both sides include the SDL 1.2 compatibility headers from
 * shared_libraries/mia_sdl/include/SDL/, so the function-pointer signatures
 * reference the real SDL_* types directly.
 *
 * Versioning: bump MIA_SDL_ABI_VERSION when any signature changes.
 */
#ifndef MIA_SDL_API_H
#define MIA_SDL_API_H

#include "SDL/SDL.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIA_SDL_ABI_VERSION 1u
#define MIA_SDL_LIBRARY_NAME "libmia_sdl_v1.so"
#define MIA_SDL_LIBRARY_PATH "/sd/MiaOS/Library/libmia_sdl_v1.so"

typedef struct MiaSdlApi {
	uint32_t abi_version;
	uint32_t struct_size;

	/* --- init / quit --- */
	int            (*init)(Uint32 flags);
	void           (*quit)(void);
	int            (*init_subsystem)(Uint32 flags);
	void           (*quit_subsystem)(Uint32 flags);
	Uint32         (*was_init)(Uint32 flags);
	const SDL_version *(*linked_version)(void);

	/* --- error --- */
	void           (*set_error)(const char *fmt, ...);
	char          *(*get_error)(void);
	void           (*clear_error)(void);

	/* --- timer --- */
	Uint32         (*get_ticks)(void);
	void           (*delay)(Uint32 ms);
	Uint64         (*get_performance_counter)(void);
	Uint64         (*get_performance_frequency)(void);

	/* --- video --- */
	const SDL_VideoInfo *(*get_video_info)(void);
	SDL_Rect     **(*list_modes)(SDL_PixelFormat *format, Uint32 flags);
	int            (*video_mode_ok)(int width, int height, int bpp, Uint32 flags);
	SDL_Surface  *(*set_video_mode)(int width, int height, int bpp, Uint32 flags);
	void           (*update_rect)(SDL_Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h);
	void           (*update_rects)(SDL_Surface *screen, int numrects, SDL_Rect *rects);
	int            (*flip)(SDL_Surface *screen);
	int            (*set_gamma)(float r, float g, float b);
	int            (*set_gamma_ramp)(const Uint16 *r, const Uint16 *g, const Uint16 *b);
	int            (*get_gamma_ramp)(Uint16 *r, Uint16 *g, Uint16 *b);
	void           (*wm_set_caption)(const char *title, const char *icon);
	void           (*wm_get_caption)(char **title, char **icon);
	void           (*wm_set_icon)(SDL_Surface *icon, Uint8 *mask);
	int            (*wm_iconify_window)(void);
	int            (*wm_toggle_full_screen)(SDL_Surface *surface);
	int            (*show_cursor)(int toggle);

	/* --- pixels --- */
	Uint32         (*map_rgb)(const SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b);
	Uint32         (*map_rgba)(const SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	void           (*get_rgb)(Uint32 pixel, const SDL_PixelFormat *fmt, Uint8 *r, Uint8 *g, Uint8 *b);
	void           (*get_rgba)(Uint32 pixel, const SDL_PixelFormat *fmt, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a);
	SDL_Palette   *(*alloc_palette)(int ncolors);
	void           (*free_palette)(SDL_Palette *palette);
	int            (*set_palette_colors)(SDL_Palette *palette, const SDL_Color *colors, int firstcolor, int ncolors);

	/* --- surface --- */
	SDL_Surface  *(*create_rgb_surface)(Uint32 flags, int width, int height, int depth,
	                                    Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
	SDL_Surface  *(*create_rgb_surface_from)(void *pixels, int width, int height, int depth,
	                                         int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
	SDL_Surface  *(*display_format)(SDL_Surface *surface);
	SDL_Surface  *(*display_format_alpha)(SDL_Surface *surface);
	void           (*free_surface)(SDL_Surface *surface);
	int            (*lock_surface)(SDL_Surface *surface);
	void           (*unlock_surface)(SDL_Surface *surface);
	int            (*set_palette)(SDL_Surface *surface, int flags, const SDL_Color *colors, int firstcolor, int ncolors);
	int            (*set_colors)(SDL_Surface *surface, const SDL_Color *colors, int firstcolor, int ncolors);
	int            (*set_color_key)(SDL_Surface *surface, Uint32 flag, Uint32 key);
	int            (*set_alpha)(SDL_Surface *surface, Uint32 flag, Uint8 alpha);
	int            (*set_surface_palette)(SDL_Surface *surface, SDL_Palette *palette);
	int            (*set_surface_color_mod)(SDL_Surface *surface, Uint8 r, Uint8 g, Uint8 b);
	int            (*fill_rect)(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color);
	int            (*blit_surface)(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
	int            (*blit_scaled)(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
	int            (*soft_stretch)(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, const SDL_Rect *dstrect);
	int            (*upper_blit)(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
	int            (*lower_blit)(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
	SDL_Surface  *(*convert_surface)(SDL_Surface *src, const SDL_PixelFormat *fmt, Uint32 flags);
	SDL_Surface  *(*load_bmp_rw)(SDL_RWops *src, int freesrc);
	int            (*save_bmp_rw)(SDL_Surface *surface, SDL_RWops *dst, int freedst);

	/* --- events --- */
	void           (*pump_events)(void);
	int            (*poll_event)(SDL_Event *event);
	int            (*wait_event)(SDL_Event *event);
	int            (*push_event)(SDL_Event *event);
	int            (*peep_events)(SDL_Event *events, int numevents, int action, Uint32 mask);
	void           (*set_event_filter)(SDL_EventFilter filter);
	SDL_EventFilter (*get_event_filter)(void);
	Uint8          (*event_state)(Uint8 type, int state);

	/* --- keyboard --- */
	Uint8         *(*get_key_state)(int *numkeys);
	SDLMod         (*get_mod_state)(void);
	void           (*set_mod_state)(SDLMod modstate);
	char          *(*get_key_name)(SDLKey key);
	int            (*enable_unicode)(int enable);
	int            (*enable_key_repeat)(int delay, int interval);
	const Uint8  *(*get_keyboard_state)(int *numkeys);
	int            (*get_scancode_from_key)(SDLKey key);

	/* --- mouse --- */
	Uint8          (*get_mouse_state)(int *x, int *y);
	Uint8          (*get_relative_mouse_state)(int *x, int *y);
	void           (*warp_mouse)(Uint16 x, Uint16 y);
	SDL_Cursor   *(*create_cursor)(Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y);
	void           (*set_cursor)(SDL_Cursor *cursor);
	SDL_Cursor   *(*get_cursor)(void);
	void           (*free_cursor)(SDL_Cursor *cursor);

	/* --- joystick --- */
	int            (*num_joysticks)(void);
	const char    *(*joystick_name)(int device_index);
	const char    *(*joystick_name_for_index)(int device_index);
	SDL_Joystick  *(*joystick_open)(int device_index);
	void           (*joystick_close)(SDL_Joystick *joystick);
	int            (*joystick_event_state)(int state);
	int            (*joystick_get_axis)(SDL_Joystick *joystick, int axis);
	Uint8          (*joystick_get_hat)(SDL_Joystick *joystick, int hat);
	int            (*joystick_get_ball)(SDL_Joystick *joystick, int ball, int *dx, int *dy);
	Uint8          (*joystick_get_button)(SDL_Joystick *joystick, int button);
	int            (*joystick_num_axes)(SDL_Joystick *joystick);
	int            (*joystick_num_balls)(SDL_Joystick *joystick);
	int            (*joystick_num_hats)(SDL_Joystick *joystick);
	int            (*joystick_num_buttons)(SDL_Joystick *joystick);
	void           (*joystick_update)(void);

	/* --- audio --- */
	int            (*audio_init)(const char *driver_name);
	void           (*audio_quit)(void);
	int            (*open_audio)(SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
	void           (*close_audio)(void);
	void           (*pause_audio)(int pause_on);
	void           (*lock_audio)(void);
	void           (*unlock_audio)(void);
	int            (*build_audio_cvt)(SDL_AudioCVT *cvt, SDL_AudioFormat src_format, Uint8 src_channels, int src_rate,
	                                  SDL_AudioFormat dst_format, Uint8 dst_channels, int dst_rate);
	int            (*convert_audio)(SDL_AudioCVT *cvt);
	void           (*mix_audio)(Uint8 *dst, const Uint8 *src, Uint32 len, int volume);

	/* --- rwops --- */
	SDL_RWops    *(*rw_from_file)(const char *file, const char *mode);
	SDL_RWops    *(*rw_from_mem)(void *data, int size);
	SDL_RWops    *(*rw_from_const_mem)(const void *data, int size);
	SDL_RWops    *(*alloc_rw)(void);
	void          (*free_rw)(SDL_RWops *area);

	/* --- mutex --- */
	SDL_mutex    *(*create_mutex)(void);
	int            (*lock_mutex)(SDL_mutex *mutex);
	int            (*try_lock_mutex)(SDL_mutex *mutex);
	int            (*unlock_mutex)(SDL_mutex *mutex);
	void           (*destroy_mutex)(SDL_mutex *mutex);
	SDL_sem      *(*create_semaphore)(Uint32 initial_value);
	void           (*destroy_semaphore)(SDL_sem *sem);
	int            (*sem_wait)(SDL_sem *sem);
	int            (*sem_try_wait)(SDL_sem *sem);
	int            (*sem_wait_timeout)(SDL_sem *sem, Uint32 ms);
	int            (*sem_post)(SDL_sem *sem);
	Uint32         (*sem_value)(SDL_sem *sem);
	SDL_cond     *(*create_cond)(void);
	void           (*destroy_cond)(SDL_cond *cond);
	int            (*cond_signal)(SDL_cond *cond);
	int            (*cond_broadcast)(SDL_cond *cond);
	int            (*cond_wait)(SDL_cond *cond, SDL_mutex *mutex);
	int            (*cond_wait_timeout)(SDL_cond *cond, SDL_mutex *mutex, Uint32 ms);

	/* --- thread --- */
	SDL_Thread   *(*create_thread)(SDL_ThreadFunction fn, const char *name, void *data);
	void           (*wait_thread)(SDL_Thread *thread, int *status);
	unsigned long  (*thread_id)(void);

	/* --- active / syswm / loadso / cpuinfo --- */
	Uint8          (*get_app_state)(void);
	int            (*get_wm_info)(SDL_SysWMinfo *info);
	void          *(*load_object)(const char *sofile);
	void          *(*load_function)(void *handle, const char *name);
	void           (*unload_object)(void *handle);
} MiaSdlApi;

typedef const MiaSdlApi *(*MiaSdlGetApiFn)(Uint32 requested_version);

/* Implemented by libmia_sdl_v1.so; declared here so both sides share it. */
const MiaSdlApi *mia_sdl_get_api(Uint32 requested_version);

#ifdef __cplusplus
}
#endif
#endif /* MIA_SDL_API_H */
