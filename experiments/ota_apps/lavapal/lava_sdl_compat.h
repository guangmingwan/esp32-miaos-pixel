#ifndef LAVA_SDL_COMPAT_H
#define LAVA_SDL_COMPAT_H

#ifdef fopen
#undef fopen
#endif
#ifdef fclose
#undef fclose
#endif
#ifdef fread
#undef fread
#endif
#ifdef fwrite
#undef fwrite
#endif
#ifdef fseek
#undef fseek
#endif
#ifdef ftell
#undef ftell
#endif

#ifdef LAVA_ESP32
#define fopen(path, mode) native_fopen((path), (mode))
#endif

#define SCREEN_W 320
#define SCREEN_H 200
#define PAL_MAX_PATH 260

#define VOID void
#define BOOL lavax_int
#define BYTE uint8_t
#define WORD uint16_t
#define DWORD uint32_t
#define SHORT int16_t
#define FLOAT lavax_float
#define LPVOID lavax_addr
#define LPCVOID lavax_addr
#define LPBYTE lavax_addr
#define LPCBYTE lavax_addr
#define LPWORD lavax_addr
#define LPDWORD lavax_addr

#define TRUE 1
#define FALSE 0

#ifndef NULL
#define NULL 0
#endif
#ifndef SDL_OK
#define SDL_OK 0
#endif
#ifndef SDL_FAIL
#define SDL_FAIL -1
#endif
#ifndef SDL_MAJOR_VERSION
#define SDL_MAJOR_VERSION 2
#endif
#ifndef SDL_VERSION_ATLEAST
#define SDL_VERSION_ATLEAST(x, y, z) 1
#endif
#ifndef SDL_SwapLE16
#define SDL_SwapLE16(x) (x)
#endif
#ifndef SDL_SwapLE32
#define SDL_SwapLE32(x) (x)
#endif
#endif

#ifndef EXTERN
#define EXTERN
#endif

#ifndef PAL_GLOBAL_BUFFER_SIZE
#define PAL_GLOBAL_BUFFER_SIZE 1024
#endif
#ifndef PAL_MAX_GLOBAL_BUFFERS
#define PAL_MAX_GLOBAL_BUFFERS 4
#endif
#ifndef PAL_MAX_PATH
#define PAL_MAX_PATH 260
#endif
#ifndef PAL_CLASSIC
#define PAL_CLASSIC 1
#endif
#define PAL_DEFAULT_WINDOW_WIDTH SCREEN_W
#define PAL_DEFAULT_WINDOW_HEIGHT SCREEN_H
#define PAL_DEFAULT_FULLSCREEN_HEIGHT SCREEN_H
#define PAL_DEFAULT_TEXTURE_WIDTH SCREEN_W
#define PAL_DEFAULT_TEXTURE_HEIGHT SCREEN_H
#define PAL_AUDIO_DEFAULT_BUFFER_SIZE 1024
#define PAL_HAS_SDLCD 0
#define PAL_HAS_MP3 0
#define PAL_HAS_OGG 0
#define PAL_HAS_OPUS 0
#define PAL_HAS_GLSL 0
#define PAL_HAS_CONFIG_PAGE 0
#define PAL_HAS_PLATFORM_STARTUP 0
#define PAL_SCALE_SCREEN 0
#define PAL_CONFIG_PREFIX "/LavaData/"
#define PAL_NATIVE_PATH_SEPARATOR "/"
#define PAL_PATH_SEPARATORS "/"
#define PAL_IS_PATH_SEPARATOR(x) ((x) == '/')
#define PAL_LOCALIZATION_EXT "slf"
#define PAL_LOG_MAX_OUTPUTS 6

#define LOGLEVEL_MIN 0
#define LOGLEVEL_VERBOSE 0
#define LOGLEVEL_DEBUG 1
#define LOGLEVEL_INFO 2
#define LOGLEVEL_WARNING 3
#define LOGLEVEL_ERROR 4
#define LOGLEVEL_FATAL 5
#define LOGLEVEL_MAX 5
#define PAL_DEFAULT_LOGLEVEL LOGLEVEL_MAX

#ifndef SDL_INIT_CDROM
#define SDL_INIT_CDROM 0
#endif
#ifndef SDL_AUDIO_BITSIZE
#define SDL_AUDIO_BITSIZE(x) (x & 0xFF)
#endif
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A, B) ((int)((B) - (A)) <= 0)
#endif
#ifndef SDL_FORCE_INLINE
#define SDL_FORCE_INLINE
#endif
#ifndef PAL_FORCE_INLINE
#define PAL_FORCE_INLINE
#endif
#define PAL_LARGE
#define PAL_IS_VALID_JOYSTICK(s) TRUE
#define PAL_FATAL_OUTPUT(s)
#define PAL_CONVERT_UTF8(s) s

#ifndef SDL_INIT_VIDEO
#define SDL_INIT_VIDEO 1
#endif
#ifndef SDL_INIT_AUDIO
#define SDL_INIT_AUDIO 2
#endif
#ifndef SDL_INIT_TIMER
#define SDL_INIT_TIMER 4
#endif
#define PAL_SDL_INIT_FLAGS (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)

