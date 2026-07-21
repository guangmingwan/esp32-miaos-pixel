/*
 * libmia_sdl_v1.so — SDL 1.2 compatibility implementation for ESP32-S3.
 *
 * Provides the MiaSdlApi function table consumed by host firmware (e.g.
 * lavapal.bin). Core surface/palette/timer/mutex/rwops/video-present are
 * implemented; subsystems the SDLPAL port does not need (joystick, cdrom,
 * thread, loadso, gamma) return success stubs so SDLPAL's conditional
 * code still links.
 */
#include "SDL/SDL.h"
#include "mia_host_abi.h"
#include "mia_sdl_api.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <time.h>

/* ----------------------------------------------------------------- error */
static char g_error[128];
static void  mia_set_error(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vsnprintf(g_error, sizeof(g_error), fmt, ap); va_end(ap); }
static char *mia_get_error(void) { return g_error; }
static void  mia_clear_error(void) { g_error[0] = 0; }

/* ----------------------------------------------------------------- timer */
static Uint32 mia_get_ticks(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (Uint32)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000); }
static void   mia_delay(Uint32 ms) { usleep((useconds_t)ms * 1000); }
static Uint64 mia_perf_counter(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (Uint64)ts.tv_sec * 1000000ULL + (Uint64)ts.tv_nsec / 1000; }
static Uint64 mia_perf_freq(void) { return 1000000ULL; }

/* ---------------------------------------------------------------- surface
 * SDL_Surface is allocated from PSRAM-capable heap. pixels buffer follows
 * the struct. format/palette are embedded so SDLPAL's ->format->palette
 * traversal works without extra indirection. */
static SDL_PixelFormat g_indexed8_format;
static SDL_Palette     g_default_palette;

static void mia_init_default_format(void) {
	static SDL_Color black[256];
	if (g_default_palette.colors == NULL) {
		g_default_palette.ncolors = 256;
		g_default_palette.colors  = black;
		g_indexed8_format.BitsPerPixel = 8;
		g_indexed8_format.BytesPerPixel = 1;
		g_indexed8_format.palette = &g_default_palette;
	}
}

static SDL_Surface *mia_create_rgb_surface(Uint32 flags, int w, int h, int depth,
                                           Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask) {
	(void)flags; (void)Rmask; (void)Gmask; (void)Bmask; (void)Amask;
	if (w <= 0 || h <= 0 || depth <= 0) return NULL;
	mia_init_default_format();
	SDL_PixelFormat *fmt = calloc(1, sizeof(SDL_PixelFormat));
	if (!fmt) return NULL;
	*fmt = g_indexed8_format;
	fmt->BitsPerPixel = (Uint8)depth;
	fmt->BytesPerPixel = (Uint8)((depth + 7) / 8);
	int pitch = w * fmt->BytesPerPixel;
	size_t pix_bytes = (size_t)pitch * h;
	SDL_Surface *s = calloc(1, sizeof(SDL_Surface));
	if (!s) { free(fmt); return NULL; }
	s->pixels = calloc(1, pix_bytes ? pix_bytes : 1);
	if (!s->pixels) { free(s); free(fmt); return NULL; }
	s->flags = flags;
	s->format = fmt;
	s->w = w; s->h = h;
	s->pitch = (Uint16)pitch;
	s->refcount = 1;
	s->clip_rect.x = 0; s->clip_rect.y = 0;
	s->clip_rect.w = (Uint16)w; s->clip_rect.h = (Uint16)h;
	return s;
}

static SDL_Surface *mia_create_rgb_surface_from(void *pixels, int w, int h, int depth,
                                                int pitch, Uint32 R, Uint32 G, Uint32 B, Uint32 A) {
	SDL_Surface *s = mia_create_rgb_surface(0, w, h, depth, R, G, B, A);
	if (!s) return NULL;
	free(s->pixels);
	s->pixels = pixels;
	s->pitch = (Uint16)pitch;
	s->flags |= SDL_PREALLOC;
	return s;
}

static void mia_free_surface(SDL_Surface *s) {
	if (!s) return;
	if (s->format) free(s->format);
	if (s->pixels && !(s->flags & SDL_PREALLOC)) free(s->pixels);
	free(s);
}
static int  mia_lock_surface(SDL_Surface *s) { (void)s; return 0; }
static void mia_unlock_surface(SDL_Surface *s) { (void)s; }

static SDL_Surface *mia_display_format(SDL_Surface *s) { if (s) s->refcount++; return s; }
static SDL_Surface *mia_display_format_alpha(SDL_Surface *s) { if (s) s->refcount++; return s; }
static SDL_Surface *mia_convert_surface(SDL_Surface *src, const SDL_PixelFormat *fmt, Uint32 flags) {
	(void)fmt; (void)flags;
	if (!src) return NULL;
	return mia_create_rgb_surface(src->flags, src->w, src->h, src->format ? src->format->BitsPerPixel : 8, 0,0,0,0);
}

