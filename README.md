# ESP32-S3 Retro-Pixel Launcher

面向 **ESP32-S3-WROOM-1-N16R8**（16MB Flash + 8MB PSRAM）掌机的启动器与 SD 卡应用平台。硬件使用 2.31 英寸 320x240 IPS 屏（HD231005C10 / ILI9342）、独立 SPI SD 卡、PCF8563 RTC、NS4168 I2S 功放、USB OTG、方向键、ABXY 和肩键/系统键。

Launcher 提供中英文界面、可切换位图字体、RTC 时钟、日志、USB CDC 文件传输，并可将 SD 卡上的独立 ESP-IDF 固件写入 `ota_1` 后启动。SD 应用不是 ELF 插件，也没有沙箱；它们是拥有完整硬件权限的固件镜像，只应安装可信来源的构建产物。

硬件原理图见 [`SCH_2.31寸彩屏掌机320_240_2026-07-02.pdf`](./SCH_2.31寸彩屏掌机320_240_2026-07-02.pdf)，界面约定见 [`DESIGN.md`](./DESIGN.md)。

## 主要特性

- 320x240 索引帧缓冲 Launcher，支持 English / 中文和 9 套可持久化字体。
- 从 `/MiaOS/<category>/<name>.app/<name>.bin` 动态发现最多 99 个 SD 应用，并按分类生成标签页。
- 双 OTA 分区：`ota_0` 固定运行 Launcher，`ota_1` 运行当前 SD 应用或 USB Disk 固件。
- MIA2 固件 Manifest：包含分类、名称、构建时间、镜像大小及双重 CRC32；相同固件可跳过重复擦写。
- Launcher 启动时可将 `ota_1` 中较新的 MIA2 应用安全同步回 SD 卡，也可手动执行 `Export OTA to SD`。
- USB Serial/JTAG CDC 文件服务：上传、下载、列表、创建、删除、重命名和直接启动 SD 应用。
- USB Mass Storage 固件：把 SD 卡作为 USB 磁盘暴露给电脑。
- USB NCM 网卡应用：扫描并连接 WiFi，将网络桥接为电脑的 USB 网卡。
- WiFi HTTP 文件管理和 FTP 文件服务。
- 启动阶段恢复菜单：清除 NVS，或手动选择 `ota_0` / `ota_1`。
- OTA 应用共用 Launcher 提供的显示、输入、SD、网络、音频和系统 Host ABI；大型字库与 SDL 兼容层可从 SD 动态加载。

## Flash 架构

[`partitions_dual.csv`](./partitions_dual.csv) 的关键分区如下：

| 分区 | 偏移 | 大小 | 用途 |
| --- | ---: | ---: | --- |
| `otadata` | `0x10000` | 8KB | OTA 启动选择 |
| `ota_0` | `0x20000` | 7MB | Launcher |
| `ota_1` | `0x720000` | 7MB | 当前 SD OTA 应用 / USB Disk |
| `coredump` | `0xE20000` | 64KB | Flash core dump（目前由 `lava_pal` 启用） |
| `startup` | `0xE30000` | 256KB | 启动恢复菜单 |

Launcher 和 USB Disk 会直接写入带 CRC 的 `otadata` 条目来切换分区，因为本硬件上调用常规镜像验证路径曾触发 TG1 interrupt WDT。

## 操作

Launcher 中：

- `UP` / `DOWN`：移动当前标签页中的选择。
- `LEFT` / `RIGHT`：切换 `System` 和 SD 分类标签页。
- `A`：打开内置项或启动 SD 应用。
- `SELECT`：打开 SD 应用操作菜单，可选择运行或把当前 `ota_1` 内容上传到该 SD 文件。
- `SELECT + START`：退出当前内置应用；OTA 应用和 USB Disk 也统一使用该组合返回 Launcher。

`System` 标签页固定包含：

- `VCP File Transfer`
- `Logs`
- `About`
- `Language`
- `Font`
- `Boot Loader`
- `Export OTA to SD`

SD 卡中的 `System` 类应用（例如 `usb disk`、`usb_wifi`）会追加在这些固定项目之后。`Boot Loader` 只显示进入 ROM 下载模式的说明：按住 `ST` 再按 `RESET`；单按 `RESET` 正常启动。

语言和字体保存在 NVS。若设置异常，可在 Launcher 启动时按住 `M` 跳过已保存设置。

### 启动恢复菜单

设备复位或上电时按住 `M`（GPIO8）约 1 秒，bootloader 会从 `startup` test 分区进入恢复菜单。松开 `M` 后可选择：

- `Clear NVRAM`：清除 NVS，选择 `ota_0` 并重启。
- `Boot ota_0`：启动 Launcher。
- `Boot ota_1`：启动当前应用分区。

