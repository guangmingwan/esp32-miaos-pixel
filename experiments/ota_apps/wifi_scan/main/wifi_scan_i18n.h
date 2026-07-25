#pragma once

typedef struct {
  const char *title;
  const char *scanning;
  const char *scan_failed;
  const char *no_networks;
  const char *found_fmt;
  const char *scroll_exit;
  const char *scan_exit;
} WifiScanText;

const WifiScanText *wifi_scan_text(void);
