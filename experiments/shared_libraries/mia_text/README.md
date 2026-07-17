# MiaOS Text Shared Library

This ESP32-S3 shared object contains the Droid GBK font, Unicode-to-GBK map,
glyph offset index, and indexed-framebuffer text renderer.

Build it with the same ESP-IDF toolchain used by the OTA apps:

```sh
idf.py so
```

The output is `build/libmia_text_v1.so`. Install it on the SD card as:

```text
/MiaOS/Library/libmia_text_v1.so
```

Apps load the library through `MiaTextApi` from `include/mia_text_api.h`. The
loader is pinned to `espressif/elf_loader` 1.3.0 because 1.3.1 requires newer
C11 atomic support than the repository's ESP-IDF 4.4.8 toolchain provides.

Shared-font apps must mount the SD card before calling `display_host_init()`.
Use `mia_text_runtime` for symbol lookup: on ESP32-S3 with PSRAM loading,
elf_loader 1.3.0 returns the D-bus alias from `dlsym()`, and the runtime maps
function symbols to the corresponding executable I-bus address.
