#include "minesweeper_i18n.h"

#include "mia_host_abi.h"

static const MinesweeperText MW_EN = {
    .title = "Minesweeper",
    .you_win = "You win! A:Restart",
    .boom = "Boom! A:Restart",
    .controls = "DPAD Move  A:Open  B:Flag",
    .exit_hint = "SEL+ST Exit",
};

static const MinesweeperText MW_ZH = {
    .title = "扫雷",
    .you_win = "胜利！ A:重来",
    .boom = "爆炸！ A:重来",
    .controls = "方向键移动 A:打开 B:标记",
    .exit_hint = "SEL+ST 退出",
};

const MinesweeperText *minesweeper_text(void) {
  return mia_host_language() == 1 ? &MW_ZH : &MW_EN;
}