/* ----------------------------------------------------------------- pixels */
static SDL_Palette *mia_alloc_palette(int ncolors) {
	SDL_Palette *p = calloc(1, sizeof(SDL_Palette));
	if (!p) return NULL;
	p->ncolors = ncolors;
	p->colors = calloc(ncolors ? ncolors : 1, sizeof(SDL_Color));
	if (!p->colors) { free(p); return NULL; }
	return p;
}
static void mia_free_palette(SDL_Palette *p) { if (!p) return; if (p->colors) free(p->colors); free(p); }
static int  mia_set_palette_colors(SDL_Palette *p, const SDL_Color *c, int first, int n) {
	if (!p || !c) return -1;
	for (int i = 0; i < n && first + i < p->ncolors; i++) p->colors[first + i] = c[i];
	return 0;
}
static Uint32 mia_map_rgb(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b) {
	if (f && f->BitsPerPixel == 8) return 0;
	return ((Uint32)r << 16) | ((Uint32)g << 8) | b;
}
static Uint32 mia_map_rgba(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	if (f && f->BitsPerPixel == 8) return 0;
	return ((Uint32)a << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | b;
}
static void mia_get_rgb(Uint32 px, const SDL_PixelFormat *f, Uint8 *r, Uint8 *g, Uint8 *b) {
	(void)f; *r = (px >> 16) & 0xff; *g = (px >> 8) & 0xff; *b = px & 0xff;
}
static void mia_get_rgba(Uint32 px, const SDL_PixelFormat *f, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a) {
	(void)f; *a = (px >> 24) & 0xff; *r = (px >> 16) & 0xff; *g = (px >> 8) & 0xff; *b = px & 0xff;
}
static int mia_set_palette(SDL_Surface *s, int flags, const SDL_Color *c, int first, int n) {
	(void)flags; return mia_set_palette_colors(s && s->format ? s->format->palette : NULL, c, first, n);
}
static int mia_set_colors(SDL_Surface *s, const SDL_Color *c, int first, int n) {
	return mia_set_palette_colors(s && s->format ? s->format->palette : NULL, c, first, n);
}
static int mia_set_color_key(SDL_Surface *s, Uint32 f, Uint32 k) { if (s&&s->format) s->format->colorkey=k; (void)f; return 0; }
static int mia_set_alpha(SDL_Surface *s, Uint32 f, Uint8 a) { if (s&&s->format) s->format->alpha=a; (void)f; return 0; }
static int mia_set_surface_palette(SDL_Surface *s, SDL_Palette *p) { if (s&&s->format) s->format->palette=p; return 0; }
static int mia_set_surface_color_mod(SDL_Surface *s, Uint8 r, Uint8 g, Uint8 b) { (void)s; (void)r; (void)g; (void)b; return 0; }

/* ----------------------------------------------------------------- blit/fill
 * Minimal direct-buffer blit/fill. Handles 8bpp and 32bpp rectangular
 * copies with clip-clamping. SDL_BlitScaled/SoftStretch fall back to a
 * nearest-neighbour 1:1 copy (good enough for the SDLPAL title path). */
static int rect_clip(const SDL_Rect *src, SDL_Surface *dst, SDL_Rect *dstrect, SDL_Rect *cs, SDL_Rect *cd) {
	SDL_Rect d = dstrect ? *dstrect : (SDL_Rect){0,0,(Uint16)dst->w,(Uint16)dst->h};
	if (d.x < 0) { cs->x = -d.x; cs->w = src->w + d.x; d.x = 0; } else { cs->x = src->x; cs->w = src->w; }
	if (d.y < 0) { cs->y = -d.y; cs->h = src->h + d.y; d.y = 0; } else { cs->y = src->y; cs->h = src->h; }
	if (d.x + cs->w > dst->w) cs->w = dst->w - d.x;
	if (d.y + cs->h > dst->h) cs->h = dst->h - d.y;
	if (cs->w <= 0 || cs->h <= 0) return -1;
	*cd = d; cd->w = cs->w; cd->h = cs->h;
	return 0;
}

static int mia_blit(SDL_Surface *src, const SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
	if (!src || !dst) return -1;
	SDL_Rect sr = srcrect ? *srcrect : (SDL_Rect){0,0,(Uint16)src->w,(Uint16)src->h};
	SDL_Rect cs, cd;
	if (rect_clip(&sr, dst, dstrect, &cs, &cd) != 0) return 0;
	int bpp = src->format && dst->format ? (src->format->BytesPerPixel < dst->format->BytesPerPixel ? src->format->BytesPerPixel : dst->format->BytesPerPixel) : 1;
	if (bpp <= 0) bpp = 1;
	for (int y = 0; y < cs.h; y++) {
		const uint8_t *s = (const uint8_t*)src->pixels + (cs.y + y) * src->pitch + cs.x * bpp;
		uint8_t *d = (uint8_t*)dst->pixels + (cd.y + y) * dst->pitch + cd.x * bpp;
		memcpy(d, s, (size_t)cs.w * bpp);
	}
	if (dstrect) *dstrect = cd;
	return 0;
}

static int mia_fill_rect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color) {
	if (!dst) return -1;
	SDL_Rect r = rect ? *rect : (SDL_Rect){0,0,(Uint16)dst->w,(Uint16)dst->h};
	if (r.x < 0) { r.w += r.x; r.x = 0; }
	if (r.y < 0) { r.h += r.y; r.y = 0; }
	if (r.x + r.w > dst->w) r.w = dst->w - r.x;
	if (r.y + r.h > dst->h) r.h = dst->h - r.y;
	if (r.w <= 0 || r.h <= 0) return 0;
	int bpp = dst->format ? dst->format->BytesPerPixel : 1;
	if (bpp <= 0) bpp = 1;
	uint8_t pixel[4];
	for (int i = 0; i < bpp; i++) pixel[i] = (uint8_t)(color >> (8 * i));
	for (int y = 0; y < r.h; y++) {
		uint8_t *d = (uint8_t*)dst->pixels + (r.y + y) * dst->pitch + r.x * bpp;
		for (int x = 0; x < r.w; x++) memcpy(d + x * bpp, pixel, bpp);
	}
	return 0;
}

