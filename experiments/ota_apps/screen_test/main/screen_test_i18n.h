#pragma once

typedef struct {
  const char *title;
  const char *pattern_red;
  const char *pattern_green;
  const char *pattern_blue;
  const char *pattern_white;
  const char *pattern_black;
  const char *exit_hint;
} ScreenTestText;

const ScreenTestText *screen_test_text(void);
