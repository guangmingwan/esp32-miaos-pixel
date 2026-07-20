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

### Common `serial_sd_client.py` commands

- Default Linux port is `/dev/ttyACM0`; override it with `--port <port>`. Use one serial client at a time. Do not run `enter`, `put`, `get`, or `exit` concurrently on the same port.
- Enter/exit VCP:
  ```sh
  uv run tools/serial_sd_client.py enter --port <port>
  uv run tools/serial_sd_client.py exit --port <port>
  ```
- Check the protocol and device:
  ```sh
  uv run tools/serial_sd_client.py ping --port <port>
  uv run tools/serial_sd_client.py info --port <port>
  ```
- List directories. The path is passed with `--remote-path`, not as a positional argument:
  ```sh
  uv run tools/serial_sd_client.py list-dir --remote-path /MiaOS/Library --port <port>
  ```
- Create/delete/rename remote paths:
  ```sh
  uv run tools/serial_sd_client.py mkdir /MiaOS/Library --port <port>
  uv run tools/serial_sd_client.py delete /MiaOS/Library/file.bin --port <port>
  uv run tools/serial_sd_client.py rename /old/name.bin /new/name.bin --port <port>
  ```
- Upload/download files. `put` and `get` use `LOCAL_PATH REMOTE_PATH` and `REMOTE_PATH LOCAL_PATH` order respectively:
  ```sh
  uv run tools/serial_sd_client.py put local.bin /MiaOS/Library/remote.bin --port <port>
  uv run tools/serial_sd_client.py get /MiaOS/Library/remote.bin /tmp/remote.bin --port <port>
  ```
- Direct-launch an SD app with an optional file argument. The helper uses `--file-path` and waits for the launch response; the device then reboots, so the command does not return to VCP:
  ```sh
  uv run tools/serial_sd_client.py launch /MiaOS/Emulators/gba.app/gba.bin \
      --file-path /roms/gba/game.gba --port <port>
  ```
- After `esptool.py`, `otatool.py`, or `launch`, USB Serial/JTAG may disappear briefly. Wait for `/dev/ttyACM*` to reappear before opening the next client. If a helper misses startup output, use a single foreground raw serial session to wait for `SFS1 READY`; never open a second client in parallel.
- VCP paths are raw byte paths. Keep paths ASCII where possible. Launcher-side Chinese paths require GBK bytes; OTA app paths use UTF-8 as described in the filename encoding rules below.

## SD Filename Encoding

- The parent launcher is built with `CONFIG_FATFS_CODEPAGE_936=y` and `CONFIG_FATFS_API_ENCODING_ANSI_OEM=y`. Any Chinese path passed to launcher-side Arduino `SD.open`, `SD.remove`, `SD.rename`, or related APIs must be encoded as GBK bytes; sending UTF-8 bytes creates names that the launcher may list but UTF-8 OTA apps cannot open.
- OTA apps under `experiments/ota_apps/` use `CONFIG_FATFS_API_ENCODING_UTF_8=y`. Their `readdir`, `fopen`, and storage-picker paths are UTF-8. Do not reuse launcher-side GBK conversion in an OTA app.
- VCP commands carry raw bytes. ASCII paths are safe everywhere. For Chinese device filenames, use a GBK-aware host operation or an app compiled with the matching UTF-8 FATFS configuration; do not assume `tools/serial_sd_client.py rename` can safely encode a Chinese destination by itself.
- `RUN <app>\t<file>` keeps its file argument in UTF-8 for the OTA app. The launcher validates the app path only and must pass the optional file argument through without calling its ANSI/OEM `SD.open` on that path.
- The static web repository `experiments/ota_apps/bbk-games/roms/*.lib` is a separate source/data repository. Never rename or edit those local files to repair device SD names; device-side SD renames are an independent deployment operation.

## VCP Direct Launch

- The VCP service accepts `RUN` after the file-transfer commands. `RUN <app-bin-path>` queues an SD OTA app launch; `RUN <app-bin-path>\t<file-path>` additionally passes one SD file path to the app after reboot. The tab separator is required when either path may contain spaces.
- Example: `RUN /MiaOS/Emulators/nes.app/nes.bin\t/roms/FC/game.nes`. The launcher validates the app path, preserves the optional UTF-8 file argument in a one-shot `/MiaOS/.launch` context, flashes the app to `ota_1`, selects `ota_1`, and reboots. The target app consumes and removes the context on startup.
- `RUN BUILTIN\tabout`, `RUN BUILTIN\tlogs`, and `RUN BUILTIN\tvcp` switch directly between launcher built-in apps without rebooting. Built-in apps ignore the optional file argument.
- The host helper supports this through `uv run tools/serial_sd_client.py launch <app-bin-path> [--file-path <file-path>]`; it enters VCP automatically, sends the request, and then the device reboots into the selected app.
- The direct file argument is consumed by the shared emulator picker path and by `music`, so NES/FC and other emulator ROMs can bypass the on-device picker. Apps that do not consume an argument still launch normally.

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
- The ESP-IDF OTA `common_host` HC165 scan is active-high on this board: real-device traces show idle raw state `0x00`, a pressed key sets its corresponding bit to `1`, and release returns it to `0`. OTA apps that compile `experiments/ota_apps/common_host/host_platform.cpp` must define `MIA_HC165_ACTIVE_HIGH=1`; omitting it treats every idle HC165 key as held and causes symptoms such as D-pad movement continuing after release.
- HC165 bit assignments are `LEFT=0`, `DOWN=1`, `UP=2`, `RIGHT=3`, `Y=4`, `X=5`, `A=6`, `B=7`. When diagnosing input, log raw-state transitions only long enough to confirm both press and release, then remove the timing-changing trace.
- SD apps are discovered from root category directories such as `/Games`, `/Utils`, `/Settings`, `/Emulators`, `/Media`, `/Application`, plus legacy `/MiaOS/{Games,Utils,Settings,Emulators,Media,Application}`; each app lives at `<category>/<name>.app/<name>.bin` (firmware filename matches app name, not `firmware.bin`).
- Firmware binaries carry an `OtaAppManifest` trailer (see `include/ota_app_manifest.h`) containing `category[16]` + `name[32]` + CRC; use `tools/append_manifest.py` to attach it after `idf.py build`.
- The System tab includes "Export OTA to SD" which reads the manifest from `ota_1` and auto-creates the SD directory and `<name>.bin` — bootstraps apps flashed directly to `ota_1`.
- When launching an SD app, the launcher compares the SD manifest with `ota_1`'s manifest using `sdManifestMatchesOta()`; if they match, the flash step is skipped and the system reboots immediately (optimization).
- `include/ota_app_flash.h` provides `miaExportOtaToSd()` (auto-path export) and `miaBootAppSlot()` (reboot without re-flash).
- `src/ota_app_flash.cpp` appends the manifest trailer when exporting from `ota_1` to SD, preserving round-trip fidelity.
- Host ABI symbols are declared in `include/mia_host_abi.h`; OTA apps are not sandboxed and must be rebuilt against matching host ABI changes.
