#include "timer_i18n.h"

#include "mia_host_abi.h"

static const TimerText TM_EN = {
    .title = "Timer",
    .running = "RUNNING",
    .paused = "PAUSED",
    .start_pause = "A:Start/Pause",
    .reset_exit = "LT+RT:Reset  SEL+ST:Exit",
};

static const TimerText TM_ZH = {
    .title = "计时器",
    .running = "运行中",
    .paused = "已暂停",
    .start_pause = "A:开始/暂停",
    .reset_exit = "LT+RT:复位  SEL+ST:退出",
};

const TimerText *timer_text(void) {
  return mia_host_language() == 1 ? &TM_ZH : &TM_EN;
}
