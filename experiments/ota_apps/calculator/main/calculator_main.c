#include "calculator_i18n.h"
#include "mia_host_abi.h"

#include <stdint.h>
#include <stdio.h>

static const char KEYS[16] = {'7', '8', '9', '/', '4', '5', '6', '*',
                              '1', '2', '3', '-', 'C', '0', '=', '+'};

static uint8_t selected_key;
static int32_t accumulator;
static int32_t entry;
static char pending_op;
static uint8_t has_accumulator;
static uint8_t error_state;

static uint8_t exit_pressed(void) {
  return mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
         mia_host_button_down(MIA_HOST_BUTTON_START);
}

static void reset_calculator(void) {
  accumulator = 0;
  entry = 0;
  pending_op = 0;
  has_accumulator = 0;
  error_state = 0;
}

static uint8_t apply_pending_op(void) {
  if (!has_accumulator) {
    accumulator = entry;
    has_accumulator = 1;
    return 1;
  }

  switch (pending_op) {
    case '+': accumulator += entry; return 1;
    case '-': accumulator -= entry; return 1;
    case '*': accumulator *= entry; return 1;
    case '/':
      if (entry == 0) {
        error_state = 1;
        return 0;
      }
      accumulator /= entry;
      return 1;
  }
  accumulator = entry;
  return 1;
}

static void press_calculator_key(char key) {
  if (key == 'C') {
    reset_calculator();
    return;
  }
  if (error_state) {
    return;
  }
  if (key >= '0' && key <= '9') {
    if (entry <= 9999999) {
      entry = entry * 10 + (key - '0');
    }
    return;
  }
  if (key == '=') {
    if (apply_pending_op()) {
      entry = accumulator;
      pending_op = 0;
      has_accumulator = 0;
    }
    return;
  }
  if (apply_pending_op()) {
    pending_op = key;
    entry = 0;
  }
}

static void draw_calculator(void) {
  const CalculatorText *text = calculator_text();
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, mia_host_text_y_centered(0, 20), text->title, MIA_HOST_BLACK,
                     MIA_HOST_YELLOW);

  char line[32];
  mia_host_fill_rect(20, 32, 280, 24, MIA_HOST_BLUE);
  mia_host_fill_rect(21, 33, 278, 22, MIA_HOST_BLACK);
  if (error_state) {
    snprintf(line, sizeof(line), "%s", text->error_div0);
  } else {
    snprintf(line, sizeof(line), "%ld", (long)entry);
  }
  mia_host_draw_text(28, 40, line, error_state ? MIA_HOST_RED : MIA_HOST_WHITE,
                     MIA_HOST_BLACK);

  if (pending_op != 0) {
    snprintf(line, sizeof(line), "%ld %c", (long)accumulator, pending_op);
    mia_host_draw_text(28, 64, line, MIA_HOST_GRAY, MIA_HOST_BLACK);
  } else {
    mia_host_draw_text(28, 64, text->a_ok, MIA_HOST_GRAY, MIA_HOST_BLACK);
  }

  for (uint8_t i = 0; i < 16; ++i) {
    int32_t x = 40 + (i % 4) * 60;
    int32_t y = 92 + (i / 4) * 32;
    uint8_t bg = i == selected_key ? MIA_HOST_BLUE : MIA_HOST_BLACK;
    uint8_t fg = i == selected_key ? MIA_HOST_YELLOW : MIA_HOST_WHITE;
    char key_text[2] = {KEYS[i], 0};
    mia_host_fill_rect(x, y, 48, 24, bg);
    mia_host_draw_text(x + 20, y + 8, key_text, fg, bg);
  }

  mia_host_draw_text(20, 222, text->exit_hint, MIA_HOST_GRAY, MIA_HOST_BLACK);
  mia_host_present();
}

int calculator_main_impl(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  if (mia_host_abi_version() != 2) {
    return 1;
  }
  selected_key = 0;
  reset_calculator();
  draw_calculator();

  while (1) {
    mia_host_buttons_poll();
    if (exit_pressed()) {
      break;
    }
    uint8_t changed = 0;
    if (mia_host_button_pressed(MIA_HOST_BUTTON_UP) && selected_key >= 4) {
      selected_key -= 4;
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN) && selected_key + 4 < 16) {
      selected_key += 4;
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_LEFT) && selected_key % 4 > 0) {
      --selected_key;
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_RIGHT) && selected_key % 4 < 3) {
      ++selected_key;
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
      press_calculator_key(KEYS[selected_key]);
      changed = 1;
    }
    if (changed) {
      draw_calculator();
    }
    mia_host_delay_ms(20);
  }

  mia_host_clear(MIA_HOST_BLACK);
  mia_host_present();
  return 0;
}
