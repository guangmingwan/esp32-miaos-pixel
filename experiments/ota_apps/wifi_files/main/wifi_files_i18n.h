#pragma once

typedef struct {
  const char *title;
  const char *mode_ap;
  const char *mode_router;
  const char *guest_exit;
} WifiFilesText;

const WifiFilesText *wifi_files_text(void);
