#include "screen_test_i18n.h"

#include "mia_host_abi.h"

static const ScreenTestText ST_EN = {
    .title = "Screen Test",
    .pattern_red = "RED",
    .pattern_green = "GREEN",
    .pattern_blue = "BLUE",
    .pattern_white = "WHITE",
    .pattern_black = "BLACK",
    .exit_hint = "A/B:Switch  SEL+ST:Exit",
};

static const ScreenTestText ST_ZH = {
    .title = "屏幕测试",
    .pattern_red = "红",
    .pattern_green = "绿",
    .pattern_blue = "蓝",
    .pattern_white = "白",
    .pattern_black = "黑",
    .exit_hint = "A/B:切换  SEL+ST:退出",
};

const ScreenTestText *screen_test_text(void) {
  return mia_host_language() == 1 ? &ST_ZH : &ST_EN;
}
