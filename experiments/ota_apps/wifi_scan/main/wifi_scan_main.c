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
static uint8_t scan_failed;

static int32_t signal_quality(int32_t rssi) {
  if (rssi >= -50) return 100;
  if (rssi <= -100) return 0;
  return (rssi + 100) * 2;
}

static void draw_message(const char *message) {
  const WifiScanText *text = wifi_scan_text();
  int32_t x = (mia_host_screen_width() - mia_host_text_width(message)) / 2;
  if (x < 4) x = 4;
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, mia_host_text_y_centered(0, 20), text->title,
                     MIA_HOST_BLACK, MIA_HOST_YELLOW);
  mia_host_draw_text(x, mia_host_text_y_centered(78, 64), message,
                     MIA_HOST_YELLOW, MIA_HOST_BLACK);
  mia_host_present();
}

static void draw_wifi_scan(void) {
  const WifiScanText *text = wifi_scan_text();
  char line[48];

  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, mia_host_text_y_centered(0, 20), text->title, MIA_HOST_BLACK,
                     MIA_HOST_YELLOW);

  if (network_count <= 0) {
    const char *message = scan_failed ? text->scan_failed : text->no_networks;
    int32_t x = (mia_host_screen_width() - mia_host_text_width(message)) / 2;
    if (x < 4) x = 4;
    mia_host_draw_text(x, 92, message, scan_failed ? MIA_HOST_RED : MIA_HOST_YELLOW,
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
    snprintf(line, sizeof(line), "%4lddBm %3ld%% %.18s", (long)network->rssi,
             (long)signal_quality(network->rssi), network->ssid);
    uint8_t selected = first_network + index == selected_line;
    uint8_t fg = selected ? MIA_HOST_BLACK : MIA_HOST_WHITE;
    uint8_t bg = selected ? MIA_HOST_YELLOW : MIA_HOST_BLACK;
    int32_t y = LIST_Y + index * LINE_HEIGHT;
    if (selected) {
      mia_host_fill_rect(4, y - 1, 312, LINE_HEIGHT, bg);
    }
    mia_host_draw_text(10, y + 2, line, fg, bg);
  }

  snprintf(line, sizeof(line), text->found_fmt, (long)network_count,
           (long)selected_line + 1, (long)network_count);
  mia_host_draw_text(8, STATUS_Y, line, MIA_HOST_GREEN, MIA_HOST_BLACK);
  mia_host_draw_text(8, HINT_Y, text->scroll_exit, MIA_HOST_GRAY,
                     MIA_HOST_BLACK);
  mia_host_present();
}

static void run_scan(void) {
  draw_message(wifi_scan_text()->scanning);
  network_count = mia_host_wifi_scan(networks, MAX_NETWORKS);
  scan_failed = network_count < 0;
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
    } else if (mia_host_button_pressed(MIA_HOST_BUTTON_LEFT)) {
      if (selected_line > 0) {
        selected_line -= VISIBLE_LINES;
        if (selected_line < 0) selected_line = 0;
        clamp_selection();
        changed = 1;
      }
    } else if (mia_host_button_pressed(MIA_HOST_BUTTON_RIGHT)) {
      if (selected_line + 1 < network_count) {
        selected_line += VISIBLE_LINES;
        if (selected_line >= network_count) selected_line = network_count - 1;
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
