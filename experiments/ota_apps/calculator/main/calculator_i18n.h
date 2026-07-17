#pragma once

typedef struct {
  const char *title;
  const char *error_div0;
  const char *a_ok;
  const char *exit_hint;
} CalculatorText;

const CalculatorText *calculator_text(void);
