#ifndef LAVA_MEM_H
#define LAVA_MEM_H

#if defined(LAVA_ESP32)
#define MEM_POOL_SIZE 0
#elif defined(__LAVA__)
#define MEM_POOL_SIZE (64 * 1024)
#else
#define MEM_POOL_SIZE (2 * 1024 * 1024)
#endif

#if defined(__LAVA__) && !defined(LAVA_ESP32)
char g_mem_pool[MEM_POOL_SIZE];
int g_mem_offset;

#define PAL_MEM_STATIC

addr PAL_MEM_ALLOC(int size);
addr PAL_MEM_CALLOC(int count, int elem_size);
addr PAL_MEM_REALLOC(addr old_ptr, int new_size);
addr PAL_MEM_STRDUP(addr str);

#else
#include <stddef.h>

/* Single global pool declared here, defined in lava_mem.c */
extern unsigned char g_mem_pool[MEM_POOL_SIZE];
extern int g_mem_offset;

void *PAL_MEM_ALLOC(size_t size);
void *PAL_MEM_CALLOC(size_t count, size_t elem_size);
void *PAL_MEM_REALLOC(void *old_ptr, size_t new_size);
char *PAL_MEM_STRDUP(char *str);

#endif

void PAL_MEM_FREE(void *p);
void PAL_MEM_RESET(void);
int PAL_MEM_USED(void);

#if !defined(__LAVA__) || defined(LAVA_ESP32)
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef strdup
#define malloc(size)        PAL_MEM_ALLOC(size)
#define calloc(count, size) PAL_MEM_CALLOC(count, size)
#define realloc(ptr, size)  PAL_MEM_REALLOC(ptr, size)
#define free(ptr)           PAL_MEM_FREE(ptr)
#define strdup(str)         PAL_MEM_STRDUP(str)
#endif

#endif
