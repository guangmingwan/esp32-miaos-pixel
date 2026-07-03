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
- Build OTA test firmware: `pio run -e esp32s3-otatest`
- Build the sample SD ELF app: `pio run -d experiments/elf_apps/hello`; its artifact is `experiments/elf_apps/hello/.pio/build/esp32dev/hello.app.elf`.
- Build the ESP32-S3 SD ELF test app: `pio run -d experiments/elf_apps/mia_test`; its artifact is `experiments/elf_apps/mia_test/.pio/build/esp32s3/app.elf`.
- Build SD versions of former built-ins with `pio run -d experiments/elf_apps/<calculator|minesweeper|rtc_set|sd_browser>`; copy each `.pio/build/esp32s3/app.elf` to the matching SD folder, e.g. `/Utils/calculator.app/app.elf`.
- There is no repo-specific lint, formatter, or unit-test command configured; use a focused `pio run -e ...` for verification.

## Firmware And Partition Gotchas

- `partitions_dual.csv` defines `ota_0` at `0x20000` and `ota_1` at `0x720000`; keep USB MSC flashing and OTA switching aligned with those offsets.
- Launcher and USB MSC code manually writes `otadata` instead of using `esp_ota_set_boot_partition()` because image verification previously triggered TG1 WDT on this ESP32-S3 setup.
- USB MSC must keep `ARDUINO_USB_MODE=1` so TinyUSB owns CDC+MSC on the single USB D+/D- pair; switching it back can cause USB protocol conflicts.
- USB MSC exits back to launcher with `SELECT + START` and writes `seq=1, seq=3` for `ota_0`; launcher writes the matching `ota_1` entries before rebooting to USB Disk.
- `sdkconfig.defaults` disables task WDT and enables ELF loader options; `sdkconfig.esp32s3` and `dependencies.lock` are generated/ignored locally, so avoid treating local diffs there as source edits.

## App And ELF Boundaries

- Built-in launcher apps implement `LauncherApp` from `include/app.h`; add new built-ins by adding the module/header and registering it in the `BUILTIN_APPS` array in `src/main.cpp`.
- App button context exposes six logical buttons `{A, B, UP, DN, LT, RT}` mapped from the full physical table in `include/pins.h`; use `g_allButtons` only for launcher/system-level controls.
- SD apps are discovered from root category directories such as `/Games`, `/Utils`, `/Settings`, `/Emulators`, `/Media`, `/Application`, plus legacy `/MiaOS/Application`; each app lives at `<category>/<name>.app/app.elf`.
- Host ABI symbols are declared in `include/mia_host_abi.h`; ELF apps are not sandboxed and must be rebuilt against matching host ABI changes.
