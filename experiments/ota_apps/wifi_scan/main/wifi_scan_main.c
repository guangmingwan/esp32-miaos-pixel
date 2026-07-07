/*
 * wifi_scan_main — ported from experiments/elf_apps/wifi_scan/src/main.c
 */

#include "mia_host_abi.h"

#include <stdint.h>
#include <stdio.h>

#define MAX_NETWORKS 16

static MiaHostWifiNetwork networks[MAX_NETWORKS];
static int32_t network_count;
static int32_t first_network;

static void draw_wifi_scan(void) {
  char line[48];

  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 6, "WiFi Scan", MIA_HOST_BLACK, MIA_HOST_YELLOW);
  if (network_count <= 0) {
    mia_host_draw_text(116, 92, "No networks", MIA_HOST_YELLOW, MIA_HOST_BLACK);
    mia_host_draw_text(80, 222, "A:Scan  SEL+ST:Exit", MIA_HOST_GRAY, MIA_HOST_BLACK);
    mia_host_present();
    return;
  }

  int32_t visible = network_count - first_network;
  if (visible > MAX_NETWORKS) {
    visible = MAX_NETWORKS;
  }
  for (int32_t index = 0; index < visible; ++index) {
    MiaHostWifiNetwork *network = &networks[first_network + index];
    snprintf(line, sizeof(line), "%2lddB %.16s", (long)network->rssi, network->ssid);
    mia_host_draw_text(8, 36 + index * 18, line,
                       index == 0 ? MIA_HOST_YELLOW : MIA_HOST_WHITE, MIA_HOST_BLACK);
  }
  snprintf(line, sizeof(line), "%ld found A:Rescan", (long)network_count);
  mia_host_draw_text(8, 206, line, MIA_HOST_GRAY, MIA_HOST_BLACK);
  mia_host_draw_text(8, 222, "UP/DN Scroll  SEL+ST:Exit", MIA_HOST_GRAY, MIA_HOST_BLACK);
  mia_host_present();
}

static void run_scan(void) {
  network_count = mia_host_wifi_scan(networks, MAX_NETWORKS);
  if (network_count < 0) {
    network_count = 0;
  }
  first_network = 0;
}

int wifi_scan_main_impl(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  if (mia_host_abi_version() != 2) {
    mia_host_log("wifi_scan.app: unsupported MiaOS host ABI");
    return 1;
  }

  run_scan();
  draw_wifi_scan();
  while (1) {
    mia_host_buttons_poll();
    if (mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
        mia_host_button_down(MIA_HOST_BUTTON_START)) {
      break;
    }

    if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
      run_scan();
      draw_wifi_scan();
    } else if (mia_host_button_pressed(MIA_HOST_BUTTON_UP) && first_network > 0) {
      --first_network;
      draw_wifi_scan();
    } else if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN) &&
               first_network + MAX_NETWORKS < network_count) {
      ++first_network;
      draw_wifi_scan();
    }
    mia_host_delay_ms(20);
  }

  mia_host_wifi_off();
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_present();
  mia_host_log("wifi_scan.app: exit by SEL+ST");
  return 0;
}
