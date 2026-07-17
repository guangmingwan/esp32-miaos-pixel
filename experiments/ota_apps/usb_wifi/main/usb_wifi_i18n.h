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
  const char *kb_hint;
  const char *running_hint;
  const char *exit_hint;
} UsbWifiText;

const UsbWifiText *usb_wifi_text(void);
