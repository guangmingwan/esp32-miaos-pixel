# Emulator Vendor Provenance

This directory contains unmodified upstream source copied from `/home/netwan/myprojects/esp32-retro-go-pixel` for the OTA emulator app lane.

| Destination | Origin | SHA256 |
| --- | --- | --- |
| `retro-core/main/main_gbc.c` | `retro-core/main/main_gbc.c` | `4af69cc37d5cef36d9ae3a270bf82a813f515cbb2867be0e00fcbfbb30cdf460` |
| `retro-core/main/main_gw.c` | `retro-core/main/main_gw.c` | `ac4c74642e952d1da366f5085b190f16ee29f19be694791eb39b7889110ee571` |
| `retro-core/main/shared.h` | `retro-core/main/shared.h` | `2510434adc554870ea93f628962686b49c1c06f0f0a94288664f000b8cdbf66e` |
| `retro-core/components/gnuboy/` | `retro-core/components/gnuboy/` | directory copied with license `licenses/gnuboy_COPYING`; upstream `tests/` archive payload excluded |
| `retro-core/components/gw-emulator/` | `retro-core/components/gw-emulator/` | directory copied with license `licenses/gw-emulator_LICENSE` |
| `retro-core/components/smsplus/` | `retro-core/components/smsplus/` at `2166f356a3d6c3b13206a26a51e9def396279d24` | engine sources copied; `COPYING` SHA256 `91df39d1816bfb17a4dda2d3d2c83b1f6f2d38d53e53e41e8f97ad5ac46a0cad` |
| `retro-core/components/smsplus/coleco_bios.h` | `retro-core/components/smsplus/coleco_bios.h` | `f89eb6f82a6578b85749cd458bc1b907f6fb693b6979e35cf8e0549f2bed1d17` |
| `gbsp/components/gbsp-libretro/` | `2c6e1f22:gbsp/components/gbsp-libretro/` | commit `2c6e1f22e4cec6a2743646244be21b1bc8643c63`; tree `f5866371c5f69552925634977ecceafa53245d6c`; selective archive SHA256 `2e065108e2444886213da06b6c5aa6876c752646ab432df5750f8fad40b1464a` |

The `gb` and `gbc` OTA apps share the `gnuboy` upstream core source and link its core C files. The `gw` OTA app uses the separate `gw-emulator` upstream source staged for this lane. No ROM, launcher, updater, network, cover, screenshot, or ZIP payload files are vendored for this task.

The `sms`, `gg`, and `coleco` apps share SMS Plus. The upstream patched `coleco_bios.h` is included and linked by `loadrom.c`, matching the original Retro-Go build. `shared.h` maps the upstream Retro-Go CRC helper to ESP-IDF's ROM CRC API so the game compatibility database remains active outside Retro-Go. The upstream launcher `main_sms.c` is reference-only and is not linked or vendored.

The GBA vendor group contains only top-level gpSP core C/C++ sources and headers, GPL/readme documents, and the two libretro API headers required by `input.h`. Platform backends, libretro frontend glue, network helpers, tools, tests, `bios/open_gba_bios.bin`, and generated `main/bios.h` are excluded. The app requires the user's canonical 16 KiB BIOS at `/bios/gba/gba_bios.bin` on the SD card (mounted as `/sd/bios/gba/gba_bios.bin` by the app), validated by MD5 `a860e8c0b6d573d191e4ec7db1b1e4f6`; no built-in BIOS fallback is linked.
