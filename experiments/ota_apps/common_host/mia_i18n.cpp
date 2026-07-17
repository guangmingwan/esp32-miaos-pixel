#include "mia_i18n.h"
#include <string.h>

struct Translation {
    const char *key;
    const char *value;
};

static const Translation zhTable[] = {
    {"1. Hold ST", "1. 按住 ST"},
    {"2. Press RESET", "2. 按 RESET"},
    {"3. Release RESET into", "3. 松开 RESET 进入"},
    {"A/B:Back", "A/B:返回"},
    {"A:Confirm  B/SEL:Back", "A:确认  B/SEL:返回"},
    {"A:OK", "A:确定"},
    {"A:Open UP/DN:Move LEFT/RIGHT:Tab", "A:打开 上/下:移动 左/右:标签"},
    {"About", "关于"},
    {"Apply font", "应用字体"},
    {"B/SEL:Later", "B/SEL:稍后"},
    {"Boot Loader", "启动引导"},
    {"Calculator", "计算器"},
    {"Category: %s", "分类: %s"},
    {"code", "代码"},
    {"Download and run", "下载并运行"},
    {"download mode", "下载模式"},
    {"Error: divide by 0", "错误: 除以零"},
    {"Export Failed", "导出失败"},
    {"Export OK", "导出成功"},
    {"Export OTA to SD", "导出 OTA 到 SD"},
    {"Font", "字体"},
    {"Language", "语言"},
    {"Logs", "日志"},
    {"MiaOS Launcher", "MiaOS 启动器"},
    {"Name: %s", "名称: %s"},
    {"No valid manifest in ota_1.", "ota_1 中无有效清单。"},
    {"normal boot", "正常启动"},
    {"OTA app exported to SD.", "OTA 应用已导出到 SD。"},
    {"Press A to apply and restart", "按 A 应用并重启"},
    {"Press A to export, B to cancel", "按 A 导出，B 取消"},
    {"Press any button", "按任意键"},
    {"RESET alone returns to", "单独按 RESET 返回"},
    {"RTC unavailable", "RTC 不可用"},
    {"SD App", "SD 应用"},
    {"SD card:no apps", "SD 卡无应用"},
    {"SD card:read error", "SD 卡读取错误"},
    {"SD card:ready", "SD 卡就绪"},
    {"SD card:run error", "SD 卡运行错误"},
    {"SD card:unavailable", "SD 卡不可用"},
    {"SD card:unknown", "SD 卡未知状态"},
    {"SEL+ST Exit", "SEL+ST 退出"},
    {"VCP File Transfer", "VCP文件传输"},
    {"System", "系统"},
    {"To: /MiaOS/%s/%s.app/%s.bin", "目标: /MiaOS/%s/%s.app/%s.bin"},
    {"Upload to SD", "上传到 SD"},
    {"<missing>", "<缺失>"},
};

static constexpr int zhCount = sizeof(zhTable) / sizeof(zhTable[0]);

#ifdef __cplusplus
static MiaLanguage currentLang = MiaLanguage::English;
#else
static MiaLanguage currentLang = MIA_LANG_ENGLISH;
#endif

static const char *langNames[] = {
    "English",
    "中文",
};

#ifdef __cplusplus
static constexpr int langCount = 2;
#else
static const int langCount = 2;
#endif

MiaLanguage miaLanguage(void) {
    return currentLang;
}

const char *miaLanguageName(MiaLanguage lang) {
    int idx = (int)lang;
    if (idx < 0 || idx >= langCount) {
        return "?";
    }
    return langNames[idx];
}

void miaCycleLanguage(void) {
    int next = ((int)currentLang + 1) % langCount;
    currentLang = (MiaLanguage)next;
}

const char *miaTr(const char *key) {
    if (key == NULL) {
        return NULL;
    }
#ifdef __cplusplus
    if (currentLang == MiaLanguage::English) {
#else
    if (currentLang == MIA_LANG_ENGLISH) {
#endif
        return key;
    }
    for (int i = 0; i < zhCount; ++i) {
        if (strcmp(zhTable[i].key, key) == 0) {
            return zhTable[i].value;
        }
    }
    return key;
}
