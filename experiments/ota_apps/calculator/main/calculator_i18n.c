#include "calculator_i18n.h"

#include "mia_host_abi.h"

static const CalculatorText CL_EN = {
    .title = "Calculator",
    .error_div0 = "Error: divide by 0",
    .a_ok = "A:OK",
    .exit_hint = "SEL+ST Exit",
};

static const CalculatorText CL_ZH = {
    .title = "计算器",
    .error_div0 = "错误：除以零",
    .a_ok = "A:确认",
    .exit_hint = "SEL+ST 退出",
};

const CalculatorText *calculator_text(void) {
  return mia_host_language() == 1 ? &CL_ZH : &CL_EN;
}