#define kKeyMenu 0x01
#define kKeySearch 0x02
#define kKeyNorth 0x04
#define kKeySouth 0x08
#define kKeyEast 0x10
#define kKeyWest 0x20
#define kKeyForce 0x40
#define kKeyUseItem 0x80
#define kKeyFlee 0x100

#define KEY_B 0x1B
#define KEY_ENTER 0x0D
#define KEY_UP 0x14
#define KEY_DOWN 0x15
#define KEY_RIGHT 0x16
#define KEY_LEFT 0x17

#ifdef LAVA_NATIVE_COMPILED

#ifdef SetFgColor
#undef SetFgColor
#endif
#ifdef SetBgColor
#undef SetBgColor
#endif
#ifdef TextOut
#undef TextOut
#endif

static SDL_Color g_current_palette[256];
static SDL_Color g_fade_source_palette[256];
static SDL_Surface *g_lava_native_screen_surface;
static SDL_Surface *g_lava_native_back_surface;
static int g_lava_direct_screen;
static int g_argc;
static char **g_argv;
static volatile int g_user_thread_done;
static char g_global_buffer_0[PAL_GLOBAL_BUFFER_SIZE];
static char g_global_buffer_1[PAL_GLOBAL_BUFFER_SIZE];
static char g_global_buffer_2[PAL_GLOBAL_BUFFER_SIZE];
static char g_global_buffer_3[PAL_GLOBAL_BUFFER_SIZE];

static addr UTIL_GlobalBuffer(int idx)
{
   if (idx == 0) return (addr)g_global_buffer_0;
   if (idx == 1) return (addr)g_global_buffer_1;
   if (idx == 2) return (addr)g_global_buffer_2;
   return (addr)g_global_buffer_3;
}

static long PAL_XY(int x, int y)
{
   return ((long)y << 16) | (long)(x & 0xFFFF);
}

static int PAL_X(long v)
{
   return (short)(v & 0xFFFF);
}

static int PAL_Y(long v)
{
   return (short)((v >> 16) & 0xFFFF);
}

#define g_screen_surface (*g_lava_native_screen_surface)
#define g_back_surface (*g_lava_native_back_surface)
#define g_back_buf ((char *)g_lava_native_back_surface->pixels)

static void lava_native_present_palette_buffer(const uint8_t *pixels, const SDL_Color *palette)
{
   LavaRuntime *rt;
   int i;

   rt = lrt_get_global();
   if (rt == NULL || pixels == NULL || palette == NULL)
   {
      return;
   }

   for (i = 0; i < 256; ++i)
   {
      lrt_set_palette(rt, i, palette[i].r, palette[i].g, palette[i].b);
   }
   lrt_write_block(0, 0, SCREEN_W, SCREEN_H, 0, pixels);
   lrt_refresh();
}

static void lava_native_present_current_screen(void)
{
   if (g_lava_native_screen_surface == NULL || g_lava_native_screen_surface->pixels == NULL)
   {
      return;
   }
   lava_native_present_palette_buffer(
      (const uint8_t *)g_lava_native_screen_surface->pixels,
      g_current_palette);
}

static int g_lava_text_batch_depth;

static void lava_begin_text_batch(void)
{
   g_lava_text_batch_depth++;
}

static void lava_end_text_batch(void)
{
   if (g_lava_text_batch_depth > 0)
   {
      g_lava_text_batch_depth--;
   }
   if (g_lava_text_batch_depth == 0)
   {
      lava_native_present_current_screen();
   }
}

static void lava_present_current_screen(void)
{
   if (g_lava_text_batch_depth > 0)
   {
      return;
   }
   lava_native_present_current_screen();
}

static void lava_apply_palette_scale(SDL_Color *src, int level, int max_level)
{
   SDL_Color scaled[256];
   int i;

   if (src == NULL || max_level <= 0)
   {
      return;
   }
   if (level < 0) level = 0;
   if (level > max_level) level = max_level;

   for (i = 0; i < 256; i++)
   {
      scaled[i].r = (uint8_t)((src[i].r * level) / max_level);
      scaled[i].g = (uint8_t)((src[i].g * level) / max_level);
      scaled[i].b = (uint8_t)((src[i].b * level) / max_level);
      scaled[i].a = src[i].a;
   }
   if (g_lava_native_screen_surface != NULL && g_lava_native_screen_surface->pixels != NULL)
   {
      lava_native_present_palette_buffer((const uint8_t *)g_lava_native_screen_surface->pixels, scaled);
   }
}

static void lava_init_video(void)
{
   if (g_lava_native_screen_surface == NULL)
   {
      g_lava_native_screen_surface = SDL_CreateRGBSurface(0, SCREEN_W, SCREEN_H, 8, 0, 0, 0, 0);
   }
   if (g_lava_native_back_surface == NULL)
   {
      g_lava_native_back_surface = SDL_CreateRGBSurface(0, SCREEN_W, SCREEN_H, 8, 0, 0, 0, 0);
   }
   if (g_lava_native_screen_surface && g_lava_native_screen_surface->pixels)
   {
      memset(g_lava_native_screen_surface->pixels, 0, (size_t)SCREEN_W * (size_t)SCREEN_H);
   }
   if (g_lava_native_back_surface && g_lava_native_back_surface->pixels)
   {
      memset(g_lava_native_back_surface->pixels, 0, (size_t)SCREEN_W * (size_t)SCREEN_H);
   }
   g_lava_direct_screen = 0;
}

