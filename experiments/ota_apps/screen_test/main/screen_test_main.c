#include "mia_host_abi.h"

#include <stdint.h>

typedef struct ScreenPattern {
  const char *label;
  uint16_t color;
} ScreenPattern;

static const ScreenPattern SCREEN_PATTERNS[] = {
    {"RED", 0xF800},
    {"GREEN", 0x07E0},
    {"BLUE", 0x001F},
    {"WHITE", 0xFFFF},
    {"BLACK", 0x0000},
};

static uint8_t pattern_index;
static uint32_t last_step_ms;

static void draw_pattern(void) {
  const ScreenPattern *pattern = &SCREEN_PATTERNS[pattern_index];
  mia_host_fill_screen_rgb565(pattern->color);
}

static void step_pattern(int8_t delta, uint32_t now_ms) {
  int count = (int)(sizeof(SCREEN_PATTERNS) / sizeof(SCREEN_PATTERNS[0]));
  int next = (int)pattern_index + delta;
  if (next < 0) {
    next += count;
  }
  if (next >= count) {
    next -= count;
  }
  pattern_index = (uint8_t)next;
  last_step_ms = now_ms;
  draw_pattern();
}

int screen_test_main_impl(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  pattern_index = 0;
  last_step_ms = mia_host_millis();
  draw_pattern();
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
