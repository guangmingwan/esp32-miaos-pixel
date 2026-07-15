/* Native GCC wrapper for the ESP32 PAL application. */

#if defined(__GNUC__)
#define __LAVA__ 0x100
#define LAVA_NATIVE_COMPILED 1
#define LAVA_WIDTH 320
#define LAVA_HEIGHT 200
#define LAVA_COLOR 8
#define LAVA_ENCODING_UTF8 1
#include "lava_rt_esp32.h"
#include "lavax_native_begin.h"
#endif

#include "main_lava_app.c"

#if defined(__GNUC__)
#include "lavax_native_end.h"

#undef main

int lavapal_main_impl(int argc, char *argv[])
{
    g_argc = argc;
    g_argv = argv;
    user_main();
    return g_exit_code;
}
#endif
