#include "mia_host_abi.h"
#include "screen_test_i18n.h"

#include <stdint.h>

static const uint16_t PATTERN_COLORS[] = {
    0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000,
};

static const char *pattern_label(uint8_t index, const ScreenTestText *text) {
  switch (index) {
    case 0: return text->pattern_red;
    case 1: return text->pattern_green;
    case 2: return text->pattern_blue;
    case 3: return text->pattern_white;
    default: return text->pattern_black;
  }
}

static uint8_t pattern_index;
static uint32_t last_step_ms;

static void draw_pattern(void) {
  mia_host_fill_screen_rgb565(PATTERN_COLORS[pattern_index]);
}

static void draw_overlay(void) {
  const ScreenTestText *text = screen_test_text();
  draw_pattern();
  uint16_t bg_inv = ~PATTERN_COLORS[pattern_index];
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 6, text->title, MIA_HOST_BLACK, MIA_HOST_YELLOW);
  mia_host_draw_text(120, 110, pattern_label(pattern_index, text),
                     MIA_HOST_WHITE, MIA_HOST_BLACK);
  mia_host_fill_rect(0, 222, mia_host_screen_width(), 18, MIA_HOST_BLACK);
  mia_host_draw_text(76, 224, text->exit_hint, MIA_HOST_GRAY, MIA_HOST_BLACK);
  mia_host_present();
}

static void step_pattern(int8_t delta, uint32_t now_ms) {
  int count = (int)(sizeof(PATTERN_COLORS) / sizeof(PATTERN_COLORS[0]));
  int next = (int)pattern_index + delta;
  if (next < 0) {
    next += count;
  }
  if (next >= count) {
    next -= count;
  }
  pattern_index = (uint8_t)next;
  last_step_ms = now_ms;
  draw_overlay();
}

int screen_test_main_impl(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  pattern_index = 0;
  last_step_ms = mia_host_millis();
  draw_overlay();
  while (1) {
    mia_host_buttons_poll();
    if (mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
        mia_host_button_down(MIA_HOST_BUTTON_START)) {
      break;
    }
    uint32_t now_ms = mia_host_millis();
    if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
      step_pattern(1, now_ms);
      continue;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
      step_pattern(-1, now_ms);
      continue;
    }
    if (now_ms - last_step_ms >= 1500) {
      step_pattern(1, now_ms);
    }
    mia_host_delay_ms(20);
  }

  mia_host_fill_screen_rgb565(0x0000);
  return 0;
}
