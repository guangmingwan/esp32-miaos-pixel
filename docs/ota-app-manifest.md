# OTA App Manifest 用途与设计

## 1. 解决的问题

MiaOS 的 SD 应用本质上是可写入 `ota_1` 分区的 ESP-IDF 固件镜像。仅靠文件名，launcher 无法可靠回答以下问题：

- 这个固件属于哪个分类、应用名是什么？
- SD 卡中的固件和 `ota_1` 中已经写入的固件是否完全相同？
- `ota_1` 中的应用应导出到 SD 卡的哪个目录？
- OTA 分区中的版本是否比 SD 卡中的版本新？
- 从 OTA 分区导出时，读出的固件内容是否损坏？

Manifest 为固件增加一个固定格式的身份和完整性尾部，用于解决这些问题。它不参与应用运行，也不改变 ESP-IDF 固件镜像本身。

## 2. 为什么使用尾随数据

Manifest 追加在完整 ESP-IDF `.bin` 文件之后：

```text
+-----------------------------+------------------+
| ESP-IDF application image   | MIA2 manifest    |
| image_size bytes            | 72 bytes         |
+-----------------------------+------------------+
```

ESP-IDF bootloader 根据镜像头和 segment 信息只读取有效镜像长度，不会执行尾部数据。因此同一个文件可以同时用于：

- 写入 `ota_1` 并正常启动；
- 保存在 SD 卡中供 launcher 扫描和刷写；
- 从 `ota_1` 导出后恢复原有应用身份。

这种设计不需要修改 ESP-IDF bootloader，也不需要在固件内部预留自定义 section。Manifest 生成工具可以在编译完成后独立运行。

## 3. MIA2 格式

当前格式定义在 `include/ota_app_manifest.h`，结构采用 little-endian、packed 布局，总长度为 72 字节。

| 偏移 | 长度 | 字段 | 说明 |
|---:|---:|---|---|
| 0 | 4 | `magic` | `0x3241494D`，内存中的 ASCII 为 `MIA2` |
| 4 | 16 | `category` | SD 分类名，如 `Utils`、`Emulators` |
| 20 | 32 | `name` | 应用名，如 `calculator`、`gba` |
| 52 | 8 | `build_epoch` | Unix 构建时间，用于新旧版本比较 |
| 60 | 4 | `image_size` | Manifest 之前的固件字节数 |
| 64 | 4 | `image_crc` | 固件内容的 CRC32，不包含 Manifest |
| 68 | 4 | `crc` | 前 68 字节 Manifest 字段的 CRC32 |

`category` 和 `name` 使用 UTF-8 字节写入固定数组，不足部分以 `\0` 填充。当前发布命名使用 ASCII，避免固定长度截断和路径兼容问题。

## 4. 两层 CRC 的职责

MIA2 有两个不同的 CRC32：

1. `image_crc` 校验固件正文，检测 SD、Flash 或传输过程造成的内容损坏。
2. `crc` 校验 Manifest 自身，防止分类、名称、时间、大小或 `image_crc` 被意外破坏。

`crc` 也可以作为低成本的固件身份值。Launcher 比较 SD 文件和 `ota_1` 时，会比较：

- `magic`
- `category`
- `name`
- `crc`

只要构建时间、固件正文或身份字段发生变化，MIA2 的 `crc` 通常都会变化，launcher 就不会把两个版本误判为同一固件。

## 5. Launcher 中的使用流程

### 5.1 避免重复刷写

用户启动 SD 应用时，launcher 分别读取 SD 文件尾部和 `ota_1` 中的 Manifest。如果二者身份完全匹配，说明目标固件已经位于 `ota_1`，launcher 会跳过擦除和写入，直接切换 OTA 启动分区。

这样可以减少启动等待时间和 Flash 擦写次数。

### 5.2 自动导出到 SD

Launcher 从 `ota_1` 读取 `category` 和 `name`，自动生成：

```text
/MiaOS/<category>/<name>.app/<name>.bin
```

导出 MIA2 固件时，launcher 按 `image_size` 读取正文，计算 CRC32 并与 `image_crc` 比较。校验通过后才写入原 Manifest；校验失败时删除不完整的输出文件。

### 5.3 启动时同步较新版本

MIA2 的 `build_epoch` 用于判断版本新旧：

- SD 版本时间大于或等于 OTA 版本：不更新。
- OTA 版本较新或 SD 文件没有有效 MIA2：将 OTA 版本同步到 SD。

同步过程使用临时文件和备份文件，写入后重新读取 Manifest，确认 `build_epoch` 和 `image_crc` 与来源一致，再替换正式文件。

## 6. OTA 分区中的定位方式

SD 文件可以直接从文件末尾读取 72 字节。Flash 分区不同：`esp_image_get_metadata()` 返回的 `image_len` 可能位于 esptool segment 对齐、可选 SHA-256 数据之前。

因此 launcher 从 `image_len` 开始向后扫描最多 512 字节，查找 `MIA2` 或兼容的 `MIA1` magic，并且只有 CRC 校验通过才接受该结构。

读取未验证或损坏的 OTA 分区时，`esp_image_get_metadata()` 在本硬件上可能触发 TG1 interrupt WDT，所以相关调用必须放在 `ScopedIntWdtPause` 保护范围内。

## 7. MIA1 兼容

旧版 MIA1 长度为 56 字节，只包含：

```text
magic + category[16] + name[32] + crc
```

Launcher 仍可读取 MIA1，并将其规范化为内部 `OtaAppManifest` 表示。但 MIA1 没有构建时间、镜像大小和镜像 CRC，不能参与可靠的新旧版本同步，也不能在导出时校验完整固件正文。

新构建和发布包应统一生成 MIA2。`tools/append_manifest.py` 检测到已有的有效 MIA1 或 MIA2 时，会先移除旧尾部再追加新的 MIA2，避免重复堆叠 Manifest。

## 8. 构建方法

```sh
idf.py build -C experiments/ota_apps/calculator

python tools/append_manifest.py \
    --input experiments/ota_apps/calculator/build/calculator.bin \
    --category Utils \
    --name calculator
```

可使用 `--output` 保留原始构建产物，使用 `--build-epoch` 或环境变量 `SOURCE_DATE_EPOCH` 生成可复现的统一发布时间：

```sh
python tools/append_manifest.py \
    --input build/app.bin \
    --output package/MiaOS/Utils/app.app/app.bin \
    --category Utils \
    --name app \
    --build-epoch 1784099068
```

## 9. 命名和路径约束

Manifest、目录和固件文件必须保持一致：

```text
category = Utils
name     = calculator
path     = /MiaOS/Utils/calculator.app/calculator.bin
```

`category` 和 `name` 不允许包含 `/`、`\` 或 `..`。自动导出和同步前会再次检查这些字段，避免 Manifest 构造任意 SD 路径。

## 10. 安全边界

CRC32 用于检测非恶意损坏，不是密码学签名。攻击者可以修改固件后重新计算 `image_crc` 和 `crc`，因此 Manifest 不能证明固件来源可信，也不能替代 Secure Boot、Flash Encryption 或签名 OTA。

当前 OTA app 也不是沙箱：它运行在完整 ESP32 权限下，并与 launcher 共享硬件和分区。只应安装来自可信构建环境的应用。若未来需要分发不受信任的第三方应用，应在 MIA2 之外增加签名格式、可信公钥和签名验证流程，而不是继续扩展 CRC32 的职责。