static int mia_blit_scaled(SDL_Surface *src, const SDL_Rect *sr, SDL_Surface *dst, SDL_Rect *dr) { return mia_blit(src, sr, dst, dr); }
static int mia_soft_stretch(SDL_Surface *src, const SDL_Rect *sr, SDL_Surface *dst, const SDL_Rect *dr) { SDL_Rect d; if (dr) d=*dr; return mia_blit(src, sr, dst, dr ? &d : NULL); }
static int mia_upper_blit(SDL_Surface *src, const SDL_Rect *sr, SDL_Surface *dst, SDL_Rect *dr) { return mia_blit(src, sr, dst, dr); }
static int mia_lower_blit(SDL_Surface *src, SDL_Rect *sr, SDL_Surface *dst, SDL_Rect *dr) { return mia_blit(src, sr, dst, dr); }

/* ----------------------------------------------------------------- bmp/rw
 * RWops wrap stdio; LoadBMP is stubbed to return NULL (SDLPAL only uses it
 * for the optional touch overlay, which is disabled in this port). */
static Sint32 rw_seek(SDL_RWops *ctx, Sint32 off, int whence) { return (Sint32)fseek((FILE*)ctx->hidden.stdio.fp, off, whence); }
static Sint32 rw_read(SDL_RWops *ctx, void *p, Sint32 sz, Sint32 n) { return (Sint32)fread(p, sz, n, (FILE*)ctx->hidden.stdio.fp) * (sz > 0 ? 1 : 0); }
static Sint32 rw_write(SDL_RWops *ctx, const void *p, Sint32 sz, Sint32 n) { return (Sint32)fwrite(p, sz, n, (FILE*)ctx->hidden.stdio.fp); }
static int    rw_close(SDL_RWops *ctx) { int r = fclose((FILE*)ctx->hidden.stdio.fp); free(ctx); return r; }

static SDL_RWops *mia_rw_from_file(const char *file, const char *mode) {
	if (!file || !mode) return NULL;
	FILE *fp = fopen(file, mode);
	if (!fp) return NULL;
	SDL_RWops *rw = calloc(1, sizeof(SDL_RWops));
	if (!rw) { fclose(fp); return NULL; }
	rw->seek = rw_seek; rw->read = rw_read; rw->write = rw_write; rw->close = rw_close;
	rw->hidden.stdio.fp = fp;
	return rw;
}
static Sint32 mem_seek(SDL_RWops *c, Sint32 o, int w) {
	uint8_t **base = (uint8_t**)&c->hidden.mem.base; uint8_t **here = (uint8_t**)&c->hidden.mem.here; uint8_t *stop = c->hidden.mem.stop;
	Sint32 off = (w == RW_SEEK_SET) ? o : (w == RW_SEEK_CUR) ? (Sint32)(*here - *base) : (Sint32)(stop - *base);
	*here = *base + off; if (*here > stop) *here = stop; return (Sint32)(*here - *base);
}
static Sint32 mem_read(SDL_RWops *c, void *p, Sint32 sz, Sint32 n) {
	Sint32 total = sz * n; Sint32 avail = (Sint32)(c->hidden.mem.stop - c->hidden.mem.here);
	if (total > avail) total = avail > sz ? (avail / sz) * sz : 0;
	memcpy(p, c->hidden.mem.here, total); c->hidden.mem.here += total; return total / (sz > 0 ? sz : 1);
}
static Sint32 mem_write(SDL_RWops *c, const void *p, Sint32 sz, Sint32 n) { (void)c; (void)p; (void)sz; return n; }
static int    mem_close(SDL_RWops *c) { free(c); return 0; }
static SDL_RWops *mia_rw_from_mem(void *data, int size) {
	SDL_RWops *rw = calloc(1, sizeof(SDL_RWops)); if (!rw) return NULL;
	rw->seek = mem_seek; rw->read = mem_read; rw->write = mem_write; rw->close = mem_close;
	rw->hidden.mem.base = data; rw->hidden.mem.here = data; rw->hidden.mem.stop = (uint8_t*)data + size;
	return rw;
}
static SDL_RWops *mia_rw_from_const_mem(const void *data, int size) { return mia_rw_from_mem((void*)data, size); }
static SDL_RWops *mia_alloc_rw(void) { return calloc(1, sizeof(SDL_RWops)); }
static void mia_free_rw(SDL_RWops *rw) { free(rw); }

