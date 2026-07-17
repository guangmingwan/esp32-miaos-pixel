#pragma once

typedef struct {
  const char *title;
  const char *you_win;
  const char *boom;
  const char *controls;
  const char *exit_hint;
} MinesweeperText;

const MinesweeperText *minesweeper_text(void);
