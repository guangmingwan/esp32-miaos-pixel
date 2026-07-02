# Dual-Firmware USB Disk 实现方案

## 架构概述

ESP32-S3 运行两个固件，分别烧录在 `ota_0`（Launcher）和 `ota_1`（USB Disk），通过 OTA 分区切换协议实现热切换：

```
Flash Layout:
  +------------------+  0x0000
  | bootloader       |
  +------------------+  0x10000
  | otadata          |  (32 字节 x2 的 OTA 选择槽)
  +------------------+  0x20000
  | ota_0 (Launcher) |  ~7MB
  |   - 菜单 UI      |
  |   - App 启动器   |
  |   - SD ELF 加载  |
  +------------------+  0x720000
  | ota_1 (USB Disk) |  ~7MB
  |   - LCD 显示     |
  |   - USB MSC      |
  +------------------+  0xF00000 (end)
```

## 双分区表中的关键决策

### 1. 分区表配置

`partitions_dual.csv` 指定两个 OTA app 分区，各约 7MB。配套 `sdkconfig.esp32s3` 必须设置：

```ini
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions_dual.csv"
```

> **为什么不用 `board_build.partitions`？**  
> Launcher 使用 `framework = arduino, espidf` 混合框架，ESP-IDF 部分的 sdkconfig 会覆盖 PlatformIO 的 `board_build.partitions` 配置。因此必须直接修改 sdkconfig 文件。

### 2. OTA 切换原理

ESP32 OTA 机制通过 `otadata` 分区（位于 0x10000，2 个 4096 字节的槽位）的两个 `esp_ota_select_entry_t` 结构体（各 32 字节）决定启动哪个 app 分区。格式：

```c
typedef struct __attribute__((packed)) {
    uint32_t ota_seq;        // 序列号，奇数=ota_0，偶数=ota_1
    uint8_t  seq_label[20];  // 填充
    uint32_t ota_state;      // 2 = valid
    uint32_t crc;            // esp_rom_crc32_le(UINT32_MAX, &ota_seq, 4)
} OtaEntry;
```

bootloader 启动时遍历所有 otadata 条目，选择 **`ota_seq` 最高**且 CRC 校验通过的条目对应的分区。序列号与分区的关系：

```
ota_slot = (seq - 1) % ota_app_count
```

- `seq=1` → ota_0, `seq=3` → ota_0（启动 Launcher）
- `seq=2` → ota_1, `seq=4` → ota_1（启动 USB Disk）

### 3. `forceOtaBoot()` — 手动写入 otadata

`esp_ota_set_boot_partition()` 在某些情况下可能触发 WDT 导致重启，因此采用**直接写入 otadata 分区**的方式：

```cpp
void forceOtaBoot(const esp_partition_t *target) {
    // 1. 计算出所需序列号（奇数=ota_0, 偶数=ota_1）
    uint32_t seq = (ota_slot == 0) ? 1 : 2;

    // 2. 构造两个 otadata 条目：一个标记目标，一个做备选
    OtaEntry entries[2];
    setEntry(&entries[0], seq, 2);       // 目标分区
    setEntry(&entries[1], seq + 2, 2);   // 更高序列号的同分区

    // 3. 写入 otadata 分区
    esp_partition_erase_range(otap, 0, otap->size);
    esp_partition_write(otap, 0,    &entries[0], sizeof(OtaEntry));
    esp_partition_write(otap, 4096, &entries[1], sizeof(OtaEntry));

    // 4. 重启
    ESP.restart();
}
```

写入两条的目的是保证即使 bootloader 选最高 seq 也能正确启动到目标分区。

### 4. 退出 USB Disk（返回 Launcher）

USB Disk 固件使用 **SELECT + START** 组合键退出（无需 HC165 移位寄存器）：

```cpp
static bool exitTriggered() {
    return digitalRead(KEY_SELECT_PIN) == 0 && digitalRead(KEY_BOOT_PIN) == 0;
}
```

触发后调用 `switchToOta0()`，写入 `seq=1, seq=3` 到 otadata 并重启。

## USB MSC 实现

### Framework 选择

两个固件用不同的框架组合：

| 固件 | framework | USB 栈 | Serial |
|------|-----------|--------|--------|
| Launcher | `arduino, espidf` | 不涉及 USB MSC | 硬件 USB Serial/JTAG |
| USB Disk | `arduino`（纯 Arduino） | TinyUSB | TinyUSB CDC |