static SDL_Surface *mia_load_bmp_rw(SDL_RWops *src, int freesrc) { (void)src; if (freesrc && src && src->close) src->close(src); return NULL; }
static int mia_save_bmp_rw(SDL_Surface *s, SDL_RWops *dst, int freedst) { (void)s; if (freedst && dst && dst->close) dst->close(dst); return 0; }

/* ----------------------------------------------------------------- video */
static SDL_Surface  *g_screen_real = NULL;
static uint16_t     *g_present_buf = NULL;   /* 320x240 RGB565 staging */
static SDL_VideoInfo g_video_info;
static int g_screen_w = 320, g_screen_h = 240;

static const SDL_VideoInfo *mia_get_video_info(void) {
	mia_init_default_format();
	g_video_info.vfmt = &g_indexed8_format;
	return &g_video_info;
}
static int  mia_video_mode_ok(int w, int h, int bpp, Uint32 f) { (void)w; (void)h; (void)f; return bpp; }
static SDL_Rect **mia_list_modes(SDL_PixelFormat *f, Uint32 fl) { (void)f; (void)fl; static SDL_Rect r = {0,0,320,240}; static SDL_Rect *m[] = { &r, NULL }; return m; }
static SDL_Surface *mia_set_video_mode(int w, int h, int bpp, Uint32 flags) {
	(void)flags;
	g_screen_w = w; g_screen_h = h;
	if (g_screen_real) mia_free_surface(g_screen_real);
	g_screen_real = mia_create_rgb_surface(SDL_SWSURFACE, w, h, bpp, 0, 0, 0, 0);
	if (!g_present_buf) g_present_buf = malloc(320 * 240 * 2);
	return g_screen_real;
}

/* Forward declaration — defined in the audio section below. */
static void mia_sdl_audio_fill(void);

/* Convert the 8bpp screen surface to RGB565 and hand it to the host. */
static void mia_present_screen(void) {
	if (!g_screen_real || !g_screen_real->format || !g_screen_real->format->palette || !g_present_buf) return;
	SDL_Palette *pal = g_screen_real->format->palette;
	uint8_t *src = (uint8_t*)g_screen_real->pixels;
	int sw = g_screen_real->w, sh = g_screen_real->h;
	int copy_w = sw < 320 ? sw : 320;
	int copy_h = sh < 240 ? sh : 240;
	int src_x = sw > 320 ? (sw - 320) / 2 : 0;
	int src_y = sh > 240 ? (sh - 240) / 2 : 0;
	int dst_x = sw < 320 ? (320 - sw) / 2 : 0;
	int dst_y = sh < 240 ? (240 - sh) / 2 : 0;
	memset(g_present_buf, 0, 320 * 240 * sizeof(uint16_t));
	for (int y = 0; y < copy_h; y++) {
		uint16_t *d = g_present_buf + (dst_y + y) * 320 + dst_x;
		uint8_t *s = src + (src_y + y) * g_screen_real->pitch + src_x;
		for (int x = 0; x < copy_w; x++) {
			SDL_Color c = pal->colors[s[x]];
			d[x] = (uint16_t)(((c.r & 0xF8) << 8) | ((c.g & 0xFC) << 3) | (c.b >> 3));
		}
	}
	mia_host_present_rgb565(g_present_buf, 320, 240, 640);
	mia_sdl_audio_fill();
}
static void mia_update_rect(SDL_Surface *s, Sint32 x, Sint32 y, Uint32 w, Uint32 h) {
	(void)s; (void)x; (void)y; (void)w; (void)h; if (s == g_screen_real) mia_present_screen();
}
static void mia_update_rects(SDL_Surface *s, int n, SDL_Rect *r) { (void)n; (void)r; if (s == g_screen_real) mia_present_screen(); }
static int  mia_flip(SDL_Surface *s) { if (s == g_screen_real) mia_present_screen(); return 0; }

static int  mia_set_gamma(float r, float g, float b) { (void)r; (void)g; (void)b; return 0; }
static int  mia_set_gamma_ramp(const Uint16 *r, const Uint16 *g, const Uint16 *b) { (void)r; (void)g; (void)b; return 0; }
static int  mia_get_gamma_ramp(Uint16 *r, Uint16 *g, Uint16 *b) { (void)r; (void)g; (void)b; return 0; }
static void mia_wm_set_caption(const char *t, const char *i) { (void)t; (void)i; }
static void mia_wm_get_caption(char **t, char **i) { if (t) *t = NULL; if (i) *i = NULL; }
static void mia_wm_set_icon(SDL_Surface *ic, Uint8 *m) { (void)ic; (void)m; }
static int  mia_wm_iconify(void) { return 0; }
static int  mia_wm_toggle_fs(SDL_Surface *s) { (void)s; return 0; }
static int  mia_show_cursor(int t) { (void)t; return 0; }

/* ----------------------------------------------------------------- events */
static Uint8 g_keystate[512];
static int   g_event_state_tab[SDL_NUMEVENTS];
static Uint8 g_quit_latched;

typedef struct {
	Uint8 button;
	SDLKey key;
} MiaButtonKey;

