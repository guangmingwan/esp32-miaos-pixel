#include "mia_host_abi.h"
#include "screen_test_i18n.h"
#include "display_host.h"

#include <stddef.h>
#include <stdint.h>

#define SCREEN_TEST_WIDTH 320
#define SCREEN_TEST_HEIGHT 240
#define PATTERN_ROWS 8
#define PATTERN_SOLID_COUNT 5
#define PATTERN_COUNT 8

static const uint16_t PATTERN_COLORS[] = {
    0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000,
};

static const char *pattern_label(uint8_t index, const ScreenTestText *text) {
  switch (index) {
    case 0: return text->pattern_red;
    case 1: return text->pattern_green;
    case 2: return text->pattern_blue;
    case 3: return text->pattern_white;
    case 4: return text->pattern_black;
    case 5: return text->pattern_checker;
    case 6: return text->pattern_gray_gradient;
    default: return text->pattern_color_gradient;
  }
}

static uint8_t pattern_index;

static uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue) {
  return (uint16_t)(((uint16_t)(red & 0xF8) << 8) |
                    ((uint16_t)(green & 0xFC) << 3) | (blue >> 3));
}

static void draw_generated_pattern(void) {
  uint16_t pattern_rows[PATTERN_ROWS * SCREEN_TEST_WIDTH];
  for (int y_start = 0; y_start < SCREEN_TEST_HEIGHT; y_start += PATTERN_ROWS) {
    int rows = SCREEN_TEST_HEIGHT - y_start;
    if (rows > PATTERN_ROWS) rows = PATTERN_ROWS;
    for (int row = 0; row < rows; ++row) {
      int y = y_start + row;
      for (int x = 0; x < SCREEN_TEST_WIDTH; ++x) {
        uint16_t color;
        if (pattern_index == 5) {
          color = ((x / 16 + y / 16) & 1) ? 0xFFFF : 0x0000;
        } else if (pattern_index == 6) {
          uint8_t value = (uint8_t)((x * 255) / (SCREEN_TEST_WIDTH - 1));
          color = rgb565(value, value, value);
        } else {
          uint8_t red = (uint8_t)((x * 255) / (SCREEN_TEST_WIDTH - 1));
          uint8_t blue = (uint8_t)(((SCREEN_TEST_WIDTH - 1 - x) * 255) /
                                   (SCREEN_TEST_WIDTH - 1));
          uint8_t green = (uint8_t)((y * 255) / (SCREEN_TEST_HEIGHT - 1));
          color = rgb565(red, green, blue);
        }
        pattern_rows[row * SCREEN_TEST_WIDTH + x] = color;
      }
    }
    display_host_present_rgb565_region(pattern_rows, 0, y_start, SCREEN_TEST_WIDTH, rows,
                                       SCREEN_TEST_WIDTH * sizeof(uint16_t));
  }
}

static void draw_pattern(void) {
  if (pattern_index < PATTERN_SOLID_COUNT) {
    mia_host_fill_screen_rgb565(PATTERN_COLORS[pattern_index]);
  } else {
    draw_generated_pattern();
  }
}

static void draw_overlay(void) {
  const ScreenTestText *text = screen_test_text();
  draw_pattern();
  mia_host_clear(255);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, mia_host_text_y_centered(0, 20), text->title, MIA_HOST_BLACK,
                     MIA_HOST_YELLOW);
  const char *label = pattern_label(pattern_index, text);
  int label_x = (mia_host_screen_width() - mia_host_text_width(label)) / 2;
  mia_host_draw_text(label_x, 110, label, MIA_HOST_WHITE, MIA_HOST_BLACK);
  mia_host_fill_rect(0, 222, mia_host_screen_width(), 18, MIA_HOST_BLACK);
  mia_host_draw_text(76, 224, text->exit_hint, MIA_HOST_GRAY, MIA_HOST_BLACK);
  display_host_present_rgb565_overlay(NULL, (uint32_t)mia_host_screen_width(),
                                      (uint32_t)mia_host_screen_height(),
                                      (uint32_t)mia_host_screen_width() * sizeof(uint16_t),
                                      255, 255);
}

static void step_pattern(int8_t delta) {
  int count = PATTERN_COUNT;
  int next = (int)pattern_index + delta;
  if (next < 0) {
    next += count;
  }
  if (next >= count) {
    next -= count;
  }
  pattern_index = (uint8_t)next;
  draw_overlay();
}

int screen_test_main_impl(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  pattern_index = 0;
  draw_overlay();
  while (1) {
    mia_host_buttons_poll();
    if (mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
        mia_host_button_down(MIA_HOST_BUTTON_START)) {
      break;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
      step_pattern(1);
      continue;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
      step_pattern(-1);
      continue;
    }
    mia_host_delay_ms(20);
  }

  mia_host_fill_screen_rgb565(0x0000);
  return 0;
}
