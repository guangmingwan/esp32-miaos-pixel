# Gmu OTA App

This is the Gmu Music Player (`jhe2/gmu`, branch `sdl20`) adapted to the
MiaOS ESP32-S3 OTA app ABI.

The Gmu core, playlist, file player, configuration, event queue, and ring
buffer remain in `vendor/gmu-src`. The SDL frontend and SDL audio backend are
replaced by `main/gmu_frontend.c` and `main/gmu_audio.c`, which use the shared
MiaOS host display, input, SD, and I2S APIs.

The first OTA port statically includes the minimp3 decoder. It currently plays
MP3/MP2 files from the SD card. HTTP streaming, SDL themes, cover artwork,
dynamic plugins, and the original SDL frontend are intentionally not enabled.

Build and append the OTA manifest:

```sh
idf.py -C experiments/ota_apps/gmu build
python tools/append_manifest.py \
  --input experiments/ota_apps/gmu/build/gmu.bin \
  --category Media --name gmu
```

Install as `/MiaOS/Media/gmu.app/gmu.bin` on the SD card. The app returns to
the launcher with `SELECT + START`.