static const MiaButtonKey g_button_keys[] = {
	{MIA_HOST_BUTTON_A, SDLK_RETURN},
	{MIA_HOST_BUTTON_B, SDLK_ESCAPE},
	{MIA_HOST_BUTTON_X, SDLK_f},
	{MIA_HOST_BUTTON_Y, SDLK_r},
	{MIA_HOST_BUTTON_UP, SDLK_UP},
	{MIA_HOST_BUTTON_DOWN, SDLK_DOWN},
	{MIA_HOST_BUTTON_LEFT, SDLK_LEFT},
	{MIA_HOST_BUTTON_RIGHT, SDLK_RIGHT},
	{MIA_HOST_BUTTON_START, SDLK_ESCAPE},
	{MIA_HOST_BUTTON_SELECT, SDLK_s},
	{MIA_HOST_BUTTON_L, SDLK_PAGEUP},
	{MIA_HOST_BUTTON_R, SDLK_PAGEDOWN},
	{MIA_HOST_BUTTON_M, SDLK_a},
};

static void mia_sync_keyboard(void) {
	mia_host_buttons_poll();
	memset(g_keystate, 0, sizeof(g_keystate));
	for (size_t i = 0; i < sizeof(g_button_keys) / sizeof(g_button_keys[0]); i++) {
		g_keystate[g_button_keys[i].key] = mia_host_button_down(g_button_keys[i].button);
	}
}

static int  mia_poll_event(SDL_Event *e) {
	mia_sync_keyboard();
	int quit_down = mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
	                mia_host_button_down(MIA_HOST_BUTTON_START);
	if (!quit_down) {
		g_quit_latched = 0;
	} else if (!g_quit_latched) {
		g_quit_latched = 1;
		if (e) {
			memset(e, 0, sizeof(*e));
			e->type = SDL_QUIT;
		}
		return 1;
	}
	return 0;
}
static int  mia_wait_event(SDL_Event *e) { int r = mia_poll_event(e); if (!r) mia_delay(20); return r; }
static int  mia_push_event(SDL_Event *e) { (void)e; return 0; }
static void mia_pump_events(void) { mia_sync_keyboard(); }
static int  mia_peep_events(SDL_Event *e, int n, int a, Uint32 m) { (void)e; (void)n; (void)a; (void)m; return 0; }
static void mia_set_event_filter(SDL_EventFilter f) { (void)f; }
static SDL_EventFilter mia_get_event_filter(void) { return NULL; }
static Uint8 mia_event_state(Uint8 t, int s) { if (s != SDL_QUERY) g_event_state_tab[t] = s; return (Uint8)g_event_state_tab[t]; }

/* ----------------------------------------------------------------- keyboard */
static Uint8 *mia_get_key_state(int *n) { if (n) *n = (int)sizeof(g_keystate); return g_keystate; }
static const Uint8 *mia_get_keyboard_state(int *n) { return mia_get_key_state(n); }
static SDLMod mia_get_mod_state(void) { return KMOD_NONE; }
static void mia_set_mod_state(SDLMod m) { (void)m; }
static char *mia_get_key_name(SDLKey k) { (void)k; static char buf[16]; return buf; }
static int  mia_enable_unicode(int e) { (void)e; return 0; }
static int  mia_enable_key_repeat(int d, int i) { (void)d; (void)i; return 0; }
static int  mia_get_scancode_from_key(SDLKey k) { return k >= 0 && k < (SDLKey)sizeof(g_keystate) ? (int)k : 0; }

/* ----------------------------------------------------------------- mouse/joystick/cdrom/active/syswm */
static Uint8 mia_get_mouse_state(int *x, int *y) { if (x)*x=0; if (y)*y=0; return 0; }
static Uint8 mia_get_rel_mouse_state(int *x, int *y) { if (x)*x=0; if (y)*y=0; return 0; }
static void mia_warp_mouse(Uint16 x, Uint16 y) { (void)x; (void)y; }
static SDL_Cursor *mia_create_cursor(Uint8 *d, Uint8 *m, int w, int h, int hx, int hy) { (void)d; (void)m; (void)w; (void)h; (void)hx; (void)hy; return NULL; }
static void mia_set_cursor(SDL_Cursor *c) { (void)c; }
static SDL_Cursor *mia_get_cursor(void) { return NULL; }
static void mia_free_cursor(SDL_Cursor *c) { (void)c; }
static int mia_num_joysticks(void) { return 0; }
static const char *mia_joystick_name(int i) { (void)i; return NULL; }
static const char *mia_joystick_name_for_index(int i) { (void)i; return NULL; }
static SDL_Joystick *mia_joystick_open(int i) { (void)i; return NULL; }
static void mia_joystick_close(SDL_Joystick *j) { (void)j; }
static int  mia_joystick_event_state(int s) { (void)s; return 0; }
static int  mia_joystick_get_axis(SDL_Joystick *j, int a) { (void)j; (void)a; return 0; }
static Uint8 mia_joystick_get_hat(SDL_Joystick *j, int h) { (void)j; (void)h; return 0; }
static int  mia_joystick_get_ball(SDL_Joystick *j, int b, int *dx, int *dy) { (void)j; (void)b; if(dx)*dx=0; if(dy)*dy=0; return 0; }
static Uint8 mia_joystick_get_button(SDL_Joystick *j, int b) { (void)j; (void)b; return 0; }
static int  mia_joystick_num_axes(SDL_Joystick *j) { (void)j; return 0; }
static int  mia_joystick_num_balls(SDL_Joystick *j) { (void)j; return 0; }
static int  mia_joystick_num_hats(SDL_Joystick *j) { (void)j; return 0; }
static int  mia_joystick_num_buttons(SDL_Joystick *j) { (void)j; return 0; }
static void mia_joystick_update(void) {}
static Uint8 mia_get_app_state(void) { return SDL_APPINPUTFOCUS | SDL_APPACTIVE; }
static int  mia_get_wm_info(SDL_SysWMinfo *i) { (void)i; return 0; }
static void *mia_load_object(const char *f) { (void)f; return NULL; }
static void *mia_load_function(void *h, const char *n) { (void)h; (void)n; return NULL; }
static void mia_unload_object(void *h) { (void)h; }

