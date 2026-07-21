# Lava PAL OTA app (SDLPAL port)

This ESP-IDF project builds the SDLPAL (仙剑奇侠传) port for the MiaOS Pixel
launcher. The SDL 1.2 compatibility layer is provided by a separate shared
library, **libmia_sdl_v1.so**, that is loaded at runtime via the ELF loader.

## Architecture

```
lava_pal.bin  ─┐
               ├─ dlopen("/sd/MiaOS/Library/libmia_sdl_v1.so")
libmia_sdl.so ─┤   → MiaSdlApi function table
               │
               └─ sdl_wrapper.c forwards each SDL_* call to the API table
                  so unmodified SDLPAL source can link.
```

* `lava_pal.bin` — host firmware. Contains SDLPAL core + a thin `SDL_Foo →
  g_sdl_api->foo` wrapper layer (`main/sdl_wrapper.c`). Bootloader selects
  `ota_1` (this app) and reboots into it.
* `libmia_sdl_v1.so` — ELF shared object providing the SDL 1.2-compatible
  surface, palette, blit, timer, mutex, RWops and event APIs on top of
  `mia_host_abi` (display + audio + buttons). Built from
  `experiments/shared_libraries/mia_sdl/`.

Both sides share the SDL type/declaration headers in
`experiments/shared_libraries/mia_sdl/include/SDL/` and the ABI contract in
`include/mia_sdl_api.h`.

## Build

Build both projects with the ESP-IDF toolchain:

```sh
source /opt/esp-idf/export.sh

# 1. Build the SDL compatibility shared library.
cd experiments/shared_libraries/mia_sdl
idf.py so
# Output: build/libmia_sdl_v1.so

# 2. Build the lavapal firmware.
cd experiments/ota_apps/lavapal
idf.py build
# Output: build/lava_pal.bin
```

The shared library must be installed on the SD card before the app can run:

```text
/sd/MiaOS/Library/libmia_sdl_v1.so
```

Append the app manifest and copy the binary as usual:

```sh
python tools/append_manifest.py --input experiments/ota_apps/lavapal/build/lava_pal.bin \
    --category Games --name lava_pal
cp experiments/ota_apps/lavapal/build/lava_pal.bin /sd/Games/lava_pal.app/lava_pal.bin
```

## SD card game data

The firmware does not ship PAL game data. Place the resource files in one of
these directories (firmware `chdir`s into it before launching):

```text
/sd/MiaOS/Games/lava_pal.app/
/sd/Games/lava_pal.app/
/sd/lava_pal/
```

Required files: `PAT.MKF`, `DATA.MKF`, plus the rest of the standard SDLPAL
resource set (`FBP.MKF`, `MGO.MKF`, `MAP.MKF`, `RNG.MKF`, `GOP.MKF`, ...).
Saved games are read from / written to the same directory (`1.rpg`–`5.rpg`).

## Disabled subsystems

The first build excludes a number of upstream modules that either require
features the ESP32-S3 port does not support or are not yet wired through the
SDL shims:

* `video_glsl.c`, `glslp.c`, `mini_glloader.c` — OpenGL ES shader backend.
* `aviplay.c`, `overlay.c` — AVI playback and on-screen touch overlay.
* `oggplay.c`, `opusplay.c`, `mp3play.c` — compressed music decoders.
* `midi_timidity.c` — Timidity MIDI backend (midi_stubs.c + midi_tsf.c are
  linked instead).

These can be re-enabled incrementally as the matching SDL shims grow.

RIX OPL music is enabled through `rixplay.cpp` and the vendored AdPlug
implementation. A dedicated Core 0 task mixes signed 16-bit PCM and submits it
through the host audio ABI while the game and SDL rendering run on Core 1.

## Notes

* `fontglyph.h` was trimmed: the 2 MB `unicode_font` + 64 KB `font_width`
  tables were moved out of `.data`/`.bss` (which would overflow internal
  DRAM on ESP-IDF 4.4 for ESP32-S3) and are now allocated from PSRAM at
  startup inside `PAL_FontManagerInit`. The original font data is restored
  at runtime by `PAL_LoadISOFont` / `PAL_LOAD_INTERNAL_FONT`.
* lavapal's `sdl_compat.h` redirects SDLPAL's `#include "sdl_compat.h"` to
  the shared `SDL/SDL.h` umbrella header. Configure macros such as
  `PAL_HAS_GLSL`, `PAL_HAS_NATIVEMIDI`, `PAL_HAS_JOYSTICKS` are forced off
  in `main/CMakeLists.txt`.
