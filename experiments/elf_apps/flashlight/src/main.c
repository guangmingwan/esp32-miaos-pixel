#include "mia_host_abi.h"

#include <stdint.h>

static uint8_t light_on;

static void draw_flashlight(void) {
  mia_host_clear(light_on ? MIA_HOST_WHITE : MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 6, "Flashlight", MIA_HOST_BLACK, MIA_HOST_YELLOW);
  mia_host_draw_text(132, 104, light_on ? "LIGHT ON" : "LIGHT OFF",
                     light_on ? MIA_HOST_BLACK : MIA_HOST_WHITE,
                     light_on ? MIA_HOST_WHITE : MIA_HOST_BLACK);
  mia_host_draw_text(84, 222, "A:Toggle  SEL+ST:Exit",
                     light_on ? MIA_HOST_BLACK : MIA_HOST_GRAY,
                     light_on ? MIA_HOST_WHITE : MIA_HOST_BLACK);
  mia_host_present();
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  light_on = 1;
  mia_host_backlight_set(1);
  draw_flashlight();
  while (1) {
    mia_host_buttons_poll();
    if (mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
        mia_host_button_down(MIA_HOST_BUTTON_START)) {
      break;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
      light_on = light_on ? 0 : 1;
      mia_host_backlight_set(light_on);
      draw_flashlight();
    }
    mia_host_delay_ms(20);
  }

  mia_host_backlight_set(1);
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_present();
  return 0;
}
