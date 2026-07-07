/*
 * ftp_server_main — ported from experiments/elf_apps/ftp_server/src/main.c
 */

#include "mia_host_abi.h"

#include <stdint.h>
#include <stdio.h>

static MiaHostFtpStatus status;

static void refresh_status(void) {
  mia_host_ftp_get_status(&status);
}

static void draw_ftp_server(void) {
  char line[48];

  refresh_status();
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 6, "FTP Server", MIA_HOST_BLACK, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 34, status.status,
                     status.running ? MIA_HOST_GREEN : MIA_HOST_YELLOW, MIA_HOST_BLACK);
  mia_host_draw_text(4, 56, status.ssid, MIA_HOST_CYAN, MIA_HOST_BLACK);
  snprintf(line, sizeof(line), "ftp://%s", status.ip);
  mia_host_draw_text(4, 82, line, MIA_HOST_YELLOW, MIA_HOST_BLACK);
  snprintf(line, sizeof(line), "User: %s", status.user);
  mia_host_draw_text(4, 108, line, MIA_HOST_WHITE, MIA_HOST_BLACK);
  snprintf(line, sizeof(line), "Pass: %s", status.pass);
  mia_host_draw_text(4, 126, line, MIA_HOST_WHITE, MIA_HOST_BLACK);
  mia_host_draw_text(4, 222, "Use PASV  SEL+ST:Exit", MIA_HOST_GRAY, MIA_HOST_BLACK);
  mia_host_present();
}

int ftp_server_main_impl(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  if (mia_host_abi_version() != 2) {
    mia_host_log("ftp_server.app: unsupported MiaOS host ABI");
    return 1;
  }

  uint32_t last_draw_ms = 0;
  mia_host_ftp_start();
  draw_ftp_server();
  while (1) {
    mia_host_buttons_poll();
    if (mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
        mia_host_button_down(MIA_HOST_BUTTON_START)) {
      break;
    }
    uint32_t now_ms = mia_host_millis();
    mia_host_ftp_poll();
    if (now_ms - last_draw_ms >= 500) {
      last_draw_ms = now_ms;
      draw_ftp_server();
    }
    mia_host_delay_ms(20);
  }

  mia_host_ftp_stop();
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_present();
  mia_host_log("ftp_server.app: exit by SEL+ST");
  return 0;
}