/* ----------------------------------------------------------------- audio
 * No separate audio thread exists in the ELF loader context.  The SDL
 * callback registered via SDL_OpenAudio is invoked from mia_sdl_audio_fill(),
 * which is called every frame from mia_present_screen().  The callback
 * fills a PCM buffer; mia_sdl_audio_fill() then pushes it to I2S via the
 * host ABI. */
static SDL_AudioCallback g_audio_cb = NULL;
static void              *g_audio_ud = NULL;
static int                g_audio_paused = 1;
static Uint8             *g_audio_buf = NULL;
static int                g_audio_buf_bytes = 0;
static Uint8              g_audio_channels = 0;

static int  mia_audio_init(const char *n) { (void)n; return 0; }
static void mia_audio_quit(void) {}
static int  mia_open_audio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {
	if (!desired || desired->freq <= 0 ||
	    (desired->channels != 1 && desired->channels != 2) ||
	    desired->format != AUDIO_S16SYS || !desired->callback) return -1;
	int buffer_bytes = desired->size > 0
		? (int)desired->size
		: (int)desired->samples * desired->channels * (int)sizeof(Sint16);
	if (buffer_bytes <= 0) return -1;

	g_audio_cb = desired->callback;
	g_audio_ud = desired->userdata;
	g_audio_buf_bytes = buffer_bytes;
	g_audio_channels = desired->channels;
	g_audio_paused = 1;
	if (g_audio_buf) free(g_audio_buf);
	g_audio_buf = (Uint8 *)malloc((size_t)g_audio_buf_bytes);
	if (!g_audio_buf || !mia_host_audio_open(desired->freq, desired->channels, 16)) {
		free(g_audio_buf);
		g_audio_buf = NULL;
		g_audio_cb = NULL;
		g_audio_ud = NULL;
		g_audio_buf_bytes = 0;
		g_audio_channels = 0;
		return -1;
	}
	if (obtained) *obtained = *desired;
	return 0;
}
static void mia_close_audio(void) {
	g_audio_cb = NULL; g_audio_ud = NULL; g_audio_paused = 1;
	if (g_audio_buf) { free(g_audio_buf); g_audio_buf = NULL; }
	g_audio_buf_bytes = 0; g_audio_channels = 0;
	mia_host_audio_stop(); mia_host_audio_close();
}
static void mia_pause_audio(int p) {
	g_audio_paused = p;
	if (p) mia_host_audio_stop();
}
static void mia_lock_audio(void) {}
static void mia_unlock_audio(void) {}
static int  mia_build_audio_cvt(SDL_AudioCVT *c, SDL_AudioFormat sf, Uint8 sc, int sr, SDL_AudioFormat df, Uint8 dc, int dr) {
	(void)c; (void)sf; (void)sc; (void)sr; (void)df; (void)dc; (void)dr; return 0;
}
static int  mia_convert_audio(SDL_AudioCVT *c) { (void)c; return 0; }
static void mia_mix_audio(Uint8 *d, const Uint8 *s, Uint32 len, int volume) {
	if (!d || !s || len < 2) return;
	const Sint16 *src = (const Sint16 *)s;
	Sint16       *dst = (Sint16 *)d;
	int samples = (int)(len / sizeof(Sint16));
	for (int i = 0; i < samples; i++) {
		int32_t v = dst[i] + (src[i] * volume / SDL_MIX_MAXVOLUME);
		if (v >  32767) v =  32767;
		if (v < -32768) v = -32768;
		dst[i] = (Sint16)v;
	}
}

/* Dormant for lavapal, whose static audio task writes directly to I2S. */
static void mia_sdl_audio_fill(void) {
	if (g_audio_paused || !g_audio_cb || !g_audio_buf ||
	    g_audio_buf_bytes <= 0 || g_audio_channels == 0) return;
	memset(g_audio_buf, 0, (size_t)g_audio_buf_bytes);
	g_audio_cb(g_audio_ud, g_audio_buf, g_audio_buf_bytes);
	mia_host_audio_write_pcm16((const int16_t *)g_audio_buf,
	                           (uint32_t)(g_audio_buf_bytes /
	                                      (g_audio_channels * sizeof(int16_t))),
	                           g_audio_channels);
}

