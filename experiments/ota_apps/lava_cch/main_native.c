#if defined(__GNUC__)
#define LAVA_NATIVE_COMPILED 1
#include "screen_config.h"
#define LAVA_WIDTH LAVA_CCH_SCREEN_WIDTH
#define LAVA_HEIGHT LAVA_CCH_SCREEN_HEIGHT
#define LAVA_COLOR 4
#define LAVA_ENCODING_UTF8 1

#if defined(LAVA_ESP32)
#include "lava_rt_esp32.h"
#else
#include "../../common/lava_rt_native.h"
#endif
#include "lavax_native_begin.h"
#endif

#if defined(LAVA_ESP32)
int g_lava_shutdown_requested;
#endif

#include "main.c"

#if defined(__GNUC__)
#include "lavax_native_end.h"
#undef main
#endif

#if defined(LAVA_ESP32)
int lava_cch_main_impl(int argc, char *argv[])
{
    return user_main(argc, argv);
}
#elif defined(__linux__)
extern int SDL_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    return SDL_main(argc, argv);
}
#endif
