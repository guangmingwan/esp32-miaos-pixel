# ESP32 Retro-Pixel 设备外设布局（原理图分析）

> 数据来源：`datasheet.png` 原理图截图
> 核心模块：**ESP32-S3-WROOM-1-N16R8**（实测 ESP32-S3 QFN56 rev v0.2，16MB Flash + 8MB PSRAM）

---

## 一、电源与系统

- **片内内存**：384KB ROM + 512KB SRAM + 16KB RTC SRAM
- **外部/封装存储**：16MB SPI Flash（quad I/O，3.3V）+ 8MB embedded PSRAM（AP_3v3）
- **晶振**：40MHz

| 网络 | 引脚 | 说明 |
|---|---|---|
| +3V3 / GND | 模块 1/2/40/41 脚 | 主电源 |
| CHIP_PU (EN) | 模块 3 脚 | 复位/使能 |
| VBAT_VOLTAGE | GPIO1 | 电池电压 ADC 采样 |
| CTRL | GPIO46 | 总控制信号（R50 10kΩ 下拉，默认低电平） |

去耦：C4 100nF（+3V3 ↔ GND）。

---

## 二、功能模块分组

### 1. LCD 显示屏（SPI 总线 1）

- **模组型号**：HD231005C10
- **尺寸**：2.31 英寸
- **面板类型**：IPS
- **分辨率**：320 × 240
- **驱动 IC**：ILI9342
- **接口**：SPI
- **色彩**：16-bit RGB565（ILI9342 典型配置）
- **厂家**：深圳华迪创显技术有限公司
- **Compatible**：标准 TFT_LCD 帧格式，可用 ESP-IDF `esp_lcd` 组件驱动

| 信号 | 引脚 |
|---|---|
| LCD_RST | GPIO3 |
| LCD_DC | GPIO9 |
| LCD_CS | GPIO10 |
| LCD_MOSI | GPIO11 |
| LCD_CLK | GPIO12 |
| LCD_BCKL | GPIO13 |

### 2. SD 卡（SPI 总线 2，独立于 LCD）

| 信号 | 引脚 |
|---|---|
| SDSPI_CS | GPIO5 |
| SDSPI_MOSI | GPIO6 |
| SDSPI_CLK | GPIO7 |
| SDSPI_MISO | GPIO15 |

### 3. 音频输出（I2S）

| 信号 | 引脚 |
|---|---|
| SND_I2S_WS (LRCLK) | GPIO42 |
| SND_I2S_BCK (BCLK) | GPIO41 |
| SND_I2S_DATA | GPIO40 |

### 4. USB（原生 USB-OTG）

| 信号 | 引脚 |
|---|---|
| DN (D-) | GPIO19 |
| DP (D+) | GPIO20 |

### 5. 按键

| 信号 | 引脚 | 说明 |
|---|---|---|
| KEY_BOOT | GPIO0 | BOOT / 启动键 |
| KEY_L | GPIO17 | 左 |
| KEY_R | GPIO18 | 右 |
| KEY_M | GPIO8 | 中 |
| KEY_SELECT | GPIO21 | 选择 |
| KEY_START | — | 与 BOOT 同节点（复用） |

### 6. 外部键盘 / 扫描输入

| 信号 | 引脚 | 推测用途 |
|---|---|---|
| K_PL | GPIO2 | 矩阵列 / 选通 |
| K_CLK | GPIO39 | 时钟（编码器 / 键盘扫描） |
| K_DAT | GPIO38 | 数据 |

### 7. I2C 扩展总线（`X` 前缀，用于传感器 / 外设）

| 信号 | 引脚 |
|---|---|
| XSCL | GPIO4 |
| XSDA | GPIO16 |

### 8. 蜂鸣器

| 信号 | 引脚 |
|---|---|
| BEEP | GPIO14 |

---

## 三、引脚总览表

| GPIO | 网络名 | 功能 |
|---|---|---|
| 0 | KEY_BOOT | BOOT 按键 / KEY_START |
| 1 | VBAT_VOLTAGE | 电池电压采样 (ADC) |
| 2 | K_PL | 键盘扫描列 |
| 3 | LCD_RST | LCD 复位 |
| 4 | XSCL | I2C SCL |
| 5 | SDSPI_CS | SD 卡 CS |
| 6 | SDSPI_MOSI | SD 卡 MOSI |
| 7 | SDSPI_CLK | SD 卡 CLK |
| 8 | KEY_M | 中按键 |
| 9 | LCD_DC | LCD D/C |
| 10 | LCD_CS | LCD CS |
| 11 | LCD_MOSI | LCD MOSI |
| 12 | LCD_CLK | LCD CLK |
| 13 | LCD_BCKL | LCD 背光 |
| 14 | BEEP | 蜂鸣器 |
| 15 | SDSPI_MISO | SD 卡 MISO |
| 16 | XSDA | I2C SDA |
| 17 | KEY_L | 左按键 |
| 18 | KEY_R | 右按键 |
| 19 | DN | USB D- |
| 20 | DP | USB D+ |
| 21 | KEY_SELECT | 选择按键 |
| 38 | K_DAT | 键盘数据 |
| 39 | K_CLK | 键盘时钟 |
| 40 | SND_I2S_DATA | 音频 I2S DATA |
| 41 | SND_I2S_BCK | 音频 I2S BCLK |
| 42 | SND_I2S_WS | 音频 I2S WS / LRCLK |
| 46 | CTRL | 总控制（下拉） |

### NC / 未连接引脚

TXD0 (GPIO43)、RXD0 (GPIO44)、GPIO35、GPIO36、GPIO37、GPIO45、GPIO47、GPIO48 — 可用于扩展。

---

## 四、设备整体定位

从外设组合判断，这是一台 **掌机形态的嵌入式设备**（`miaos-pixel` / retro-pixel）：

- **显示**：SPI LCD 屏（**HD231005C10 / ILI9342, 2.31" IPS, 320×240**）
- **存储**：SPI SD 卡
- **声音**：I2S 功放 + 蜂鸣器
- **输入**：方向键（L/M/R）+ SELECT/START + 外部键盘扫描
- **扩展总线**：I2C（X 前缀）
- **供电**：电池（电压采样）+ USB（供电 / 数据 / 烧录）
- **预留 GPIO**：多个空闲引脚便于未来扩展

---

## 五、设计注意点

1. **LCD 与 SD 采用两条独立 SPI 总线**，避免高速 SD 卡操作干扰刷屏，但占用较多 GPIO。
2. **GPIO0 (BOOT) 与 KEY_START 共节点** —— 开机时按下 START 会进入下载模式，固件需注意上电瞬间按键状态。
3. **GPIO46 (CTRL) 通过 R50 10kΩ 下拉** —— 默认低电平，疑似功放使能 / 音频静音 / 某总开关，需通过代码置高生效。
4. **GPIO35-48 中有多个 NC**，便于扩展（例如再加 SPI、CAN、以太网等）。