/* ----------------------------------------------------------------- mutex/thread (libc stubs — ELF loader has no FreeRTOS) */
struct SDL_mutex { volatile int locked; };
static SDL_mutex *mia_create_mutex(void) { return calloc(1, sizeof(SDL_mutex)); }
static int  mia_lock_mutex(SDL_mutex *m) { if (!m) return -1; while (m->locked) usleep(1000); m->locked = 1; return 0; }
static int  mia_try_lock_mutex(SDL_mutex *m) { if (!m) return -1; if (m->locked) return -1; m->locked = 1; return 0; }
static int  mia_unlock_mutex(SDL_mutex *m) { if (!m) return -1; m->locked = 0; return 0; }
static void mia_destroy_mutex(SDL_mutex *m) { free(m); }
struct SDL_semaphore { volatile int val; };
static SDL_sem *mia_create_semaphore(Uint32 v) { SDL_sem *x = calloc(1, sizeof(SDL_sem)); if (x) x->val = (int)v; return x; }
static void mia_destroy_semaphore(SDL_sem *s) { free(s); }
static int  mia_sem_wait(SDL_sem *s) { if (!s) return -1; while (s->val <= 0) usleep(1000); s->val--; return 0; }
static int  mia_sem_try_wait(SDL_sem *s) { if (!s || s->val <= 0) return -1; s->val--; return 0; }
static int  mia_sem_wait_timeout(SDL_sem *s, Uint32 ms) { if (!s) return -1; Uint32 left = ms; while (s->val <= 0 && left > 0) { usleep(1000); left--; } if (s->val <= 0) return -1; s->val--; return 0; }
static int  mia_sem_post(SDL_sem *s) { if (!s) return -1; s->val++; return 0; }
static Uint32 mia_sem_value(SDL_sem *s) { return s ? (Uint32)s->val : 0; }
struct SDL_cond { volatile int flag; };
static SDL_cond *mia_create_cond(void) { return calloc(1, sizeof(SDL_cond)); }
static void mia_destroy_cond(SDL_cond *c) { free(c); }
static int  mia_cond_signal(SDL_cond *c) { if (c) c->flag = 1; return 0; }
static int  mia_cond_broadcast(SDL_cond *c) { if (c) c->flag = 1; return 0; }
static int  mia_cond_wait(SDL_cond *c, SDL_mutex *m) { if (!c) return -1; if (m) { m->locked = 0; } c->flag = 0; usleep(10000); if (m) { while (m->locked) usleep(1000); m->locked = 1; } return 0; }
static int  mia_cond_wait_timeout(SDL_cond *c, SDL_mutex *m, Uint32 ms) { (void)ms; return mia_cond_wait(c, m); }
struct SDL_Thread { int done; };
static SDL_Thread *mia_create_thread(SDL_ThreadFunction fn, const char *name, void *data) { SDL_Thread *t = calloc(1, sizeof(SDL_Thread)); if (t && fn) { fn(data); t->done = 1; } return t; }
static void mia_wait_thread(SDL_Thread *t, int *s) { if (s) *s = 0; free(t); }
static unsigned long mia_thread_id(void) { return 1; }

/* ----------------------------------------------------------------- init/quit */
static Uint32 g_init_flags;
static int  mia_init(Uint32 f) { mia_init_default_format(); memset(g_keystate, 0, sizeof(g_keystate)); g_quit_latched = 0; g_init_flags = f; return 0; }
static void mia_quit(void) { g_init_flags = 0; }
static int  mia_init_subsystem(Uint32 f) { g_init_flags |= f; return 0; }
static void mia_quit_subsystem(Uint32 f) { g_init_flags &= ~f; }
static Uint32 mia_was_init(Uint32 f) { return g_init_flags & f; }
static const SDL_version g_linked_version = { SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL };
static const SDL_version *mia_linked_version(void) { return &g_linked_version; }
void SDL_GetVersion(SDL_version *v) { if (v) *v = g_linked_version; }

