/*
 * minesweeper_main — ported from experiments/elf_apps/minesweeper/src/main.c
 */

#include "mia_host_abi.h"
#include "minesweeper_i18n.h"

#include <stdint.h>
#include <string.h>

#define BOARD_W 8
#define BOARD_H 6
#define MINE_COUNT 8
#define CELL_SIZE 24
#define BOARD_X 64
#define BOARD_Y 36

static uint8_t mines[BOARD_H][BOARD_W];
static uint8_t revealed[BOARD_H][BOARD_W];
static uint8_t flags[BOARD_H][BOARD_W];
static uint8_t cursor_x;
static uint8_t cursor_y;
static uint8_t started;
static uint8_t game_over;
static uint8_t won;
static uint32_t rng_state;

static uint8_t exit_pressed(void) {
  return mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
         mia_host_button_down(MIA_HOST_BUTTON_START);
}

static uint32_t next_rand(void) {
  rng_state = rng_state * 1664525UL + 1013904223UL;
  return rng_state;
}

static uint8_t in_bounds(int8_t x, int8_t y) {
  return x >= 0 && x < BOARD_W && y >= 0 && y < BOARD_H;
}

static uint8_t adjacent_mines(uint8_t x, uint8_t y) {
  uint8_t count = 0;
  for (int8_t dy = -1; dy <= 1; ++dy) {
    for (int8_t dx = -1; dx <= 1; ++dx) {
      if ((dx != 0 || dy != 0) && in_bounds(x + dx, y + dy) &&
          mines[y + dy][x + dx]) {
        ++count;
      }
    }
  }
  return count;
}

static void reset_game(void) {
  memset(mines, 0, sizeof(mines));
  memset(revealed, 0, sizeof(revealed));
  memset(flags, 0, sizeof(flags));
  cursor_x = 0;
  cursor_y = 0;
  started = 0;
  game_over = 0;
  won = 0;
}

static void place_mines(uint8_t safe_x, uint8_t safe_y) {
  uint8_t placed = 0;
  while (placed < MINE_COUNT) {
    uint8_t x = next_rand() % BOARD_W;
    uint8_t y = next_rand() % BOARD_H;
    if (mines[y][x] || (x == safe_x && y == safe_y)) {
      continue;
    }
    mines[y][x] = 1;
    ++placed;
  }
  started = 1;
}

static void reveal_cell(uint8_t x, uint8_t y) {
  if (flags[y][x] || revealed[y][x] || game_over) {
    return;
  }
  if (!started) {
    place_mines(x, y);
  }
  revealed[y][x] = 1;
  if (mines[y][x]) {
    game_over = 1;
    return;
  }
  if (adjacent_mines(x, y) == 0) {
    for (int8_t dy = -1; dy <= 1; ++dy) {
      for (int8_t dx = -1; dx <= 1; ++dx) {
        int8_t next_x = x + dx;
        int8_t next_y = y + dy;
        if ((dx != 0 || dy != 0) && in_bounds(next_x, next_y)) {
          reveal_cell(next_x, next_y);
        }
      }
    }
  }
}

static void update_win_state(void) {
  if (game_over) {
    return;
  }
  uint8_t hidden_safe = 0;
  for (uint8_t y = 0; y < BOARD_H; ++y) {
    for (uint8_t x = 0; x < BOARD_W; ++x) {
      if (!mines[y][x] && !revealed[y][x]) {
        ++hidden_safe;
      }
    }
  }
  if (hidden_safe == 0) {
    won = 1;
    game_over = 1;
  }
}

static void draw_game(void) {
  const MinesweeperText *text = minesweeper_text();
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_fill_rect(0, 0, mia_host_screen_width(), 20, MIA_HOST_YELLOW);
  mia_host_draw_text(4, 6, text->title, MIA_HOST_BLACK, MIA_HOST_YELLOW);

  for (uint8_t y = 0; y < BOARD_H; ++y) {
    for (uint8_t x = 0; x < BOARD_W; ++x) {
      int32_t px = BOARD_X + x * CELL_SIZE;
      int32_t py = BOARD_Y + y * CELL_SIZE;
      uint8_t bg = revealed[y][x] ? MIA_HOST_GRAY : MIA_HOST_BLUE;
      char text[2] = {' ', 0};
      uint8_t fg = MIA_HOST_WHITE;
      if (game_over && mines[y][x]) {
        bg = MIA_HOST_RED;
        text[0] = '*';
      } else if (flags[y][x]) {
        text[0] = 'F';
        fg = MIA_HOST_YELLOW;
      } else if (revealed[y][x]) {
        uint8_t count = adjacent_mines(x, y);
        if (count > 0) {
          text[0] = '0' + count;
          fg = MIA_HOST_BLACK;
        }
      }
      mia_host_fill_rect(px, py, CELL_SIZE - 1, CELL_SIZE - 1, bg);
      if (x == cursor_x && y == cursor_y) {
        mia_host_fill_rect(px, py, CELL_SIZE - 1, 2, MIA_HOST_YELLOW);
        mia_host_fill_rect(px, py + CELL_SIZE - 3, CELL_SIZE - 1, 2, MIA_HOST_YELLOW);
        mia_host_fill_rect(px, py, 2, CELL_SIZE - 1, MIA_HOST_YELLOW);
        mia_host_fill_rect(px + CELL_SIZE - 3, py, 2, CELL_SIZE - 1, MIA_HOST_YELLOW);
      }
      mia_host_draw_text(px + 9, py + 8, text, fg, bg);
    }
  }

  if (game_over) {
    mia_host_draw_text(84, 196, won ? text->you_win : text->boom,
                       won ? MIA_HOST_GREEN : MIA_HOST_RED, MIA_HOST_BLACK);
  } else {
    mia_host_draw_text(38, 206, text->controls, MIA_HOST_GRAY,
                       MIA_HOST_BLACK);
  }
  mia_host_draw_text(92, 222, text->exit_hint, MIA_HOST_GRAY, MIA_HOST_BLACK);
  mia_host_present();
}

int minesweeper_main_impl(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  if (mia_host_abi_version() != 2) {
    return 1;
  }
  rng_state = mia_host_millis() | 1;
  reset_game();
  draw_game();
  while (1) {
    mia_host_buttons_poll();
    if (exit_pressed()) {
      break;
    }
    uint8_t changed = 0;
    if (mia_host_button_pressed(MIA_HOST_BUTTON_UP) && cursor_y > 0) {
      --cursor_y;
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN) && cursor_y + 1 < BOARD_H) {
      ++cursor_y;
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_LEFT) && cursor_x > 0) {
      --cursor_x;
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_RIGHT) && cursor_x + 1 < BOARD_W) {
      ++cursor_x;
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_B) && !game_over &&
        !revealed[cursor_y][cursor_x]) {
      flags[cursor_y][cursor_x] = !flags[cursor_y][cursor_x];
      changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
      if (game_over) {
        reset_game();
      } else {
        reveal_cell(cursor_x, cursor_y);
        update_win_state();
      }
      changed = 1;
    }
    if (changed) {
      draw_game();
    }
    mia_host_delay_ms(20);
  }
  mia_host_clear(MIA_HOST_BLACK);
  mia_host_present();
  return 0;
}
