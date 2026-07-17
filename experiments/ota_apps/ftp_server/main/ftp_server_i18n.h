#pragma once

typedef struct {
  const char *title;
  const char *use_pasv_exit;
} FtpServerText;

const FtpServerText *ftp_server_text(void);
