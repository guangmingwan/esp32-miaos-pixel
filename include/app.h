#pragma once

#include <Arduino.h>

struct ButtonState {
  bool down;
  bool pressed;
  bool released;
};

struct AppContext {
  ButtonState buttons[6];
  bool tftReady;
  bool sdReady;
};

using AppCallback = void (*)(AppContext &context);
using AppTickCallback = void (*)(AppContext &context, uint32_t nowMs);

struct LauncherApp {
  const char *name;
  AppCallback begin;
  AppTickCallback tick;
  AppCallback end;
};
