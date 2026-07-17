#include "usb_wifi_i18n.h"

#include "mia_host_abi.h"

static const UsbWifiText UW_EN = {
    .title_usb_wifi = "USB WiFi",
    .scanning = "Scanning WiFi...",
    .no_networks = "No networks found",
    .select_wifi = "Select WiFi",
    .enter_password = "Enter Password",
    .connecting = "Connecting...",
    .connected = "Connected",
    .disconnected = "Disconnected, retry",
    .ncm_ready = "NCM ready",
    .list_hint = "UP/DN:A  B:Back",
    .kb_hint = "A:Key  B:Back  START:Done",
    .running_hint = "SEL+ST:Exit",
    .exit_hint = "SEL+ST:Exit",
};

static const UsbWifiText UW_ZH = {
    .title_usb_wifi = "USB 网卡",
    .scanning = "正在扫描WiFi...",
    .no_networks = "未找到网络",
    .select_wifi = "选择WiFi",
    .enter_password = "输入密码",
    .connecting = "连接中...",
    .connected = "已连接",
    .disconnected = "已断开，重试",
    .ncm_ready = "NCM 就绪",
    .list_hint = "上/下:A  B:返回",
    .kb_hint = "A:输入  B:返回  START:完成",
    .running_hint = "SEL+ST:退出",
    .exit_hint = "SEL+ST:退出",
};

const UsbWifiText *usb_wifi_text(void) {
  return mia_host_language() == 1 ? &UW_ZH : &UW_EN;
}
