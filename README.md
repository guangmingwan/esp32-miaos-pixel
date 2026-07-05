# ESP32-S3 Retro-Pixel Launcher

PlatformIO project for the **ESP32-S3-WROOM-1-N16R8** board (16MB Flash + 8MB PSRAM).

The firmware drives the onboard **HD231005C10** LCD module (2.31" IPS, 320×240,
driver IC **ILI9342**, SPI) and starts a simple launcher. The first built-in app
is the hardware diagnostic screen with chip info, TFT/SD status, ADC values,
buttons, uptime, and frame count. It also prints the same status to the serial
monitor.

See [`retro-pixel-datasheet.md`](./retro-pixel-datasheet.md) for the full
schematic-derived peripheral map.

## Detected Device

Detected with `esptool v5.3.1` on Windows `COM10` (`TinyUSB CDC`, USB VID:PID
`303A:1001`):

- **Chip**: ESP32-S3 (QFN56), revision v0.2
- **CPU**: dual core + LP core, up to 240MHz
- **Wireless**: Wi-Fi + Bluetooth 5 LE
- **Crystal**: 40MHz
- **On-chip memory**: 384KB ROM, 512KB SRAM, 16KB RTC SRAM
- **Flash**: 16MB SPI flash, quad I/O, 3.3V, JEDEC `46:4018`
- **PSRAM**: 8MB embedded PSRAM, vendor `AP_3v3`, 85C rating
- **USB mode during detection**: USB-Serial/JTAG ROM loader
- **MAC**: `a4:cb:8f:c1:c0:00`
- **Security fuses**: Secure Boot disabled, Flash Encryption disabled, JTAG and
  USB download modes not permanently disabled

## Launcher Model

The launcher uses one firmware image with built-in apps linked at compile time.
It can also discover native ELF apps on the SD card and run them in the same
firmware address space through the ESP-IDF `elf_loader` component.

- `A` starts the selected app.
- `B` exits the active app and returns to the launcher.
- `UP` / `DOWN` move through the items in the current tab.
- `LEFT` / `RIGHT` switch between `System` and SD category tabs.

`About` remains the only informational built-in launcher app. The launcher now exposes a
`System` tab for `Serial Files`, `Logs`, `About`, `USB Disk`, and `Boot Loader`, while SD-loaded ELF
apps are grouped into tabs by their SD category directory.

## WiFi SD File Server

The `WiFi Files` app publishes the SD card over guest HTTP access. It provides a
browser page for listing directories, downloading files, uploading files,
creating folders, and deleting files or empty folders.

On start, the app reads optional WiFi settings from `/wifi.txt` on the SD card:

```ini
ssid=YourRouterWifiName
password=YourRouterWifiPassword
ap_ssid=MiaOS-SD
ap_password=
```

If `ssid` is set, the ESP32 first tries to join that router. A PC connected to
the same router by Ethernet can then open the URL shown on the TFT, for example
`http://192.168.1.23/`. If the file is missing or router connection fails, the
ESP32 starts an open fallback hotspot named `MiaOS-SD`; connect a phone or WiFi
client to it and open `http://192.168.4.1/`.

Access is intentionally unauthenticated. Use it only on a trusted LAN or while
connected directly to the fallback hotspot.

## FTP SD Server

The `FTP Server` app starts a dedicated open WiFi access point and publishes the
SD card through FTP. It is useful for standard FTP clients such as FileZilla or
WinSCP.

- WiFi SSID: `MiaOS-FTP`
- FTP host: `192.168.4.1`
- FTP port: `21`
- FTP user: `guest`
- FTP password: `guest`

Use passive mode (`PASV`) in the FTP client. The app is intended for direct,
temporary access while you are connected to the device hotspot.

## SD ELF Apps

`include/sd_app_loader.h` discovers native ELF apps on the already-mounted
Arduino SD card without remounting it. Apps are listed from category folders at
the SD root plus the legacy MiaOS path:

```text
/Games/*.app/app.elf
/Utils/*.app/app.elf
/Settings/*.app/app.elf
/Emulators/*.app/app.elf
/Media/*.app/app.elf
/Application/*.app/app.elf
/MiaOS/Games/*.app/app.elf
/MiaOS/Utils/*.app/app.elf
/MiaOS/Settings/*.app/app.elf
/MiaOS/Emulators/*.app/app.elf
/MiaOS/Media/*.app/app.elf
/MiaOS/Application/*.app/app.elf
```

Selecting an SD app calls the runner API in `include/mia_elf_runner.h`, which
loads the ELF bytes through Arduino `SD.open()`, registers the experimental host
ABI symbols from `include/mia_host_abi.h`, and runs the app when the ESP-IDF
`elf_loader` component is available. The current host ABI is version 2. For
button input, apps must call `mia_host_buttons_poll()` once per logical loop,
then read cached state through `mia_host_button_down()`,
`mia_host_button_pressed()`, or `mia_host_button_released()`. Do not poll more
than once per loop before all input decisions are made; doing so can advance the
pressed/released edges before later handlers consume them. ABI v1 apps must be
rebuilt against the matching v2 host header.

The ABI includes:

```c
uint32_t mia_host_abi_version(void);
void mia_host_log(const char *message);
void mia_host_buttons_poll(void);
uint8_t mia_host_button_down(uint8_t button);
uint8_t mia_host_button_pressed(uint8_t button);
uint8_t mia_host_button_released(uint8_t button);
```

Build the sample app and copy it to the SD card as `app.elf`:

```sh
pio run -d experiments/elf_apps/hello
```

Generated artifact:

```text
experiments/elf_apps/hello/.pio/build/esp32s3/app.elf
```

SD card target path:

