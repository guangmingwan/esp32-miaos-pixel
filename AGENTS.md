# Repository Guidelines

## Project Shape

- This is a PlatformIO ESP32-S3 project for ESP32-S3-WROOM-1-N16R8 hardware with 16MB flash, 8MB PSRAM, dual OTA partitions, ILI9342 LCD, SD card, WiFi file apps, and OTA-partition-based SD-loaded apps.
- The main launcher is `env:esp32s3` and uses mixed `framework = arduino, espidf`; `src/main.cpp` provides `app_main()` only when `MIA_ESPIDF_APP_MAIN` is defined.
- `env:esp32s3-usbmsc` is a separate pure-Arduino USB mass-storage firmware from `src_usbmsc/` and is intended for `ota_1`, not the normal launcher image.
- `env:esp32s3-otatest` is a pure ESP-IDF test firmware from `test_ota_esp32s3/`.
- First-party code is mainly in `src/`, `include/`, `src_usbmsc/`, `test_ota_esp32s3/`, and `experiments/`; avoid editing generated/vendor trees such as `.pio/` and `managed_components/` unless the task specifically requires it.

## Commands

- Build the launcher: `pio run -e esp32s3`
- Upload the launcher: `pio run -e esp32s3 -t upload`; override ports with `--upload-port`, do not bake local ports into `platformio.ini`.
- Monitor serial at 115200: `pio device monitor --port <port> -b 115200`
- Build USB MSC firmware: `pio run -e esp32s3-usbmsc`
- Flash USB MSC firmware to `ota_1`: `esptool.py --chip esp32s3 --port <port> -b 921600 write_flash 0x720000 .pio/build/esp32s3-usbmsc/firmware.bin`
- Build a single release image: build both `esp32s3` and `esp32s3-usbmsc`, then merge with fixed offsets from `partitions_dual.csv`:
  `python ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 merge_bin -o esp32-miaos-pixel_<short-git-hash>[-dirty]_esp32-s3-devkit.img --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 .pio/build/esp32s3/bootloader.bin 0x8000 .pio/build/esp32s3/partitions.bin 0x10000 .pio/build/esp32s3/ota_data_initial.bin 0x20000 .pio/build/esp32s3/firmware.bin 0x720000 .pio/build/esp32s3-usbmsc/firmware.bin`
- Flash the merged release image: `python ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 --port <port> -b 921600 write_flash 0x0 esp32-miaos-pixel_<short-git-hash>[-dirty]_esp32-s3-devkit.img`.
- Build OTA test firmware: `pio run -e esp32s3-otatest`
- Build OTA apps with `idf.py build -C experiments/ota_apps/<name>`; the app binary is `experiments/ota_apps/<name>/build/<name>.bin`. Append manifest and copy to the matching SD folder as `<name>.bin`, e.g.:
  ```sh
  python tools/append_manifest.py --input experiments/ota_apps/hello/build/hello.bin --category Application --name hello
  cp experiments/ota_apps/hello/build/hello.bin /sd/Application/hello.app/hello.bin
  ```
- Build all OTA apps: `idf.py build -C experiments/ota_apps/<calculator|minesweeper|rtc_set|sd_browser|diagnostic|screen_test|flashlight|timer|wifi_scan|wifi_files|ftp_server|music>`; after each build, append manifest and copy the `.bin` to `/<category>/<name>.app/<name>.bin`.
- There is no repo-specific lint, formatter, or unit-test command configured; use a focused `pio run -e ...` for verification.
- Do not start background serial monitors or tmux serial listeners. When serial logs are needed, use a foreground, time-bounded capture command and report the captured output.

## VCP File Transfer Host Control