菜单中使用 `UP` / `DOWN` 移动，`A` 或 `START` 确认，`B` 或 `SELECT` 取消并重启。此功能要求 `startup_menu.bin` 已烧录到 `0xE30000`；仅构建 Launcher 不会自动写入该分区。

## SD OTA 应用

Launcher 当前只扫描 `/MiaOS` 下的一级分类目录。分类名是动态的，但标准发布包使用：`Application`、`Emulators`、`Games`、`Media`、`Settings`、`System` 和 `Utils`。

每个应用必须使用以下结构，目录名、固件名和 Manifest 中的 `name` 应一致：

```text
/MiaOS/<category>/<name>.app/<name>.bin
```

例如：

```text
/MiaOS/Utils/calculator.app/calculator.bin
/MiaOS/Emulators/gba.app/gba.bin
/MiaOS/System/usb disk.app/usb disk.bin
```

旧的 `<name>.app/firmware.bin` 不会被识别；SD 根目录下的 `/Games`、`/Utils` 等分类也不再扫描。

### 当前应用

[`tools/build_sd.py`](./tools/build_sd.py) 的默认发布清单包括：

| 分类 | 应用 |
| --- | --- |
| `Application` | `hello` |
| `Emulators` | `coleco`, `gb`, `gba`, `gbc`, `gg`, `gw`, `lynx`, `megadrive`, `msx`, `nes`, `pce`, `sms`, `snes` |
| `Games` | `lava_pal`, `minesweeper` |
| `Media` | `music`（MP3 / WAV / FLAC / OGG） |
| `Settings` | `rtc_set` |
| `System` | `usb disk`, `usb_wifi` |
| `Utils` | `calculator`, `flashlight`, `wifi_scan`, `diagnostic`, `ftp_server`, `screen_test`, `sd_browser`, `timer`, `wifi_files` |

源码树还包含尚未加入默认发布清单的应用或测试项目，例如 `gmu`（当前支持 MP3 / MP2）、`lava_cch`、`mia_test` 和 `psram_test`。它们可以单独构建并按相同目录规则安装。

模拟器可通过应用内文件选择器打开 ROM。VCP 的 `launch` 命令还可传入文件路径，直接绕过选择器；该能力也适用于 `music`。

### MIA2 Manifest

当前固件尾部附加 72 字节 `MIA2` Manifest，而不是旧版 56 字节 `MIA1`。MIA2 增加了 `build_epoch`、`image_size` 和 `image_crc`：

- SD 固件与 `ota_1` Manifest 相同时，Launcher 跳过 Flash 擦写并直接启动。
- `ota_1` 版本较新或 SD 文件没有有效 MIA2 时，Launcher 在启动阶段把它同步到 `/MiaOS/<category>/<name>.app/<name>.bin`。
- 导出前会校验镜像 CRC，并通过临时文件和备份文件避免留下不完整固件。
- MIA1 仍可读取和启动，但不能可靠参与版本同步和镜像完整性校验。

格式、兼容性和安全边界见 [`docs/ota-app-manifest.md`](./docs/ota-app-manifest.md)。

构建单个 OTA 应用并附加 Manifest：

```sh
idf.py build -C experiments/ota_apps/calculator
python tools/append_manifest.py \
    --input experiments/ota_apps/calculator/build/calculator.bin \
    --category Utils --name calculator
```

然后安装为 `/MiaOS/Utils/calculator.app/calculator.bin`。`append_manifest.py` 会先移除已有的有效 MIA1/MIA2 尾部，避免重复追加。

### 共享库

发布包在 `/MiaOS/Library` 中包含：

- `libmia_text_v1.so`：Droid GBK 字库、Unicode/GBK 映射和文本渲染。
- `libmia_sdl_v1.so`：OTA 游戏使用的 SDL 兼容层。

构建命令：

```sh
idf.py -C experiments/shared_libraries/mia_text so
idf.py -C experiments/shared_libraries/mia_sdl so
```

这些 `.so` 是 OTA 应用运行时加载的共享组件，不表示 Launcher 又恢复为 ELF 应用加载架构。

## 文件服务

### VCP File Transfer

Launcher 空闲时，主机可通过 ESP32-S3 USB Serial/JTAG CDC 虚拟串口进入文件服务，无需先在设备上手动打开菜单：

```sh
uv run tools/serial_sd_client.py enter --port /dev/ttyACM0
uv run tools/serial_sd_client.py ping --port /dev/ttyACM0
uv run tools/serial_sd_client.py info --port /dev/ttyACM0
uv run tools/serial_sd_client.py list-dir --remote-path /MiaOS --port /dev/ttyACM0
```