```text
/MiaOS/Application/hello.app/app.elf
```

Expected SD card layout for the apps now shipped as SD ELF apps:

```text
SD card root/
├── Emulators/
├── Games/
│   └── minesweeper.app/app.elf
├── Media/
├── Settings/
│   ├── diagnostic.app/app.elf
│   ├── rtc_set.app/app.elf
│   └── wifi_scan.app/app.elf
├── Utils/
│   ├── calculator.app/app.elf
│   ├── flashlight.app/app.elf
│   ├── ftp_server.app/app.elf
│   ├── screen_test.app/app.elf
│   ├── sd_browser.app/app.elf
│   ├── timer.app/app.elf
│   └── wifi_files.app/app.elf
└── Application/
    └── hello.app/app.elf
```

On the launcher, use `LEFT` / `RIGHT` to switch tabs. `Boot Loader` now shows a
manual instruction dialog instead of forcing ROM download mode; hold `ST` and
press `RESET` to enter the boot loader, and press `RESET` alone to boot normally.

The `Logs` entry in the `System` tab reads `/MiaOS/logs/latest.log` from the SD
card. The launcher overwrites this file on startup and records the current boot
summary plus the most recent SD ELF launch result and error code. The file also
includes launcher-owned serial traces from startup, SD scanning, USB Disk, and
ELF loader execution; it does not automatically capture every third-party library
message written directly to `Serial`.

## Serial File Transfer

The `Serial Files` entry in the `System` tab starts a serial file service over
the ESP32-S3 USB CDC/JTAG port. It uses a simple command protocol rather than a
network FTP stack.

Supported commands:

```text
PING
LIST <path>
MKDIR <path>
DELETE <path>
PUT <path> <size>
```

Use the host helper script:

```sh
uv run tools/serial_sd_client.py ping
uv run tools/serial_sd_client.py list-dir /MiaOS/Application
uv run tools/serial_sd_client.py mkdir /MiaOS/Test
uv run tools/serial_sd_client.py put ./app.elf /MiaOS/Application/test.app/app.elf
```

Paths should not contain spaces. The launcher writes `Serial Files` activity to
`/MiaOS/logs/latest.log`.

Build the migrated SD apps with:

```sh
pio run -d experiments/elf_apps/diagnostic
pio run -d experiments/elf_apps/screen_test
pio run -d experiments/elf_apps/flashlight
pio run -d experiments/elf_apps/timer
pio run -d experiments/elf_apps/wifi_scan
pio run -d experiments/elf_apps/wifi_files
pio run -d experiments/elf_apps/ftp_server
```

Copy each generated `.pio/build/esp32s3/app.elf` to the matching SD directory:

```text
/Settings/diagnostic.app/app.elf
/Utils/screen_test.app/app.elf
/Utils/flashlight.app/app.elf
/Utils/timer.app/app.elf
/Settings/wifi_scan.app/app.elf
/Utils/wifi_files.app/app.elf
/Utils/ftp_server.app/app.elf
```

ELF apps are not sandboxed. A bad or incompatible ELF can crash or corrupt the
launcher. Keep the ABI versioned and rebuild apps against the matching host ABI.

## Hardware

- **SoC Module**：ESP32-S3-WROOM-1-N16R8（实测 ESP32-S3 QFN56 rev v0.2，16MB Flash + 8MB PSRAM）
- **On-chip Memory**：384KB ROM + 512KB SRAM + 16KB RTC SRAM
- **LCD**：HD231005C10，2.31" IPS，320×240，ILI9342，SPI
- **SD Card**：独立 SPI 总线
- **Audio**：I2S 功放输出 + 蜂鸣器
- **Input**：KEY_L / KEY_R / KEY_M / KEY_SELECT / KEY_START / BOOT
- **External Keyboard Scan**：K_PL / K_CLK / K_DAT
- **Expansion Bus**：I2C（X 前缀，用于传感器 / 外设）
- **USB**：原生 USB-OTG（D-/D+）
- **Power**：电池供电（VBAT_VOLTAGE 电压采样）+ USB

## IO Mapping

| Function | Net | GPIO |
| --- | --- | --- |
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
| CTRL (amp / enable) | CTRL | 46 |
| Beep | BEEP | 14 |
| Key BOOT | KEY_BOOT | 0 |
| Key L | KEY_L | 17 |
| Key R | KEY_R | 18 |
| Key M | KEY_M | 8 |
| Key SELECT | KEY_SELECT | 21 |
| Key Scan PL | K_PL | 2 |
| Key Scan CLK | K_CLK | 39 |
| Key Scan DAT | K_DAT | 38 |

NC / 可扩展：GPIO35、GPIO36、GPIO37、GPIO43（TXD0）、GPIO44（RXD0）、GPIO45、GPIO47、GPIO48。

## Commands

构建 / 烧录 / 监控（PlatformIO，环境名 `esp32s3`）：

```sh
pio run                          # 构建
pio run -t upload                # 烧录
pio device monitor               # 串口监控
```

ESP32-S3 在 Linux 上通常以 USB 串口或原生 USB CDC 形式出现，端口路径形如：

```sh
ls /dev/ttyUSB* /dev/ttyACM*
ls /dev/serial/by-id
```

然后通过 `--upload-port` / `--port` 覆盖默认端口，无需改 `platformio.ini`：

```sh
pio run -t upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0 -b 115200
```

> 当前 `platformio.ini` 与实测硬件一致：ESP32-S3-WROOM-1-N16R8，16MB Flash，
> 8MB PSRAM。`src/` 下显示驱动、帧缓冲尺寸、SPI 引脚常量和按键映射已按
> HD231005C10 / ILI9342（320×240）迁移。
