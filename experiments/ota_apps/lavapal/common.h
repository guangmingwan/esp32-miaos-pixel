#ifndef _COMMON_H
#define _COMMON_H

#ifndef ENABLE_REVISIED_BATTLE
# define PAL_CLASSIC        1
#endif

#include "defines.h"

#ifdef __LAVA__

#if defined(LAVA_ESP32)
#include "lava_sdl_esp32.h"
#elif defined(LAVA_NATIVE_COMPILED)
#include "sdl_compat.h"
#endif
#include "lava_sdl_compat.h"
#include "lava_mem.h"

#else

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <stdarg.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "sdl_compat.h"

#define __WIDETEXT(quote) L##quote
#define WIDETEXT(quote) __WIDETEXT(quote)

#define STR_INDIR(x)                    #x
#define STR(x)                          STR_INDIR(x)

#if !defined(fmax) || !defined(fmin)
# include <math.h>
#endif

#include <float.h>

#ifndef max
# define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
# define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A, B)  ((Sint32)((B) - (A)) <= 0)
#endif

#ifndef SDL_INIT_CDROM
# define SDL_INIT_CDROM       0
#endif

#ifndef SDL_AUDIO_BITSIZE
# define SDL_AUDIO_BITSIZE(x)         (x & 0xFF)
#endif

#ifndef SDL_FORCE_INLINE
#if defined(_MSC_VER)
#define SDL_FORCE_INLINE __forceinline
#elif ( (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__) )
#define SDL_FORCE_INLINE __attribute__((always_inline)) __inline__
#else
#define SDL_FORCE_INLINE SDL_INLINE
#endif
#endif

#if defined(_MSC_VER)
# define PAL_FORCE_INLINE SDL_FORCE_INLINE
#else
# define PAL_FORCE_INLINE SDL_FORCE_INLINE
#endif

#ifdef _WIN32

# include <windows.h>
# include <io.h>

# if defined(_MSC_VER)
#  if _MSC_VER < 1900
#   define vsnprintf _vsnprintf
#   define snprintf _snprintf
#  endif
#  define strdup _strdup
#  define access _access
#  pragma warning (disable:4244)
# endif

# define PAL_MAX_PATH  MAX_PATH

#else

# include <unistd.h>
# include <dirent.h>
# ifdef __APPLE__
#  include <objc/objc.h>
# endif

# ifndef FALSE
#  define FALSE               0
# endif
# ifndef TRUE
#  define TRUE                1
# endif
# define VOID                void

#define CHAR char
#define WCHAR wchar_t
#define SHORT short
#define LONG long

#define ULONG unsigned long
#define PULONG ULONG *
#define USHORT unsigned short
#define PUSHORT USHORT *
#define UCHAR unsigned char
#define PUCHAR UCHAR *

#define WORD unsigned short
#define LPWORD WORD *
#define DWORD unsigned int
#define LPDWORD DWORD *
#define INT int
#define LPINT INT *
# if !defined( __APPLE__ ) && !defined( GEKKO )
#  define BOOL int
#  define LPBOOL BOOL *
# endif
#define UINT unsigned int
#define PUINT UINT *
#define UINT32 unsigned int
#define PUINT32 UINT32 *
#define BYTE unsigned char
#define LPBYTE BYTE *
#define LPCBYTE BYTE *
#define FLOAT float
#define LPFLOAT FLOAT *
#define LPVOID void *
#define LPCVOID void *
#define LPSTR CHAR *
#define LPCSTR CHAR *
#define LPWSTR WCHAR *
#define LPCWSTR WCHAR *

#ifdef PATH_MAX
# define PAL_MAX_PATH  PATH_MAX
#else
# define PAL_MAX_PATH  1024
#endif

#endif

#include "lava_mem.h"

#ifdef __cplusplus
# define PAL_C_LINKAGE       extern "C"
# define PAL_C_LINKAGE_BEGIN PAL_C_LINKAGE {
# define PAL_C_LINKAGE_END   }
#else
# define PAL_C_LINKAGE
# define PAL_C_LINKAGE_BEGIN ;
# define PAL_C_LINKAGE_END   ;
#endif

