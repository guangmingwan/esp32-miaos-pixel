/*
 * wifi_scan_main — ported from experiments/elf_apps/wifi_scan/src/main.c
 */

#include "mia_host_abi.h"
#include "wifi_scan_i18n.h"

#include <stdint.h>
#include <stdio.h>

#define MAX_NETWORKS 32
#define VISIBLE_LINES 9
#define LINE_HEIGHT 18
#define LIST_Y 26
#define STATUS_Y 196
#define HINT_Y 224

static MiaHostWifiNetwork networks[MAX_NETWORKS];
static int32_t network_count;
static int32_t first_network;
static int32_t selected_line;

static void draw_wifi_scan(void) {
  const WifiScanText *text = wifi_scan_text();
  char line[48];

  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 6, text->title, MIA_HOST_BLACK, MIA_HOST_YELLOW);

  if (network_count <= 0) {
    mia_host_draw_text(116, 92, text->no_networks, MIA_HOST_YELLOW,
                       MIA_HOST_BLACK);
    mia_host_draw_text(80, HINT_Y, text->scan_exit, MIA_HOST_GRAY,
                       MIA_HOST_BLACK);
    mia_host_present();
    return;
  }

  int32_t visible = network_count - first_network;
  if (visible > VISIBLE_LINES) {
    visible = VISIBLE_LINES;
  }
  for (int32_t index = 0; index < visible; ++index) {
    MiaHostWifiNetwork *network = &networks[first_network + index];
    snprintf(line, sizeof(line), "%4lddB %.24s", (long)network->rssi,
             network->ssid);
    uint8_t fg = (index == selected_line) ? MIA_HOST_BLACK : MIA_HOST_WHITE;
    uint8_t bg = (index == selected_line) ? MIA_HOST_YELLOW : MIA_HOST_BLACK;
    int32_t y = LIST_Y + index * LINE_HEIGHT;
    if (index == selected_line) {
      mia_host_fill_rect(4, y - 1, 312, LINE_HEIGHT, bg);
    }
    mia_host_draw_text(10, y + 2, line, fg, bg);
  }

  snprintf(line, sizeof(line), "%ld found  %s", (long)network_count,
           text->rescan);
  mia_host_draw_text(8, STATUS_Y, line, MIA_HOST_GREEN, MIA_HOST_BLACK);
  mia_host_draw_text(8, HINT_Y, text->scroll_exit, MIA_HOST_GRAY,
                     MIA_HOST_BLACK);
  mia_host_present();
}

static void run_scan(void) {
  network_count = mia_host_wifi_scan(networks, MAX_NETWORKS);
  if (network_count < 0) {
    network_count = 0;
  }
  first_network = 0;
  selected_line = 0;
}

static void clamp_selection(void) {
  if (network_count <= 0) {
    first_network = 0;
    selected_line = 0;
    return;
  }
  if (selected_line < 0) {
    selected_line = 0;
  }
  if (selected_line >= network_count) {
    selected_line = network_count - 1;
  }
  /* Keep selected line inside the visible window. */
  if (selected_line < first_network) {
    first_network = selected_line;
  }
  while (selected_line >= first_network + VISIBLE_LINES) {
    ++first_network;
  }
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

    uint8_t changed = 0;
    if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
      run_scan();
      changed = 1;
    } else if (mia_host_button_pressed(MIA_HOST_BUTTON_UP)) {
      if (selected_line > 0) {
        --selected_line;
        clamp_selection();
        changed = 1;
      }
    } else if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN)) {
      if (selected_line + 1 < network_count) {
        ++selected_line;
        clamp_selection();
        changed = 1;
      }
    }
    if (changed) {
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
