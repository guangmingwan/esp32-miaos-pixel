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

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  uint8_t previous_up = 0;
  uint8_t previous_down = 0;
  uint8_t previous_a = 0;
  run_scan();
  draw_wifi_scan();
  while (!(mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
           mia_host_button_down(MIA_HOST_BUTTON_START))) {
    uint8_t up_down = mia_host_button_down(MIA_HOST_BUTTON_UP);
    uint8_t down_down = mia_host_button_down(MIA_HOST_BUTTON_DOWN);
    uint8_t a_down = mia_host_button_down(MIA_HOST_BUTTON_A);
    uint8_t up_pressed = up_down && !previous_up;
    uint8_t down_pressed = down_down && !previous_down;
    uint8_t a_pressed = a_down && !previous_a;
    previous_up = up_down;
    previous_down = down_down;
    previous_a = a_down;

    if (a_pressed) {
      run_scan();
      draw_wifi_scan();
    } else if (up_pressed && first_network > 0) {
      --first_network;
      draw_wifi_scan();
    } else if (down_pressed && first_network + MAX_NETWORKS < network_count) {
      ++first_network;
      draw_wifi_scan();
    }
    mia_host_delay_ms(20);
  }

  mia_host_wifi_off();
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_present();
  return 0;
}
