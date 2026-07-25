#pragma once

typedef struct {
  const char *title;
  const char *tft_ok;
  const char *sd_ok;
  const char *ota_ok;
  const char *rtc_ok;
  const char *btn;
  const char *exit_hint;
  const char *boot_note;
} DiagnosticText;

const DiagnosticText *diagnostic_text(void);