- The built-in app uses the ESP32-S3 USB Serial/JTAG CDC virtual COM port (`/dev/ttyACM*` on Linux), not the GPIO UART. While the launcher is idle, enter VCP File Transfer with `uv run tools/serial_sd_client.py enter --port <port>`. The raw wire command is `SFS1 ENTER\n`; the device replies `SFS1 ENTERING` and then `SFS1 READY` after the app starts.
- Exit VCP File Transfer and return to the launcher with `uv run tools/serial_sd_client.py exit --port <port>`. The raw wire command is `SFS1 EXIT\n`; the device replies `SFS1 EXITING` before running the normal app `end` lifecycle.
- `SFS1 ENTER` is idempotent while VCP File Transfer is already active and replies `SFS1 READY`. Automatic entry is only polled while the launcher is idle; it cannot switch away from another active built-in or OTA app.
- Do not send `SFS1 EXIT` during a `PUT` payload. VCP File Transfer treats all incoming bytes as file data until the declared upload size is received or the upload times out.
- `PUT <path> <size>` uses 6144-byte flow-control windows with an 8192-byte HWCDC RX queue. After `READY`, the host must send at most 6144 bytes and wait for `ACK <total-received>` before sending the next window; the final window returns `OK stored`. Do not restore unacknowledged streaming because the HWCDC RX queue can overflow and silently drop USB packets.
- `GET <path>` replies `DATA <size>`, followed by exactly that many raw bytes, followed by `OK sent`. The host must read the declared byte count before parsing the completion line.
- A focused real-device check is: enter, wait for `SFS1 READY`, send `PING` and require `OK PONG`, perform the desired file operation, then exit and require `SFS1 EXITING`. For transfer changes, round-trip a multi-megabyte nonzero file and compare cryptographic hashes before deleting the device copy.

## Firmware And Partition Gotchas

- `partitions_dual.csv` defines `ota_0` at `0x20000` and `ota_1` at `0x720000`; keep USB MSC flashing and OTA switching aligned with those offsets.
- For merged release images, do not trust generated `flash_args` if it places the app at `0x10000`; explicitly place launcher `firmware.bin` at `ota_0` (`0x20000`) and USB MSC `firmware.bin` at `ota_1` (`0x720000`). Name release images like `esp32-miaos-pixel_2739f43-dirty_esp32-s3-devkit.img`, using `-dirty` when `git status --porcelain` is non-empty.
- Launcher and USB MSC code manually writes `otadata` instead of using `esp_ota_set_boot_partition()` because image verification previously triggered TG1 WDT on this ESP32-S3 setup.
- Do not restore `otadata` by blindly flashing the all-`0xFF` `ota_data_initial.bin`. On a device that still has an older partition table, `0x10000` may be a factory app rather than `otadata`; writing there destroys the app header and causes a boot loop in bootloader `unpack_load_app()`. Before offset-based recovery, read the device table from `0x8000` and decode it with `gen_esp32part.py`, or synchronize the complete dual-OTA layout as described below.
- To migrate/recover a device whose on-flash partition table does not match `partitions_dual.csv`, build both images and write the matching bootloader, partition table, initial OTA data, and app slots together:
  `python ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 --port <port> -b 921600 write_flash 0x0 .pio/build/esp32s3/bootloader.bin 0x8000 .pio/build/esp32s3/partitions.bin 0x10000 .pio/build/esp32s3/ota_data_initial.bin 0x20000 .pio/build/esp32s3/firmware.bin 0x720000 <ota_1-app.bin>`.
  This leaves NVS at `0x9000` intact because only the listed ranges are erased.
- After the dual-OTA partition table is installed, restore boot selection to launcher/`ota_0` with the host-side ESP-IDF OTA tool, which generates valid CRC-bearing OTA select entries instead of leaving the partition erased:
  `source /opt/esp-idf/export.sh && python /opt/esp-idf/components/app_update/otatool.py --port <port> --baud 921600 --partition-table-file .pio/build/esp32s3/partitions.bin switch_ota_partition --slot 0`.
  The equivalent firmware-side convention is `seq=1` and `seq=3` with `ESP_OTA_IMG_VALID`. Verify recovery with a foreground, time-bounded serial capture; successful launcher boot reaches `ESP32-S3 Retro-Pixel launcher` / `[setup]` logs rather than repeating `Saved PC ... unpack_load_app` resets.
