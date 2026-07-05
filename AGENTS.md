# Repository Guidelines

## Project Shape

- This is a PlatformIO ESP32-S3 project for ESP32-S3-WROOM-1-N16R8 hardware with 16MB flash, 8MB PSRAM, dual OTA partitions, ILI9342 LCD, SD card, WiFi file apps, and experimental SD-loaded ELF apps.
- The main launcher is `env:esp32s3` and uses mixed `framework = arduino, espidf`; `src/main.cpp` provides `app_main()` only when `MIA_ESPIDF_APP_MAIN` is defined.
- `env:esp32s3-usbmsc` is a separate pure-Arduino USB mass-storage firmware from `src_usbmsc/` and is intended for `ota_1`, not the normal launcher image.
- `env:esp32s3-otatest` is a pure ESP-IDF test firmware from `test_ota_esp32s3/`.
- First-party code is mainly in `src/`, `include/`, `src_usbmsc/`, `test_ota_esp32s3/`, and `experiments/`; avoid editing generated/vendor trees such as `.pio/`, `managed_components/`, and `components/elf_loader/` unless the task specifically requires it.

## Commands

- Build the launcher: `pio run -e esp32s3`
- Upload the launcher: `pio run -e esp32s3 -t upload`; override ports with `--upload-port`, do not bake local ports into `platformio.ini`.
- Monitor serial at 115200: `pio device monitor --port <port> -b 115200`
- Build USB MSC firmware: `pio run -e esp32s3-usbmsc`
- Flash USB MSC firmware to `ota_1`: `esptool.py --chip esp32s3 --port <port> -b 921600 write_flash 0x720000 .pio/build/esp32s3-usbmsc/firmware.bin`
- Build a single release image: build both `esp32s3` and `esp32s3-usbmsc`, then merge with fixed offsets from `partitions_dual.csv`:
  `python ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 merge_bin -o retro-go_<short-git-hash>[-dirty]_esp32-s3-devkit.img --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 .pio/build/esp32s3/bootloader.bin 0x8000 .pio/build/esp32s3/partitions.bin 0x10000 .pio/build/esp32s3/ota_data_initial.bin 0x20000 .pio/build/esp32s3/firmware.bin 0x720000 .pio/build/esp32s3-usbmsc/firmware.bin`
- Flash the merged release image: `python ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 --port <port> -b 921600 write_flash 0x0 retro-go_<short-git-hash>[-dirty]_esp32-s3-devkit.img`.
- Build OTA test firmware: `pio run -e esp32s3-otatest`
- Build the sample SD ELF app: `pio run -d experiments/elf_apps/hello`; its artifact is `experiments/elf_apps/hello/.pio/build/esp32s3/app.elf`.
- Build the ESP32-S3 SD ELF test app: `pio run -d experiments/elf_apps/mia_test`; its artifact is `experiments/elf_apps/mia_test/.pio/build/esp32s3/app.elf`.
- Build SD ELF apps with `pio run -d experiments/elf_apps/<calculator|minesweeper|rtc_set|sd_browser|diagnostic|screen_test|flashlight|timer|wifi_scan|wifi_files|ftp_server>`; copy each `.pio/build/esp32s3/app.elf` to the matching SD folder, e.g. `/Utils/calculator.app/app.elf`.
- There is no repo-specific lint, formatter, or unit-test command configured; use a focused `pio run -e ...` for verification.
- Do not start background serial monitors or tmux serial listeners. When serial logs are needed, use a foreground, time-bounded capture command and report the captured output.

## Firmware And Partition Gotchas

- `partitions_dual.csv` defines `ota_0` at `0x20000` and `ota_1` at `0x720000`; keep USB MSC flashing and OTA switching aligned with those offsets.
- For merged release images, do not trust generated `flash_args` if it places the app at `0x10000`; explicitly place launcher `firmware.bin` at `ota_0` (`0x20000`) and USB MSC `firmware.bin` at `ota_1` (`0x720000`). Name release images like `retro-go_2739f43-dirty_esp32-s3-devkit.img`, using `-dirty` when `git status --porcelain` is non-empty.
- Launcher and USB MSC code manually writes `otadata` instead of using `esp_ota_set_boot_partition()` because image verification previously triggered TG1 WDT on this ESP32-S3 setup.
- USB MSC must keep `ARDUINO_USB_MODE=1` so TinyUSB owns CDC+MSC on the single USB D+/D- pair; switching it back can cause USB protocol conflicts.
- USB MSC exits back to launcher with `SELECT + START` and writes `seq=1, seq=3` for `ota_0`; launcher writes the matching `ota_1` entries before rebooting to USB Disk.
- FatFs working/context allocations must stay in internal SRAM, not PSRAM. On this ESP32-S3 stack, placing FatFs allocations in PSRAM can trigger watchdog timeout, crash, and reboot during mount or later filesystem writes.
- SD fix method: when using ESP-IDF FatFs on this project, force `ff_memalloc()` in `components/fatfs/port/freertos/ffsystem.c` to allocate with `heap_caps_malloc(msize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)` instead of a PSRAM-capable/default heap path. Treat this as the required mount-stability fix unless the allocator behavior is reworked and re-verified on hardware.
- `sdkconfig.defaults` disables task WDT and enables ELF loader options; `sdkconfig.esp32s3` and `dependencies.lock` are generated/ignored locally, so avoid treating local diffs there as source edits.

## App And ELF Boundaries

- Built-in launcher apps implement `LauncherApp` from `include/app.h`; add new built-ins by adding the module/header and registering it in the `BUILTIN_APPS` array in `src/main.cpp`.
- App button context exposes six logical buttons `{A, B, UP, DN, LT, RT}` mapped from the full physical table in `include/pins.h`; use `g_allButtons` only for launcher/system-level controls.
- SD apps are discovered from root category directories such as `/Games`, `/Utils`, `/Settings`, `/Emulators`, `/Media`, `/Application`, plus legacy `/MiaOS/{Games,Utils,Settings,Emulators,Media,Application}`; each app lives at `<category>/<name>.app/app.elf`.
- Host ABI symbols are declared in `include/mia_host_abi.h`; ELF apps are not sandboxed and must be rebuilt against matching host ABI changes.