文件操作：

```sh
uv run tools/serial_sd_client.py mkdir /MiaOS/Test --port /dev/ttyACM0
uv run tools/serial_sd_client.py put local.bin /MiaOS/Test/remote.bin --port /dev/ttyACM0
uv run tools/serial_sd_client.py get /MiaOS/Test/remote.bin ./download.bin --port /dev/ttyACM0
uv run tools/serial_sd_client.py rename /MiaOS/Test/remote.bin /MiaOS/Test/new.bin --port /dev/ttyACM0
uv run tools/serial_sd_client.py delete /MiaOS/Test/new.bin --port /dev/ttyACM0
```

直接启动应用并传入文件：

```sh
uv run tools/serial_sd_client.py launch /MiaOS/Emulators/gba.app/gba.bin \
    --file-path /roms/gba/game.gba --port /dev/ttyACM0
```

结束服务并回到 Launcher：

```sh
uv run tools/serial_sd_client.py exit --port /dev/ttyACM0
```

底层协议支持 `PING`、`INFO`、`LIST`、`MKDIR`、`DELETE`、`RENAME`、`PUT`、`GET`、`RUN` 和 `HELP`。上传采用 6144 字节流控窗口；请使用仓库自带客户端，不要在收到 `ACK` 前持续写入，否则 USB CDC RX 队列可能溢出。一个串口同一时间只能运行一个客户端。

VCP 路径是原始字节路径。当前 Launcher FatFs 配置使用 UTF-8，仍建议应用目录和固件名保持 ASCII，以兼容 Manifest 固定字段、主机 shell 和不同应用的路径处理。

### WiFi Files

`wifi_files` 在 SD 卡根目录读取可选的 `/wifi.txt`：

```ini
ssid=YourRouterWifiName
password=YourRouterWifiPassword
ap_ssid=MiaOS
ap_password=
```

配置了 `ssid` 时会先尝试连接路由器；失败或未配置时创建 `MiaOS` 热点。TFT 会显示 HTTP 地址。网页支持目录浏览、下载、上传、创建目录以及删除文件或空目录。

HTTP 服务不做身份认证，只应在可信局域网或设备直连热点中临时使用。

### FTP Server

`ftp_server` 创建开放热点并共享 SD 卡：

- WiFi SSID：`MiaOS`
- FTP 地址：`ftp://192.168.4.1:21`
- 用户名：`miaos`
- 密码：`miaos`
- 连接模式：被动模式（PASV）

### USB Disk 与 USB WiFi

`usb disk` 使用 TinyUSB CDC + MSC 复合设备把 SD 卡暴露为 USB Mass Storage。进入后 SD 卡由电脑直接访问，不应同时由其他固件读写；按 `SELECT + START` 返回 Launcher。

`usb_wifi` 使用 USB NCM 把 ESP32-S3 作为电脑的 USB 网卡。设备端可扫描 SSID、用屏幕键盘输入密码并保存连接配置。主机需要支持 USB NCM。

## 构建与发布

需要 PlatformIO；OTA 应用、共享库和恢复菜单还需要可用的 ESP-IDF 环境。

构建、烧录和监控 Launcher：

```sh
pio run -e esp32s3
pio run -e esp32s3 -t upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0 -b 115200
```

构建 USB Disk、OTA 测试固件和恢复菜单：

```sh
pio run -e esp32s3-usbmsc
pio run -e esp32s3-otatest
idf.py build -C experiments/startup_menu
```

将 USB Disk 或恢复菜单单独写入固定分区：

```sh
esptool.py --chip esp32s3 --port /dev/ttyACM0 -b 921600 \
    write_flash 0x720000 .pio/build/esp32s3-usbmsc/firmware.bin
esptool.py --chip esp32s3 --port /dev/ttyACM0 -b 921600 \
    write_flash 0xE30000 experiments/startup_menu/build/startup_menu.bin
```

### SD 发布包

先构建所需 OTA 应用、USB Disk 和两套共享库，再把已有产物打包：

```sh
python tools/build_sd.py
```

默认输出为 `dist/esp32-miaos-pixel_<git-revision>[-dirty]_sd.zip`。脚本会为每个应用重新生成 MIA2 Manifest，并跳过尚未构建的应用；缺少任一共享库时会停止并报告对应构建命令。

### 单文件 Flash 镜像

完整发布镜像必须显式按 [`partitions_dual.csv`](./partitions_dual.csv) 的偏移合并，不能使用把主应用放在 `0x10000` 的默认 flash 参数：

