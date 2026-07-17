#include "wifi_files_i18n.h"

#include "mia_host_abi.h"

static const WifiFilesText WF_EN = {
    .title = "WiFi Files",
    .mode_ap = "Mode: AP",
    .mode_router = "Mode: Router",
    .guest_exit = "Guest access  SEL+ST:Exit",
};

static const WifiFilesText WF_ZH = {
    .title = "WiFi 文件",
    .mode_ap = "模式: 热点",
    .mode_router = "模式: 路由器",
    .guest_exit = "访客访问  SEL+ST:退出",
};

const WifiFilesText *wifi_files_text(void) {
  return mia_host_language() == 1 ? &WF_ZH : &WF_EN;
}
