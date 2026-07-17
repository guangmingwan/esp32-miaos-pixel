#include "hello_i18n.h"

#include "mia_host_abi.h"

static const HelloText HELLO_EN = {
    .title = "Hello from OTA App",
    .greeting = "",
    .exit_hint = "SEL+ST: Exit",
};

static const HelloText HELLO_ZH = {
    .title = "OTA 应用示例",
    .greeting = "",
    .exit_hint = "SEL+ST: 退出",
};

const HelloText *hello_text(void) {
  return mia_host_language() == 1 ? &HELLO_ZH : &HELLO_EN;
}
