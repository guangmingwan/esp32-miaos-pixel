#include "wifi_scan_i18n.h"

#include "mia_host_abi.h"

static const WifiScanText WS_EN = {
    .title = "WiFi Scan",
    .scanning = "Scanning...",
    .scan_failed = "Scan failed",
    .no_networks = "No networks",
    .found_fmt = "%ld found  %ld/%ld",
    .scroll_exit = "UP/DN:Move LT/RT:Page A:Scan SEL+ST:Exit",
    .scan_exit = "A:Scan  SEL+ST:Exit",
};

static const WifiScanText WS_ZH = {
    .title = "WiFi 扫描",
    .scanning = "正在扫描...",
    .scan_failed = "扫描失败",
    .no_networks = "未找到网络",
    .found_fmt = "发现 %ld 个  %ld/%ld",
    .scroll_exit = "上/下:移动 左/右:翻页 A:扫描 SEL+ST:退出",
    .scan_exit = "A:扫描 SEL+ST:退出",
};

const WifiScanText *wifi_scan_text(void) {
  return mia_host_language() == 1 ? &WS_ZH : &WS_EN;
}
