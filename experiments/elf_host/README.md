# MiaOS ELF Host Experiment

This is an ESP-IDF-only proof of concept for loading native ESP32 ELF apps from
the SD card. It is intentionally separate from the root Arduino firmware.

## Host Firmware

Build the host firmware:

```sh
pio run
```

The host mounts the SD card over SDSPI using the board pins already used by the
Arduino firmware:

| Signal | GPIO |
| --- | --- |
| SCK | 18 |
| MOSI | 23 |
| MISO | 19 |
| CS | 22 |

At boot, the host scans:

```text
/sdcard/MiaOS/Application/*.app/app.elf
```

It loads the first app it finds, registers the current experimental host ABI,
relocates the ELF with `esp_elf_relocate`, and runs it with `esp_elf_request`.

## Sample App

Build the sample ELF app from the workspace root:

```sh
pio run -d experiments/elf_apps/hello
```

The generated app artifact is:

```text
experiments/elf_apps/hello/.pio/build/esp32dev/hello.app.elf
```

Copy it to the SD card as:

```text
/MiaOS/Application/hello.app/app.elf
```

The sample app calls these host-provided symbols:

```c
uint32_t mia_host_abi_version(void);
void mia_host_log(const char *message);
```

Those symbols must stay undefined in the ELF file; the host resolves them at
runtime through the `esp_elf_register_symbol` table.

## Current Limits

- This does not integrate with the root Arduino launcher yet.
- There is no UI app picker; the host runs the first `*.app/app.elf` it finds.
- The host ABI is experimental and currently only supports logging and an ABI
  version check.
- Native ELF apps run in the same address space as the firmware. A bad app can
  crash or corrupt the host.
