/*
 * wifi_files_main — ported from experiments/elf_apps/wifi_files/src/main.c
 */

#include "wifi_files_i18n.h"
#include "mia_host_abi.h"

#include <stdint.h>
#include <stdio.h>

static MiaHostWifiFilesStatus status;

static void refresh_status(void) {
  mia_host_wifi_files_get_status(&status);
}

static void draw_wifi_files(void) {
  const WifiFilesText *text = wifi_files_text();
  char line[48];

  refresh_status();
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, mia_host_text_y_centered(0, 20), text->title, MIA_HOST_BLACK,
                     MIA_HOST_YELLOW);
  mia_host_draw_text(4, 34, status.status,
                     status.running ? MIA_HOST_GREEN : MIA_HOST_YELLOW, MIA_HOST_BLACK);
  mia_host_draw_text(4, 56, status.ap_mode ? text->mode_ap : text->mode_router, MIA_HOST_CYAN,
                     MIA_HOST_BLACK);
  mia_host_draw_text(4, 78, status.ssid, MIA_HOST_WHITE, MIA_HOST_BLACK);
  snprintf(line, sizeof(line), "http://%s", status.ip);
  mia_host_draw_text(4, 104, line, MIA_HOST_YELLOW, MIA_HOST_BLACK);
  mia_host_draw_text(4, 222, text->guest_exit, MIA_HOST_GRAY,
                     MIA_HOST_BLACK);
  mia_host_present();
}

int wifi_files_main_impl(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  if (mia_host_abi_version() != 2) {
    mia_host_log("wifi_files.app: unsupported MiaOS host ABI");
    return 1;
  }

  uint32_t last_draw_ms = 0;
  mia_host_wifi_files_start();
  draw_wifi_files();
  while (1) {
    mia_host_buttons_poll();
    if (mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
        mia_host_button_down(MIA_HOST_BUTTON_START)) {
      break;
    }
    uint32_t now_ms = mia_host_millis();
    mia_host_wifi_files_poll();
    if (now_ms - last_draw_ms >= 500) {
      last_draw_ms = now_ms;
      draw_wifi_files();
    }
    mia_host_delay_ms(20);
  }

  mia_host_wifi_files_stop();
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_present();
  mia_host_log("wifi_files.app: exit by SEL+ST");
  return 0;
}
