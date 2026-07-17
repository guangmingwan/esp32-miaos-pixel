#include "wifi_scan_i18n.h"

#include "mia_host_abi.h"

static const WifiScanText WS_EN = {
    .title = "WiFi Scan",
    .no_networks = "No networks",
    .rescan = "A:Rescan",
    .scroll_exit = "UP/DN Scroll  SEL+ST:Exit",
    .scan_exit = "A:Scan  SEL+ST:Exit",
};

static const WifiScanText WS_ZH = {
    .title = "WiFi 扫描",
    .no_networks = "未找到网络",
    .rescan = "A:重新扫描",
    .scroll_exit = "上/下:滚动 SEL+ST:退出",
    .scan_exit = "A:扫描 SEL+ST:退出",
};

const WifiScanText *wifi_scan_text(void) {
  return mia_host_language() == 1 ? &WS_ZH : &WS_EN;
}
