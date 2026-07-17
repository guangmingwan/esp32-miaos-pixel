#pragma once

typedef struct {
  const char *title;
  const char *greeting;
  const char *exit_hint;
} HelloText;

const HelloText *hello_text(void);