#include "pal_config.h"

#if !SDL_VERSION_ATLEAST(2,0,0)
# if PAL_HAS_GLSL
#  undef PAL_HAS_GLSL
# endif
#define SDL_strcasecmp strcasecmp
#define SDL_setenv(a,b,c)
#endif

#ifndef PAL_DEFAULT_FULLSCREEN_HEIGHT
# define PAL_DEFAULT_FULLSCREEN_HEIGHT PAL_DEFAULT_WINDOW_HEIGHT
#endif

#ifndef PAL_DEFAULT_TEXTURE_WIDTH
# define PAL_DEFAULT_TEXTURE_WIDTH     PAL_DEFAULT_WINDOW_WIDTH
#endif

#ifndef PAL_DEFAULT_TEXTURE_HEIGHT
# define PAL_DEFAULT_TEXTURE_HEIGHT    PAL_DEFAULT_WINDOW_HEIGHT
#endif

#ifndef PAL_AUDIO_DEFAULT_BUFFER_SIZE
# define PAL_AUDIO_DEFAULT_BUFFER_SIZE   1024
#endif

#ifndef PAL_HAS_SDLCD
# define PAL_HAS_SDLCD        0
#endif

#ifndef PAL_HAS_MP3
# define PAL_HAS_MP3          0
#endif
#ifndef PAL_HAS_OGG
# define PAL_HAS_OGG          0
#endif
#ifndef PAL_HAS_OPUS
# define PAL_HAS_OPUS         0
#endif

#ifndef PAL_CONFIG_PREFIX
# define PAL_CONFIG_PREFIX PAL_PREFIX
#endif

#ifndef PAL_LARGE
# define PAL_LARGE
#endif

#ifndef PAL_SCALE_SCREEN
# define PAL_SCALE_SCREEN   TRUE
#endif

#ifndef PAL_IS_VALID_JOYSTICK
# define PAL_IS_VALID_JOYSTICK(s)  TRUE
#endif

#ifndef PAL_FATAL_OUTPUT
# define PAL_FATAL_OUTPUT(s)
#endif

#ifndef PAL_CONVERT_UTF8
# define PAL_CONVERT_UTF8(s) s
#endif

#ifndef PAL_NATIVE_PATH_SEPARATOR
# define PAL_NATIVE_PATH_SEPARATOR "/"
#endif

#define PAL_fread(buf, elem, num, fp) if (fread((buf), (elem), (num), (fp)) < (num)) return -1

#define LOGLEVEL_MIN 0
#define LOGLEVEL_VERBOSE 0
#define LOGLEVEL_DEBUG 1
#define LOGLEVEL_INFO 2
#define LOGLEVEL_WARNING 3
#define LOGLEVEL_ERROR 4
#define LOGLEVEL_FATAL 5
#define LOGLEVEL_MAX 5

#define LOGLEVEL int

#define PAL_LOG_MAX_OUTPUTS   (LOGLEVEL_MAX + 1)

#if defined(DEBUG) || defined(_DEBUG)
# define PAL_DEFAULT_LOGLEVEL  LOGLEVEL_MIN
#else
# define PAL_DEFAULT_LOGLEVEL  LOGLEVEL_MAX
#endif

#ifndef PAL_HAS_CONFIG_PAGE
# define PAL_HAS_CONFIG_PAGE   FALSE
#endif

#define PAL_MAX_GLOBAL_BUFFERS 4
#define PAL_GLOBAL_BUFFER_SIZE 1024

#ifndef PAL_PATH_SEPARATORS
# define PAL_PATH_SEPARATORS "/"
#endif

#ifndef PAL_IS_PATH_SEPARATOR
# define PAL_IS_PATH_SEPARATOR(x) ((x) == '/')
#endif

#endif

#endif
