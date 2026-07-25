# MiaOS Build Manifest

本文档是发布产物的构建清单。`tools/build_sd.py` 中的 `APPS` 是 SD 发布包清单；两者职责不同：构建清单规定使用哪套工具链生成输入产物，打包清单只读取已有产物、刷新 MIA2 Manifest 并生成 ZIP。

## 固定工具链

### PIO44: PlatformIO 固件

| 组件 | 固定/验证版本 |
| --- | --- |
| PlatformIO Core | `6.1.19`（当前验证版本，最低使用 6.1 系列） |
| Platform | `platformio/espressif32@7.0.1`（由 `platformio.ini` 固定） |
| ESP-IDF | `4.4.7`（PlatformIO 包 `framework-espidf@3.40407.240606`） |
| Arduino-ESP32 | `2.0.17`（PlatformIO 包 `framework-arduinoespressif32@3.20017.241212`） |
| Xtensa GCC | `8.4.0`, `esp-2021r2-patch5` |
| esptool.py | PlatformIO 包 `tool-esptoolpy@2.41100.0` |

PIO44 只用于根目录 [`platformio.ini`](../platformio.ini) 中的三个环境，不要用它构建独立 OTA app。

### IDF44: 常规 OTA 固件

| 组件 | 固定版本 |
| --- | --- |
| ESP-IDF | `v4.4.8` |
| Xtensa GCC | `8.4.0`, `esp-2021r2-patch5` |
| CMake | `>=3.16` |
| Managed component | `espressif/cmake_utilities@0.5.3` |
| Managed component | `espressif/elf_loader@1.3.0`（使用共享字库/SDL 的项目） |

各项目的 `dependencies.lock` 是 IDF44 版本的最终依据。构建前应先激活同一套环境：

```sh
source /opt/esp-idf-4.4.8/export.sh
idf.py --version
xtensa-esp32s3-elf-gcc --version
```

版本输出应分别为 `ESP-IDF v4.4.8` 和 GCC `8.4.0`。本机若把该版本安装在 `/opt/esp-idf`，相应调整路径即可。

### IDF52: PSRAM/USB 新版固件

| 组件 | 固定版本 |
| --- | --- |
| ESP-IDF | `v5.2.6` |
| Xtensa GCC | `13.2.0`, `esp-13.2.0_20250707` |
| CMake | `>=3.16` |

`lava_cch` 依赖 ESP32-S3 external-BSS-to-PSRAM；该 Kconfig 能力在 IDF 4.4 对 ESP32-S3 不可用。`gmu` 和 `usb_wifi` 使用 IDF 5 的独立 `esp_partition` / `esp_driver_*` 组件以及新版 POSIX/TinyUSB API。因此这三个项目必须使用 IDF52，不能放进 IDF44 的批量构建。

```sh
source /opt/esp-idf-5.2.6/export.sh
idf.py --version
xtensa-esp-elf-gcc --version
```

版本输出应分别为 `ESP-IDF v5.2.6` 和 `esp-13.2.0_20250707`。不要让 IDF44 与 IDF52 共用同一个项目的 `build/` 或生成的 `sdkconfig`；切换 major 版本时使用干净构建目录，并从 `sdkconfig.defaults` 重新生成配置。

## 产物清单

### PlatformIO

| 产物 | 工具链 | 构建命令 | 输出 |
| --- | --- | --- | --- |
| Launcher | PIO44 | `pio run -e esp32s3` | `.pio/build/esp32s3/firmware.bin` |
| USB Disk | PIO44 | `pio run -e esp32s3-usbmsc` | `.pio/build/esp32s3-usbmsc/firmware.bin` |
| OTA test | PIO44 | `pio run -e esp32s3-otatest` | `.pio/build/esp32s3-otatest/firmware.bin` |

OTA test 是开发验证固件，不进入 SD 发布包或合并发布镜像。

### IDF44 基础产物

