# Lava PAL resource files

此目录是 `lava_pal` 的游戏数据打包输入目录。`tools/build_sd.py` 会将目录中的所有文件复制到 SD 包的：

```text
/MiaOS/Games/lava_pal.app/
```

## 仓库内置文件

以下 GB2312 数据随源码入库：

- `M_GB2312.MSG`
- `WORD_GB2312.DAT`
- `DESC_GB2312.DAT`
- `FONT_GB2312.DAT`
- `FONT_GB2312_SMALL.DAT`

## 需要自行获取

以下文件来自《仙剑奇侠传》原版游戏，受版权保护，不随本仓库分发。请从自己合法拥有的 DOS/Windows 游戏安装介质中提取，并保持文件名不变：

- `ABC.MKF`
- `BALL.MKF`
- `DATA.MKF`
- `F.MKF`
- `FBP.MKF`
- `FIRE.MKF`
- `GOP.MKF`
- `MAP.MKF`
- `MGO.MKF`
- `PAT.MKF`
- `RGM.MKF`
- `RNG.MKF`
- `SSS.MKF`
- `MUS.MKF`
- `VOC.MKF` 或 `SOUNDS.MKF`

打包时若上述文件或音效文件组缺失，脚本会显示警告、跳过缺失项并继续生成 SD 包。

## 可选兼容文件

不同游戏版本可能还包含以下文件。放入此目录后也会自动打包，但当前 MiaOS 移植版不将它们作为启动必需文件：

- `MIDI.MKF`
- `M.MSG`
- `WORD.DAT`
- `SETUP.DAT`

存档 `1.rpg` 至 `5.rpg`、运行时配置 `sdlpal.cfg` 和固件 `lava_pal.bin` 不应放在此目录。
