#include "apps/minesweeper_app.h"

#include <Arduino.h>

#include "lava_native_display.h"

extern ButtonState g_allButtons[];

enum LavaPalette : uint8_t {
  LAVA_BLACK = 0,
  LAVA_WHITE = 1,
  LAVA_BLUE = 2,
  LAVA_GREEN = 3,
  LAVA_RED = 4,
  LAVA_YELLOW = 5,
  LAVA_CYAN = 6,
  LAVA_GRAY = 7,
  LAVA_DARK_BLUE = 8,
};

static constexpr uint8_t BOARD_W = 8;
static constexpr uint8_t BOARD_H = 6;
static constexpr uint8_t MINE_COUNT = 8;
static constexpr int16_t CELL_SIZE = 24;
static constexpr int16_t BOARD_X = 64;
static constexpr int16_t BOARD_Y = 36;
static constexpr uint8_t DPAD_UP_INDEX = 10;
static constexpr uint8_t DPAD_DOWN_INDEX = 11;
static constexpr uint8_t DPAD_LEFT_INDEX = 12;
static constexpr uint8_t DPAD_RIGHT_INDEX = 13;

static bool g_mines[BOARD_H][BOARD_W];
static bool g_revealed[BOARD_H][BOARD_W];
static bool g_flags[BOARD_H][BOARD_W];
static uint8_t g_cursorX = 0;
static uint8_t g_cursorY = 0;
static bool g_started = false;
static bool g_gameOver = false;
static bool g_won = false;

static bool inBounds(int8_t x, int8_t y) {
  return x >= 0 && x < BOARD_W && y >= 0 && y < BOARD_H;
}

static uint8_t adjacentMines(uint8_t x, uint8_t y) {
  uint8_t count = 0;
  for (int8_t dy = -1; dy <= 1; ++dy) {
    for (int8_t dx = -1; dx <= 1; ++dx) {
      if ((dx != 0 || dy != 0) && inBounds(x + dx, y + dy) &&
          g_mines[y + dy][x + dx]) {
        ++count;
      }
    }
  }
  return count;
}

static void resetMinesweeper() {
  memset(g_mines, 0, sizeof(g_mines));
  memset(g_revealed, 0, sizeof(g_revealed));
  memset(g_flags, 0, sizeof(g_flags));
  g_cursorX = 0;
  g_cursorY = 0;
  g_started = false;
  g_gameOver = false;
  g_won = false;
}

static void placeMines(uint8_t safeX, uint8_t safeY) {
  uint8_t placed = 0;
  while (placed < MINE_COUNT) {
    const uint8_t x = random(BOARD_W);
    const uint8_t y = random(BOARD_H);
    if (g_mines[y][x] || (x == safeX && y == safeY)) {
      continue;
    }
    g_mines[y][x] = true;
    ++placed;
  }
  g_started = true;
}

static void revealCell(uint8_t x, uint8_t y) {
  if (g_flags[y][x] || g_revealed[y][x] || g_gameOver) {
    return;
  }
  if (!g_started) {
    placeMines(x, y);
  }
  g_revealed[y][x] = true;
  if (g_mines[y][x]) {
    g_gameOver = true;
    return;
  }
  if (adjacentMines(x, y) == 0) {
    for (int8_t dy = -1; dy <= 1; ++dy) {
      for (int8_t dx = -1; dx <= 1; ++dx) {
        const int8_t nextX = x + dx;
        const int8_t nextY = y + dy;
        if ((dx != 0 || dy != 0) && inBounds(nextX, nextY)) {
          revealCell(nextX, nextY);
        }
      }
    }
  }
}

static void updateWinState() {
  if (g_gameOver) {
    return;
  }
  uint8_t hiddenSafe = 0;
  for (uint8_t y = 0; y < BOARD_H; ++y) {
    for (uint8_t x = 0; x < BOARD_W; ++x) {
      if (!g_mines[y][x] && !g_revealed[y][x]) {
        ++hiddenSafe;
      }
    }
  }
  if (hiddenSafe == 0) {
    g_won = true;
    g_gameOver = true;
  }
}

