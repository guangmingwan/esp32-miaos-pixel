#include "ftp_server_i18n.h"

#include "mia_host_abi.h"

static const FtpServerText FTP_EN = {
    .title = "FTP Server",
    .use_pasv_exit = "Use PASV  SEL+ST:Exit",
};

static const FtpServerText FTP_ZH = {
    .title = "FTP 服务器",
    .use_pasv_exit = "请使用 PASV  SEL+ST:退出",
};

const FtpServerText *ftp_server_text(void) {
  return mia_host_language() == 1 ? &FTP_ZH : &FTP_EN;
}