| 产物 | 构建命令 | 输出 |
| --- | --- | --- |
| Startup recovery menu | `idf.py -C experiments/startup_menu build` | `experiments/startup_menu/build/startup_menu.bin` |
| Text shared library | `idf.py -C experiments/shared_libraries/mia_text so` | `experiments/shared_libraries/mia_text/build/libmia_text_v1.so` |
| SDL shared library | `idf.py -C experiments/shared_libraries/mia_sdl so` | `experiments/shared_libraries/mia_sdl/build/libmia_sdl_v1.so` |

### IDF44 OTA apps

以下项目统一使用：

```sh
idf.py -C experiments/ota_apps/<project> build
```

| 分类 | `<project>` / 输出名 |
| --- | --- |
| Application | `hello` |
| Emulators | `coleco`, `gb`, `gba`, `gbc`, `gg`, `gw`, `lynx`, `megadrive`, `msx`, `nes`, `pce`, `sms`, `snes` |
| Games | `lavapal` -> `build/lava_pal.bin`, `minesweeper` |
| Media | `music` |
| Settings | `settings` |
| Utils | `calculator`, `diagnostic`, `flashlight`, `ftp_server`, `screen_test`, `sd_browser`, `timer`, `wifi_files`, `wifi_scan` |

除 `lavapal` 外，输出均为 `experiments/ota_apps/<project>/build/<project>.bin`。

### IDF52 OTA apps

| 分类 | 项目 | 构建命令 | 输出 |
| --- | --- | --- | --- |
| Games | `lava_cch` | `idf.py -C experiments/ota_apps/lava_cch build` | `experiments/ota_apps/lava_cch/build/lava_cch.bin` |
| Media | `gmu` | `idf.py -C experiments/ota_apps/gmu build` | `experiments/ota_apps/gmu/build/gmu.bin` |
| System | `usb_wifi` | `idf.py -C experiments/ota_apps/usb_wifi build` | `experiments/ota_apps/usb_wifi/build/usb_wifi.bin` |

## SD 运行时依赖

`tools/build_sd.py` 除固件外还处理以下可分发依赖：

| 应用 | SD 路径 | 来源 |
| --- | --- | --- |
| 所有共享字库应用 | `/MiaOS/Library/libmia_text_v1.so` | IDF44 text shared library |
| `lava_pal` 等 SDL 应用 | `/MiaOS/Library/libmia_sdl_v1.so` | IDF44 SDL shared library |
| `lava_cch` | `/MiaOS/Games/lava_cch.app/LavaData/BOOK.DAT` | `experiments/ota_apps/lava_cch/LavaData/BOOK.DAT` |
| `lava_pal` | `/MiaOS/Games/lava_pal.app/*` | `experiments/ota_apps/lavapal/resource/` 中的全部文件 |

`lava_pal` 的 GB2312 转换数据和资源说明随仓库提供。PAL 原版游戏数据受版权保护，不进入仓库；使用者需从自己合法拥有的游戏介质提取到 `experiments/ota_apps/lavapal/resource/`。打包脚本会收录该目录的全部文件；缺少必需原版文件时显示警告、跳过缺失项并继续生成 ZIP。完整清单见该目录的 `README.md`。

模拟器 ROM、音乐文件同样属于用户数据，不进入发布包。

## 推荐构建顺序

1. 激活 PIO44，构建 `esp32s3`、`esp32s3-usbmsc`；需要测试固件时再构建 `esp32s3-otatest`。
2. 激活 IDF44，构建 startup menu、`mia_text`、`mia_sdl` 和全部 IDF44 OTA apps。
3. 关闭 IDF44 shell 或重新开 shell，激活 IDF52，构建 `lava_cch`、`gmu`、`usb_wifi`。
4. 回到仓库根目录运行 `python tools/build_sd.py`。脚本不会调用编译器；缺少固件时会显示 `Skipped`，缺少共享库或已登记的运行时数据时会失败。
5. 检查打包摘要中不存在意外的 `Skipped`，再发布生成的 ZIP。

`tools/build_sd.py` 需要 Python `>=3.10`。MIA2 的 `build_epoch` 默认取当前时间；可设置 `SOURCE_DATE_EPOCH` 或传入 `--build-epoch` 生成可复现的统一时间戳。
