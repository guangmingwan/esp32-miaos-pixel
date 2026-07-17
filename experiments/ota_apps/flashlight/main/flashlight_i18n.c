#include "flashlight_i18n.h"

#include "mia_host_abi.h"

static const FlashlightText FL_EN = {
    .title = "Flashlight",
    .light_on = "LIGHT ON",
    .light_off = "LIGHT OFF",
    .controls = "A:Toggle  SEL+ST:Exit",
};

static const FlashlightText FL_ZH = {
    .title = "手电筒",
    .light_on = "灯光 开",
    .light_off = "灯光 关",
    .controls = "A:切换  SEL+ST:退出",
};

const FlashlightText *flashlight_text(void) {
  return mia_host_language() == 1 ? &FL_ZH : &FL_EN;
}
