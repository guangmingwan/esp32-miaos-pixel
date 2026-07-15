# Lava PAL OTA app

This ESP-IDF project builds the Lava-compatible PAL port for the MiaOS Pixel
launcher.

## Build

```sh
source /opt/esp-idf/export.sh
make manifest
```

The resulting OTA image is `build/lava_pal.bin`. The app manifest uses the
`Games/lava_pal` identity.

## SD card data

The firmware does not include PAL game data. Put the required PAL resource
files and `lava_pal.bin` in one of these directories:

```text
/MiaOS/Games/lava_pal.app/
/Games/lava_pal.app/
/lava_pal/
```

Saved games are read from and written to `1.rpg` through `5.rpg` in the same
directory. Existing saves must be copied there before starting the game.
