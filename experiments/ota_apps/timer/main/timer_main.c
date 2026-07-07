#include "mia_host_abi.h"

#include <stdint.h>
#include <stdio.h>

static uint8_t running;
static uint32_t elapsed_ms;
static uint32_t last_start_ms;
static uint32_t last_draw_ms;

static uint32_t current_elapsed(uint32_t now_ms) {
  if (!running) {
    return elapsed_ms;
  }
  return elapsed_ms + now_ms - last_start_ms;
}

static void draw_timer(uint32_t now_ms) {
  uint32_t total_seconds = current_elapsed(now_ms) / 1000;
  uint32_t minutes = total_seconds / 60;
  uint32_t seconds = total_seconds % 60;
  uint32_t tenths = (current_elapsed(now_ms) / 100) % 10;
  char line[24];

  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 6, "Timer", MIA_HOST_BLACK, MIA_HOST_YELLOW);
  mia_host_draw_text(118, 54, running ? "RUNNING" : "PAUSED",
                     running ? MIA_HOST_GREEN : MIA_HOST_YELLOW, MIA_HOST_BLACK);
  snprintf(line, sizeof(line), "%02lu:%02lu.%lu", (unsigned long)minutes,
           (unsigned long)seconds, (unsigned long)tenths);
  mia_host_draw_text(110, 96, line, MIA_HOST_WHITE, MIA_HOST_BLACK);
  mia_host_draw_text(82, 206, "A:Start/Pause", MIA_HOST_GRAY, MIA_HOST_BLACK);
  mia_host_draw_text(58, 222, "LT+RT:Reset  SEL+ST:Exit", MIA_HOST_GRAY,
                     MIA_HOST_BLACK);
  mia_host_present();
}

int timer_main_impl(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  running = 0;
  elapsed_ms = 0;
  last_start_ms = mia_host_millis();
  last_draw_ms = 0;
  draw_timer(last_start_ms);
  while (1) {
    mia_host_buttons_poll();
    if (mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
        mia_host_button_down(MIA_HOST_BUTTON_START)) {
      break;
    }
    uint32_t now_ms = mia_host_millis();
    uint8_t changed = 0;
    uint8_t r_down = mia_host_button_down(MIA_HOST_BUTTON_R);

    if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
      if (running) {
        elapsed_ms += now_ms - last_start_ms;
        running = 0;
      } else {
        last_start_ms = now_ms;
        running = 1;
      }
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_L) && r_down) {
      running = 0;
      elapsed_ms = 0;
      changed = 1;
    }
    if (changed || now_ms - last_draw_ms >= 100) {
      last_draw_ms = now_ms;
      draw_timer(now_ms);
    }
    mia_host_delay_ms(20);
  }

  mia_host_clear(MIA_HOST_BLACK);
  mia_host_present();
  return 0;
}
