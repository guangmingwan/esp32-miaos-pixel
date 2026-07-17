#pragma once

typedef struct {
  const char *title;
  const char *running;
  const char *paused;
  const char *start_pause;
  const char *reset_exit;
} TimerText;

const TimerText *timer_text(void);