/* ----------------------------------------------------------------- export */
__attribute__((visibility("default")))
const MiaSdlApi *mia_sdl_get_api(Uint32 requested_version) {
	static const MiaSdlApi api = {
		.abi_version = MIA_SDL_ABI_VERSION,
		.struct_size = sizeof(MiaSdlApi),
		.init = mia_init, .quit = mia_quit,
		.init_subsystem = mia_init_subsystem, .quit_subsystem = mia_quit_subsystem,
		.was_init = mia_was_init, .linked_version = mia_linked_version,
		.set_error = mia_set_error, .get_error = mia_get_error, .clear_error = mia_clear_error,
		.get_ticks = mia_get_ticks, .delay = mia_delay,
		.get_performance_counter = mia_perf_counter, .get_performance_frequency = mia_perf_freq,
		.get_video_info = mia_get_video_info, .list_modes = mia_list_modes,
		.video_mode_ok = mia_video_mode_ok, .set_video_mode = mia_set_video_mode,
		.update_rect = mia_update_rect, .update_rects = mia_update_rects, .flip = mia_flip,
		.set_gamma = mia_set_gamma, .set_gamma_ramp = mia_set_gamma_ramp, .get_gamma_ramp = mia_get_gamma_ramp,
		.wm_set_caption = mia_wm_set_caption, .wm_get_caption = mia_wm_get_caption,
		.wm_set_icon = mia_wm_set_icon, .wm_iconify_window = mia_wm_iconify,
		.wm_toggle_full_screen = mia_wm_toggle_fs, .show_cursor = mia_show_cursor,
		.map_rgb = mia_map_rgb, .map_rgba = mia_map_rgba, .get_rgb = mia_get_rgb, .get_rgba = mia_get_rgba,
		.alloc_palette = mia_alloc_palette, .free_palette = mia_free_palette, .set_palette_colors = mia_set_palette_colors,
		.create_rgb_surface = mia_create_rgb_surface, .create_rgb_surface_from = mia_create_rgb_surface_from,
		.display_format = mia_display_format, .display_format_alpha = mia_display_format_alpha,
		.free_surface = mia_free_surface, .lock_surface = mia_lock_surface, .unlock_surface = mia_unlock_surface,
		.set_palette = mia_set_palette, .set_colors = mia_set_colors,
		.set_color_key = mia_set_color_key, .set_alpha = mia_set_alpha,
		.set_surface_palette = mia_set_surface_palette, .set_surface_color_mod = mia_set_surface_color_mod,
		.fill_rect = mia_fill_rect, .blit_surface = mia_blit, .blit_scaled = mia_blit_scaled,
		.soft_stretch = mia_soft_stretch, .upper_blit = mia_upper_blit, .lower_blit = mia_lower_blit,
		.convert_surface = mia_convert_surface, .load_bmp_rw = mia_load_bmp_rw, .save_bmp_rw = mia_save_bmp_rw,
		.pump_events = mia_pump_events, .poll_event = mia_poll_event, .wait_event = mia_wait_event,
		.push_event = mia_push_event, .peep_events = mia_peep_events,
		.set_event_filter = mia_set_event_filter, .get_event_filter = mia_get_event_filter,
		.event_state = mia_event_state,
		.get_key_state = mia_get_key_state, .get_mod_state = mia_get_mod_state,
		.set_mod_state = mia_set_mod_state, .get_key_name = mia_get_key_name,
		.enable_unicode = mia_enable_unicode, .enable_key_repeat = mia_enable_key_repeat,
		.get_keyboard_state = mia_get_keyboard_state, .get_scancode_from_key = mia_get_scancode_from_key,
		.get_mouse_state = mia_get_mouse_state, .get_relative_mouse_state = mia_get_rel_mouse_state,
		.warp_mouse = mia_warp_mouse, .create_cursor = mia_create_cursor,
		.set_cursor = mia_set_cursor, .get_cursor = mia_get_cursor, .free_cursor = mia_free_cursor,
		.num_joysticks = mia_num_joysticks, .joystick_name = mia_joystick_name,
		.joystick_name_for_index = mia_joystick_name_for_index, .joystick_open = mia_joystick_open,
		.joystick_close = mia_joystick_close, .joystick_event_state = mia_joystick_event_state,
		.joystick_get_axis = mia_joystick_get_axis, .joystick_get_hat = mia_joystick_get_hat,
		.joystick_get_ball = mia_joystick_get_ball, .joystick_get_button = mia_joystick_get_button,
		.joystick_num_axes = mia_joystick_num_axes, .joystick_num_balls = mia_joystick_num_balls,
		.joystick_num_hats = mia_joystick_num_hats, .joystick_num_buttons = mia_joystick_num_buttons,
		.joystick_update = mia_joystick_update,
		.audio_init = mia_audio_init, .audio_quit = mia_audio_quit,
		.open_audio = mia_open_audio, .close_audio = mia_close_audio,
		.pause_audio = mia_pause_audio, .lock_audio = mia_lock_audio, .unlock_audio = mia_unlock_audio,
		.build_audio_cvt = mia_build_audio_cvt, .convert_audio = mia_convert_audio, .mix_audio = mia_mix_audio,
		.rw_from_file = mia_rw_from_file, .rw_from_mem = mia_rw_from_mem,
		.rw_from_const_mem = mia_rw_from_const_mem, .alloc_rw = mia_alloc_rw, .free_rw = mia_free_rw,
		.create_mutex = mia_create_mutex, .lock_mutex = mia_lock_mutex, .try_lock_mutex = mia_try_lock_mutex,
		.unlock_mutex = mia_unlock_mutex, .destroy_mutex = mia_destroy_mutex,
		.create_semaphore = mia_create_semaphore, .destroy_semaphore = mia_destroy_semaphore,
		.sem_wait = mia_sem_wait, .sem_try_wait = mia_sem_try_wait, .sem_wait_timeout = mia_sem_wait_timeout,
		.sem_post = mia_sem_post, .sem_value = mia_sem_value,
		.create_cond = mia_create_cond, .destroy_cond = mia_destroy_cond,
		.cond_signal = mia_cond_signal, .cond_broadcast = mia_cond_broadcast,
		.cond_wait = mia_cond_wait, .cond_wait_timeout = mia_cond_wait_timeout,
		.create_thread = mia_create_thread, .wait_thread = mia_wait_thread, .thread_id = mia_thread_id,
		.get_app_state = mia_get_app_state, .get_wm_info = mia_get_wm_info,
		.load_object = mia_load_object, .load_function = mia_load_function, .unload_object = mia_unload_object,
	};
	return requested_version == MIA_SDL_ABI_VERSION ? &api : NULL;
}