```sh
python ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 merge_bin \
    -o esp32-miaos-pixel_<revision>_esp32-s3-devkit.img \
    --flash_mode dio --flash_freq 80m --flash_size 16MB \
    0x0 .pio/build/esp32s3/bootloader.bin \
    0x8000 .pio/build/esp32s3/partitions.bin \
    0x10000 .pio/build/esp32s3/ota_data_initial.bin \
    0x20000 .pio/build/esp32s3/firmware.bin \
    0x720000 .pio/build/esp32s3-usbmsc/firmware.bin \
    0xE30000 experiments/startup_menu/build/startup_menu.bin
```

烧录：

```sh
python ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 \
    --port /dev/ttyACM0 -b 921600 write_flash 0x0 \
    esp32-miaos-pixel_<revision>_esp32-s3-devkit.img
```

不要在不了解设备当前分区表时单独把全 `0xFF` 的 `ota_data_initial.bin` 写到 `0x10000`；旧分区布局可能在该地址放置应用镜像。迁移旧设备时应一次写入匹配的 bootloader、分区表、otadata 和两个 OTA 槽。

## 日志与调试

Launcher 启动时覆盖 `/MiaOS/logs/latest.log`，记录启动摘要、SD 扫描、OTA 同步/刷写、USB Disk 和 VCP 操作。第三方库直接写到 `Serial` 的内容不保证进入该文件。

`lava_pal` 启用了写入 `coredump` 分区的 Flash core dump。崩溃后可在再次复现前读取：

```sh
python ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 \
    --port /dev/ttyACM0 -b 921600 read_flash 0xE20000 0x10000 /tmp/coredump.bin
source /opt/esp-idf/export.sh
espcoredump.py info_corefile -c /tmp/coredump.bin -t esp32s3 \
    -e experiments/ota_apps/lavapal/build/lava_pal.elf
```

必须使用与崩溃固件完全匹配的 ELF。其他 OTA 应用默认没有启用 core dump。

## Hardware

- **SoC Module**：ESP32-S3-WROOM-1-N16R8，16MB Flash + 8MB OPI PSRAM
- **LCD**：HD231005C10，2.31 英寸 IPS，320x240，ILI9342，SPI
- **SD Card**：独立 SPI 总线
- **RTC / Expansion**：PCF8563 与扩展 I2C 总线
- **Audio**：NS4168 I2S 功放 + 蜂鸣器
- **Input**：直连 GPIO 肩键/系统键 + 74HC165 扫描的方向键和 ABXY
- **USB**：原生 USB OTG（GPIO19 / GPIO20）
- **Power**：电池电压 ADC 采样 + USB 供电

## IO Mapping

| Function | Net | GPIO |
| --- | --- | ---: |
| LCD RST | LCD_RST | 3 |
| LCD DC | LCD_DC | 9 |
| LCD CS | LCD_CS | 10 |
| LCD MOSI | LCD_MOSI | 11 |
| LCD CLK | LCD_CLK | 12 |
| LCD Backlight | LCD_BCKL | 13 |
| SD CS | SDSPI_CS | 5 |
| SD MOSI | SDSPI_MOSI | 6 |
| SD CLK | SDSPI_CLK | 7 |
| SD MISO | SDSPI_MISO | 15 |
| I2C SCL | XSCL | 4 |
| I2C SDA | XSDA | 16 |
| Audio I2S WS | SND_I2S_WS | 42 |
| Audio I2S BCK | SND_I2S_BCK | 41 |
| Audio I2S DATA | SND_I2S_DATA | 40 |
| USB D- | DN | 19 |
| USB D+ | DP | 20 |
| Battery Voltage | VBAT_VOLTAGE | 1 |
| Audio Amp Enable | CTRL | 46 |
| Beep | BEEP | 14 |
| Key BOOT / START | KEY_BOOT | 0 |
| Key L | KEY_L | 17 |
| Key R | KEY_R | 18 |
| Key M | KEY_M | 8 |
| Key SELECT | KEY_SELECT | 21 |
| HC165 PL | K_PL | 2 |
| HC165 CLK | K_CLK | 39 |
| HC165 DAT | K_DAT | 38 |

74HC165 位分配为 `LEFT=0`、`DOWN=1`、`UP=2`、`RIGHT=3`、`Y=4`、`X=5`、`A=6`、`B=7`。LCD 的 MISO 使用未连接的 GPIO48 作为占位，避免 Arduino SPI 默认占用背光 GPIO13。

NC / 可扩展 GPIO：35、36、37、43（TXD0）、44（RXD0）、45、47、48。

## License

项目许可证见 [`LICENSE`](./LICENSE)。模拟器、游戏移植和第三方库各自保留上游许可证；制作分发包时应同时包含相应许可证文件。