#ifdef GetCommandLine
#undef GetCommandLine
#endif

static void GetCommandLine(addr cmdline)
{
   char *dst;
   int i;
   int pos;

   if (cmdline == 0)
   {
      return;
   }

   dst = (char *)cmdline;
   pos = 0;
   for (i = 0; i < g_argc && pos < 255; i++)
   {
      char *src;
      int j;

      src = g_argv[i];
      if (src == 0)
      {
         continue;
      }
      if (i != 0)
      {
         dst[pos++] = ' ';
         if (pos >= 255)
         {
            break;
         }
      }
      for (j = 0; src[j] != 0 && pos < 255; j++)
      {
         dst[pos++] = src[j];
      }
   }
   dst[pos] = 0;
}

static void SDL_SetPalette(addr palette, int firstcolor, int ncolors)
{
   if (palette == 0 || firstcolor < 0 || ncolors <= 0)
   {
      return;
   }
   if (firstcolor + ncolors > 256)
   {
      ncolors = 256 - firstcolor;
   }

   memcpy(&g_current_palette[firstcolor], (void *)palette, (size_t)ncolors * sizeof(SDL_Color));
   lrt_set_palette_vm(firstcolor, ncolors, (const uint8_t *)palette);
}

static void VIDEO_UpdateScreen(addr rect)
{
   (void)rect;
   lava_native_present_current_screen();
}

static void lava_native_set_fgcolor(int color)
{
   lrt_set_fgcolor_global(color);
}

static void lava_native_set_bgcolor(int color)
{
   lrt_set_bgcolor_global(color);
}

static void lava_native_textout(int x, int y, addr str, int mode)
{
   if (str == 0)
   {
      return;
   }

   /* Keep native text on the same direct drawing path as graphics. Do not
    * mirror the legacy workaround by reading the runtime screen back into
    * g_lava_native_screen_surface after TextOut. */
   lrt_textout(x, y, (const char *)str, mode);
   if (g_lava_text_batch_depth == 0)
   {
      lava_native_present_current_screen();
   }
}

#define SetFgColor(color) lava_native_set_fgcolor(color)
#define SetBgColor(color) lava_native_set_bgcolor(color)
#define TextOut(x, y, str, mode) lava_native_textout((x), (y), (str), (mode))

static void PAL_FadeOut(int step)
{
   int i;
   SDL_Color source[256];
   SDL_Color scaled[256];

   memcpy(source, g_current_palette, sizeof(source));
   for (i = 15; i >= 0; i--)
   {
      int c;
      for (c = 0; c < 256; c++)
      {
         scaled[c].r = (uint8_t)((source[c].r * i) / 15);
         scaled[c].g = (uint8_t)((source[c].g * i) / 15);
         scaled[c].b = (uint8_t)((source[c].b * i) / 15);
         scaled[c].a = source[c].a;
      }
      lava_native_present_palette_buffer((const uint8_t *)g_lava_native_screen_surface->pixels, scaled);
      SDL_Delay(step * 50);
   }
}

static void PAL_FadeIn(int step)
{
   int i;
   SDL_Color source[256];
   SDL_Color scaled[256];

   memcpy(source, g_current_palette, sizeof(source));
   for (i = 0; i <= 15; i++)
   {
      int c;
      for (c = 0; c < 256; c++)
      {
         scaled[c].r = (uint8_t)((source[c].r * i) / 15);
         scaled[c].g = (uint8_t)((source[c].g * i) / 15);
         scaled[c].b = (uint8_t)((source[c].b * i) / 15);
         scaled[c].a = source[c].a;
      }
      lava_native_present_palette_buffer((const uint8_t *)g_lava_native_screen_surface->pixels, scaled);
      SDL_Delay(step * 50);
   }
}

static int memset16(addr dest, int val, int count)
{
   int i;
   for (i = 0; i < count; i++)
   {
      *(uint16_t *)(dest + i * 2) = (uint16_t)val;
   }
   return 0;
}

static void UTIL_LogOutput(int level, addr fmt, ...)
{
   (void)level;
   (void)fmt;
}

static int RandomLong(int min, int max)
{
   int span;
   if (max < min)
   {
      int t = min;
      min = max;
      max = t;
   }
   span = max - min + 1;
   if (span <= 0)
   {
      return min;
   }
   return min + (rand() % span);
}

static int UTIL_Delay(int ms)
{
   SDL_Delay(ms);
   return 0;
}

static void TerminateOnError(char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   vfprintf(stderr, fmt, ap);
   va_end(ap);
   fputc('\n', stderr);
   exit(1);
}

#endif /* LAVA_SDL_COMPAT_H */