static void drawMinesweeper(AppContext &context) {
  if (!context.tftReady) {
    return;
  }
  lavaClear(LAVA_BLACK);
  lavaFillRect(0, 0, LAVA_SCREEN_W, 20, LAVA_YELLOW);
  lavaDrawText(4, 6, "Minesweeper", LAVA_BLACK, LAVA_YELLOW);

  for (uint8_t y = 0; y < BOARD_H; ++y) {
    for (uint8_t x = 0; x < BOARD_W; ++x) {
      const int16_t px = BOARD_X + x * CELL_SIZE;
      const int16_t py = BOARD_Y + y * CELL_SIZE;
      const bool cursor = x == g_cursorX && y == g_cursorY;
      uint8_t bg = g_revealed[y][x] ? LAVA_GRAY : LAVA_BLUE;
      char text[2] = {' ', 0};
      uint8_t fg = LAVA_WHITE;

      if (g_gameOver && g_mines[y][x]) {
        bg = LAVA_RED;
        text[0] = '*';
      } else if (g_flags[y][x]) {
        text[0] = 'F';
        fg = LAVA_YELLOW;
      } else if (g_revealed[y][x]) {
        const uint8_t count = adjacentMines(x, y);
        if (count > 0) {
          text[0] = '0' + count;
          fg = LAVA_BLACK;
        }
      }

      lavaFillRect(px, py, CELL_SIZE - 1, CELL_SIZE - 1, bg);
      lavaDrawRect(px, py, CELL_SIZE - 1, CELL_SIZE - 1, cursor ? LAVA_YELLOW : LAVA_BLACK);
      lavaDrawText(px + 9, py + 8, text, fg, bg);
    }
  }

  if (g_gameOver) {
    lavaDrawText(84, 196, g_won ? "You win! A:Restart" : "Boom! A:Restart",
                 g_won ? LAVA_GREEN : LAVA_RED, LAVA_BLACK);
  } else {
    lavaDrawText(38, 206, "DPAD Move  A:Open  B:Flag", LAVA_GRAY, LAVA_BLACK);
  }
  lavaDrawText(92, 222, "SEL+ST Exit", LAVA_GRAY, LAVA_BLACK);
  lavaPresent();
}

static void minesweeperBegin(AppContext &context) {
  randomSeed(micros());
  resetMinesweeper();
  drawMinesweeper(context);
}

static void minesweeperTick(AppContext &context, uint32_t nowMs) {
  (void)nowMs;
  bool changed = false;

  if (g_allButtons[DPAD_UP_INDEX].pressed && g_cursorY > 0) {
    --g_cursorY;
    changed = true;
  }
  if (g_allButtons[DPAD_DOWN_INDEX].pressed && g_cursorY + 1 < BOARD_H) {
    ++g_cursorY;
    changed = true;
  }
  if (g_allButtons[DPAD_LEFT_INDEX].pressed && g_cursorX > 0) {
    --g_cursorX;
    changed = true;
  }
  if (g_allButtons[DPAD_RIGHT_INDEX].pressed && g_cursorX + 1 < BOARD_W) {
    ++g_cursorX;
    changed = true;
  }
  if (context.buttons[1].pressed && !g_gameOver && !g_revealed[g_cursorY][g_cursorX]) {
    g_flags[g_cursorY][g_cursorX] = !g_flags[g_cursorY][g_cursorX];
    changed = true;
  }
  if (context.buttons[0].pressed) {
    if (g_gameOver) {
      resetMinesweeper();
    } else {
      revealCell(g_cursorX, g_cursorY);
      updateWinState();
    }
    changed = true;
  }

  if (changed) {
    drawMinesweeper(context);
  }
}

static void minesweeperEnd(AppContext &context) {
  (void)context;
  lavaClear(LAVA_BLACK);
  lavaPresent();
}

const LauncherApp &minesweeperApp() {
  static const LauncherApp app = {"Minesweeper", minesweeperBegin, minesweeperTick,
                                  minesweeperEnd};
  return app;
}
