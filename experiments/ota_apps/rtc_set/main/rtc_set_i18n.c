#include "rtc_set_i18n.h"

#include "mia_host_abi.h"

static const RtcSetText RTC_EN = {
    .title = "RTC Set",
    .label_year = "Year",
    .label_month = "Month",
    .label_day = "Day",
    .label_hour = "Hour",
    .label_minute = "Minute",
    .label_second = "Second",
    .status_edited = "Edited",
    .status_read_ok = "RTC read OK",
    .status_read_fail = "RTC read FAIL",
    .status_write_ok = "RTC write OK",
    .status_write_fail = "RTC write FAIL",
    .controls_field = "UP/DN field  LT/RT value",
    .controls_save = "A:Save  B:Read  SEL+ST:Exit",
};

static const RtcSetText RTC_ZH = {
    .title = "时钟设置",
    .label_year = "年",
    .label_month = "月",
    .label_day = "日",
    .label_hour = "时",
    .label_minute = "分",
    .label_second = "秒",
    .status_edited = "已修改",
    .status_read_ok = "读取成功",
    .status_read_fail = "读取失败",
    .status_write_ok = "写入成功",
    .status_write_fail = "写入失败",
    .controls_field = "上/下:选择 左/右:调整",
    .controls_save = "A:保存 B:读取 SEL+ST:退出",
};

const RtcSetText *rtc_set_text(void) {
  return mia_host_language() == 1 ? &RTC_ZH : &RTC_EN;
}
