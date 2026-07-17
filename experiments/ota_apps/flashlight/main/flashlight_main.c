#include "flashlight_i18n.h"
#include "mia_host_abi.h"

#include <stdint.h>

static uint8_t light_on;

static void draw_flashlight(void) {
  const FlashlightText *text = flashlight_text();
  mia_host_clear(light_on ? MIA_HOST_WHITE : MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 6, text->title, MIA_HOST_BLACK, MIA_HOST_YELLOW);
  mia_host_draw_text(132, 104, light_on ? text->light_on : text->light_off,
                     light_on ? MIA_HOST_BLACK : MIA_HOST_WHITE,
                     light_on ? MIA_HOST_WHITE : MIA_HOST_BLACK);
  mia_host_draw_text(84, 222, text->controls,
                     light_on ? MIA_HOST_BLACK : MIA_HOST_GRAY,
                     light_on ? MIA_HOST_WHITE : MIA_HOST_BLACK);
  mia_host_present();
}

int flashlight_main_impl(int argc, char *argv[]) {
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