> **为什么 USB Disk 用纯 Arduino？**  
> 混合框架（`arduino, espidf`）下 TinyUSB 栈可能与 ESP-IDF 的 USB 驱动冲突。纯 Arduino 框架保持 TinyUSB 栈干净。

### ARDUINO_USB_MODE 的重要性

`platformio.ini` 中 USB Disk 环境的 build flags：

```ini
[env:esp32s3-usbmsc]
build_flags =
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
    -Iinclude
```

| 模式 | 作用 | 问题 |
|------|------|------|
| `ARDUINO_USB_MODE=0`（默认） | Serial 走硬件 USB Serial/JTAG | 硬件 USB Serial/JTAG 和 TinyUSB(MSC) 抢同一对 D+/D- 引脚，USB 协议冲突（EPROTO） |
| `ARDUINO_USB_MODE=1` | Serial 走 TinyUSB CDC | TinyUSB 统一管理 CDC+MSC，无冲突 |

设置为 `1` 后，TinyUSB 栈初始化一个复合 USB 设备，包含两个接口：
- Interface 0: **CDC ACM**（串口日志）
- Interface 1: **MSC**（Mass Storage Class）

### MSC 回调实现

使用 FatFs 的 `disk_read()` / `disk_write()` 直接操作 SD 卡扇区（物理驱动器 0）：

```cpp
#include <ff.h>        // FatFs 类型定义（BYTE, DRESULT...）
#include <diskio.h>    // disk_read / disk_write 原型

static int32_t onMscRead(uint32_t lba, uint32_t offset, void *buf, uint32_t len) {
    uint32_t sector = lba + offset / 512;
    uint32_t boff = offset % 512;
    uint32_t count = (boff + len + 511) / 512;

    if (boff == 0 && (len % 512) == 0) {
        // 对齐读 — 直接读取
        if (disk_read(0, (BYTE*)buf, sector, count) != RES_OK) return -1;
    } else {
        // 非对齐读 — 读整扇区再复制偏移部分
        ...
    }
    return len;  // ← 关键：返回实际传输字节数，不是 0
}

static int32_t onMscWrite(uint32_t lba, uint32_t offset, uint8_t *buf, uint32_t len) {
    ...
    if (boff == 0 && (len % 512) == 0) {
        // 对齐写
        if (disk_write(0, (BYTE*)buf, sector, count) != RES_OK) return -1;
    } else {
        // 非对齐写 — 读-改-写
        ...
    }
    return len;
}
```

> **关键陷阱**：MSC 回调必须返回 **实际传输的字节数**（`len`），而不是 0。  
> 返回 0 会让 TinyUSB 认为"0 字节传输完成"，USB 主机等待 30 秒后超时复位设备。

### 烧录流程

构建 USB Disk 固件后，直接烧录到 0x720000（ota_1）：

```sh
pio run -e esp32s3-usbmsc
python3 esptool.py --chip esp32s3 --port /dev/ttyACM0 -b 921600 \
    write_flash 0x720000 .pio/build/esp32s3-usbmsc/firmware.bin
```

## 完整使用流程

```
1. 上电 → Launcher（ota_0）启动
2. 菜单选择 "USB Disk"
3. Launcher 调用 forceOtaBoot() 写入 otadata
4. 重启 → ota_1 启动 USB Disk 固件
5. LCD 显示 "USB Disk Mode" → 初始化 SD → 启动 USB MSC
6. 用 USB 线连电脑 → /dev/sdc 出现，可读写 SD 卡
7. 按 SELECT + START → USB Disk 写入 otadata 指向 ota_0
8. 重启 → 返回 Launcher
```

## 已知限制

- **单 USB 口复用**：ESP32-S3 只有一个物理 USB（GPIO 19/20 D+/D-），串口日志和 MSC 通过 TinyUSB 复合设备共用同一连线。不能同时接两个 USB 口（一个看日志一个传数据）。
- **SD 卡独占**：USB MSC 模式下 SD 卡被 FatFs 独占，PC 直接访问原始扇区。SPI 模式的读写在 MSC 回调中完成，未加并发锁。
- **TinyUSB CDC vs 硬件 USB Serial/JTAG**：`ARDUINO_USB_MODE=1` 后 TinyUSB 接管 USB，硬件 USB Serial/JTAG 不再活动。烧录仍然可用（烧录时进入下载模式，`esptool` 通过 ROM 串口 loader 写入）。
