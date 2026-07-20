/*
 * dlfcn.h shim — ESP-IDF provides dynamic loading via esp_dlfcn.h (from the
 * elf_loader component). util.c pulls in <dlfcn.h>; redirect it.
 */
#ifndef LAVAPAL_DLFCN_H
#define LAVAPAL_DLFCN_H

#include "esp_dlfcn.h"

#ifndef RTLD_LAZY
#define RTLD_LAZY RTLD_NOW
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif
#ifndef RTLD_LOCAL
#define RTLD_LOCAL 0
#endif
#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT ((void *)0)
#endif

#endif /* LAVAPAL_DLFCN_H */
