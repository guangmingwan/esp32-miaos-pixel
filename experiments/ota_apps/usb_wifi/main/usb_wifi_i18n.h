#pragma once

typedef struct {
  const char *title_usb_wifi;
  const char *scanning;
  const char *no_networks;
  const char *select_wifi;
  const char *enter_password;
  const char *connecting;
  const char *connected;
  const char *disconnected;
  const char *ncm_ready;
  const char *list_hint;
  const char *rescan_exit_hint;
  const char *kb_hint;
  const char *key_done;
  const char *key_delete;
  const char *key_cancel;
  const char *usb_error_fmt;
  const char *error_fmt;
  const char *connect_failed;
  const char *wifi_timeout;
  const char *retry_back_hint;
  const char *bridge_title;
  const char *signal_fmt;
  const char *signal_unavailable;
  const char *ip_fmt;
  const char *gateway_fmt;
  const char *mask_fmt;
  const char *mac_fmt;
  const char *link_speed;
  const char *dns_fmt;
  const char *running_hint;
  const char *exit_hint;
  const char *saved_fmt;
  const char *saved_controls;
  const char *win10_tips;
  const char *win10_step1;
  const char *win10_step2;
  const char *win10_step3;
} UsbWifiText;

const UsbWifiText *usb_wifi_text(void);
