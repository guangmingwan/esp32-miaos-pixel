#pragma once

#include <Arduino.h>
#include <stdint.h>

enum class MiaElfRunStatus : uint8_t {
  Ok,
  SdUnavailable,
  ReadError,
  RunError,
};

struct MiaElfRunResult {
  MiaElfRunStatus status;
  int errorCode;
};

MiaElfRunResult miaRunElfApp(const char *path, bool sdReady);
const char *miaElfRunStatusText(MiaElfRunStatus status);
