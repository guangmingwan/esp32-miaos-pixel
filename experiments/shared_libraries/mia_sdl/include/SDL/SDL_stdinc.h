/*
 * MiaOS SDL 1.2 compatibility — stdinc replacements.
 * These map directly to newlib libc so SDLPAL's SDL_-prefixed
 * string/memory helpers resolve without a real SDL.
 */
#ifndef _SDL_STDINC_H
#define _SDL_STDINC_H

#include "SDL_types.h"
#include "SDL_byteorder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_arraysize(array)    (sizeof(array) / sizeof(array[0]))
#define SDL_TABLESIZE(table)    SDL_arraysize(table)

/* SDL_* stdinc aliases — SDLPAL references these by name. */
#define SDL_malloc   malloc
#define SDL_calloc   calloc
#define SDL_realloc  realloc
#define SDL_free     free
#define SDL_getenv   getenv
#define SDL_setenv   setenv
#define SDL_putenv   putenv
#define SDL_qsort    qsort
#define SDL_abs      abs
#define SDL_atoi     atoi
#define SDL_atof     atof
#define SDL_strtol   strtol
#define SDL_strtoul  strtoul
#define SDL_strtod   strtod
#define SDL_memcpy   memcpy
#define SDL_memmove  memmove
#define SDL_memset   memset
#define SDL_memcmp   memcmp
#define SDL_strlen   strlen
#define SDL_strlcpy  strlcpy
#define SDL_strlcat  strlcat
#define SDL_strdup   strdup
#define SDL_strchr   strchr
#define SDL_strrchr  strrchr
#define SDL_strstr   strstr
#define SDL_strcmp   strcmp
#define SDL_strncmp  strncmp
#define SDL_strcasecmp  strcasecmp
#define SDL_strncasecmp strncasecmp
#define SDL_sscanf   sscanf
#define SDL_snprintf snprintf
#define SDL_vsnprintf vsnprintf
#define SDL_sscanf   sscanf
#define SDL_toupper  toupper
#define SDL_tolower  tolower
#define SDL_isspace  isspace
#define SDL_isdigit  isdigit
#define SDL_isalpha  isalpha
#define SDL_isalnum  isalnum
#define SDL_isprint  isprint

#define SDL_ICONV_ERROR  ((size_t)-1)
#define SDL_ICONV_E2BIG  ((size_t)-2)
#define SDL_ICONV_EILSEQ ((size_t)-3)
#define SDL_ICONV_EINVAL ((size_t)-4)

#ifdef __cplusplus
}
#endif
#endif /* _SDL_STDINC_H */