- `esp_image_get_metadata()` can also trigger TG1 WDT on this board when reading a partition with invalid/corrupted data. Wrap calls with `ScopedIntWdtPause` (from `include/int_wdt_guard.h`) when it may be called on unverified partition data — e.g. `miaReadOtaManifest()` and `miaExportAppSlotToSd()` in `src/ota_app_flash.cpp`.
- USB MSC must keep `ARDUINO_USB_MODE=1` so TinyUSB owns CDC+MSC on the single USB D+/D- pair; switching it back can cause USB protocol conflicts.
- USB MSC exits back to launcher with `SELECT + START` and writes `seq=1, seq=3` for `ota_0`; launcher writes the matching `ota_1` entries before rebooting to USB Disk.
- `sdkconfig.defaults` disables task WDT and previously enabled ELF loader options (now disabled — OTA partition mode replaces ELF loader); `sdkconfig.esp32s3` and `dependencies.lock` are generated/ignored locally, so avoid treating local diffs there as source edits.

## Droid GBK Font Rendering Gotchas

- `fontDroidGbk12` is a variable-length font containing printable ASCII plus GBK Chinese glyphs. Its generated GBK offset table does not index ASCII, so never resolve ASCII by repeatedly scanning the font blob from the beginning; launcher text measurement and drawing amplify that into a long render that can trigger the 300 ms TG1 interrupt WDT.
- Keep printable ASCII U+0020 through U+007E on the compile-time `DROID_ASCII_OFFSETS` O(1) path in `src/lava_text.cpp`. Chinese characters use Unicode-to-GBK lookup followed by `DROID_GBK12_GLYPH_OFFSETS`. Do not add a mutable runtime glyph cache or fall back to `fontBasic8x8` for Droid ASCII.
- U+0020 space is a valid zero-bitmap Droid glyph (`width=0`, `height=0`, `xDelta=3`). Treat it as advance-only instead of rejecting it as malformed or drawing a fallback box.
- `drawLauncher()` pauses TG1 WDT with `ScopedIntWdtPause`. While `g_launcherDrawing` is true, the project `esp32_task_wdt_reset()` yield shim must not call `delay(1)` from inside font decoding; the launcher render scope suppresses that delay and restores normal yielding after the frame completes.
- Do not hide this class of bug by increasing the interrupt-WDT timeout. Keep the ESP-IDF default 300 ms timeout, optimize glyph lookup, and bound/validate glyph offsets and bitmap dimensions.
- Serial logging changes rendering timing and previously masked the failure. For timing-sensitive renderer diagnosis, use a foreground time-bounded serial capture for reset causes and RTC-retained numeric breadcrumbs for the last render stage; remove all breadcrumbs after locating the fault.

## App And OTA App Boundaries

- Built-in launcher apps implement `LauncherApp` from `include/app.h`; add new built-ins by adding the module/header and registering it in the `BUILTIN_APPS` array in `src/main.cpp`.
- App button context exposes six logical buttons `{A, B, UP, DN, LT, RT}` mapped from the full physical table in `include/pins.h`; use `g_allButtons` only for launcher/system-level controls.
- SD apps are discovered from root category directories such as `/Games`, `/Utils`, `/Settings`, `/Emulators`, `/Media`, `/Application`, plus legacy `/MiaOS/{Games,Utils,Settings,Emulators,Media,Application}`; each app lives at `<category>/<name>.app/<name>.bin` (firmware filename matches app name, not `firmware.bin`).
- Firmware binaries carry an `OtaAppManifest` trailer (see `include/ota_app_manifest.h`) containing `category[16]` + `name[32]` + CRC; use `tools/append_manifest.py` to attach it after `idf.py build`.
- The System tab includes "Export OTA to SD" which reads the manifest from `ota_1` and auto-creates the SD directory and `<name>.bin` — bootstraps apps flashed directly to `ota_1`.
- When launching an SD app, the launcher compares the SD manifest with `ota_1`'s manifest using `sdManifestMatchesOta()`; if they match, the flash step is skipped and the system reboots immediately (optimization).
- `include/ota_app_flash.h` provides `miaExportOtaToSd()` (auto-path export) and `miaBootAppSlot()` (reboot without re-flash).
- `src/ota_app_flash.cpp` appends the manifest trailer when exporting from `ota_1` to SD, preserving round-trip fidelity.
- Host ABI symbols are declared in `include/mia_host_abi.h`; OTA apps are not sandboxed and must be rebuilt against matching host ABI changes.
