#pragma once

typedef struct {
  const char *title;
  const char *light_on;
  const char *light_off;
  const char *controls;
} FlashlightText;

const FlashlightText *flashlight_text(void);
