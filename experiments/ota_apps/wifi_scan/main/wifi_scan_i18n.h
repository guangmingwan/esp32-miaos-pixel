#pragma once

typedef struct {
  const char *title;
  const char *no_networks;
  const char *rescan;
  const char *scroll_exit;
  const char *scan_exit;
} WifiScanText;

const WifiScanText *wifi_scan_text(void);
