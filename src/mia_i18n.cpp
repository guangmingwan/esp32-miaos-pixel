#include "mia_i18n.h"

#include <cstring>

#include <Arduino.h>
#include <esp_err.h>
#include <nvs_flash.h>

#include "launcher_log.h"

struct MiaTranslation {
  const char *english;
  const char *chinese;
};

struct MiaAppNameTranslation {
  const char *name;
  const char *english;
  const char *chinese;
};

// Use a dedicated NVS namespace via the native ESP-IDF nvs_flash API to avoid
// any Arduino Preferences wrapper bugs on ESP32-S3.
static constexpr const char *MIA_I18N_NAMESPACE = "mia-i18n";
static constexpr const char *MIA_I18N_LANGUAGE_KEY = "lang";

static const MiaTranslation TRANSLATIONS[] = {
    {"MiaOS Launcher", "MiaOS 启动器"},
    {"System", "系统"},
    {"Language", "语言"},
    {"Font", "字体"},
    {"English", "English"},
    {"Chinese", "中文"},
    {"USB Disk", "USB 磁盘"},
    {"Boot Loader", "引导模式"},
    {"Export OTA to SD", "导出 OTA 到 SD"},
    {"SD card:ready", "SD 卡:就绪"},
    {"SD card:unavailable", "SD 卡:不可用"},
    {"SD card:no apps", "SD 卡:无应用"},
    {"SD card:read error", "SD 卡:读取错误"},
    {"SD card:run error", "SD 卡:运行错误"},
    {"SD card:unknown", "SD 卡:未知"},
    {"A:Open UP/DN:Move LEFT/RIGHT:Tab", "A:打开 上/下:移动 左/右:分页"},
    {"1. Hold ST", "1. 按住 ST"},
    {"2. Press RESET", "2. 按 RESET"},
    {"3. Release RESET into", "3. 松开 RESET 进入"},
    {"download mode", "下载模式"},
    {"RESET alone returns to", "单独 RESET 返回"},
    {"normal boot", "正常启动"},
    {"A/B:Back", "A/B:返回"},
    {"SD App", "SD 应用"},
    {"Download and run", "下载并运行"},
    {"Loading", "加载中"},
    {"Upload to SD", "上传到 SD"},
    {"A:Confirm  B/SEL:Back", "A:确认  B/SEL:返回"},
    {"Category: %s", "分类: %s"},
    {"Name: %s", "名称: %s"},
    {"To: /MiaOS/%s/%s.app/%s.bin", "到: /MiaOS/%s/%s.app/%s.bin"},
    {"Press A to export, B to cancel", "按 A 导出，按 B 取消"},
    {"Export OK", "导出成功"},
    {"Export Failed", "导出失败"},
    {"OTA app exported to SD.", "OTA 应用已导出到 SD。"},
    {"No valid manifest in ota_1.", "ota_1 中无有效清单。"},
    {"OTA App Sync", "OTA 应用同步"},
    {"OTA update found", "发现 OTA 更新"},
    {"Writing OTA app", "正在写入 OTA 应用"},
    {"Verifying update", "正在校验更新"},
    {"Update complete", "更新完成"},
    {"OTA update failed", "OTA 更新失败"},
    {"App", "应用"},
    {"Press any button", "按任意键"},
    {"VCP File Transfer", "VCP文件传输"},
    {"USB VCP file service", "USB VCP文件服务"},
    {"SD unavailable", "SD 卡不可用"},
    {"Host: tools/serial_sd_client.py", "主机: tools/serial_sd_client.py"},
    {"%lu / %lu bytes", "%lu / %lu 字节"},
    {"SEL+ST:Exit", "SEL+ST:退出"},
    {"Idle", "空闲"},
    {"Waiting for host", "等待主机"},
    {"Receiving file", "正在接收文件"},
    {"Sending file", "正在发送文件"},
    {"Listed directory", "已列出目录"},
    {"Created directory", "已创建目录"},
    {"Deleted path", "已删除路径"},
    {"Renamed path", "已重命名路径"},
    {"Pinged", "已响应 Ping"},
    {"Help shown", "已显示帮助"},
    {"Info shown", "已显示信息"},
    {"Rejected long line", "已拒绝超长命令"},
    {"Upload timeout", "上传超时"},
    {"Write failed", "写入失败"},
    {"Verify failed", "校验失败"},
    {"Upload complete", "上传完成"},
    {"Download timeout", "下载超时"},
    {"Read failed", "读取失败"},
    {"Download complete", "下载完成"},
    {"Stopped", "已停止"},
    {"Logs", "日志"},
    {"PSRAM Test", "PSRAM 测试"},
    {"About", "关于"},
    {"Apply font", "应用字体"},
    {"Press A to apply and restart", "按 A 应用并重启"},
    {"B/SEL:Later", "B/SEL:稍后"},
    {"Games", "游戏"},
    {"Utils", "工具"},
    {"Settings", "设置"},
    {"Emulators", "模拟器"},
    {"Media", "媒体"},
    {"Application", "应用"},
    {"calculator", "计算器"},
    {"lava_cch", "中国象棋[lava兼容版]"},
    {"lava_pal", "仙剑奇侠传[lava兼容版]"},
    {"minesweeper", "扫雷"},
    {"settings", "系统设置"},
    {"rtc_set", "时间设置"},
    {"sd_browser", "SD 浏览器"},
    {"diagnostic", "诊断"},
    {"screen_test", "屏幕测试"},
    {"flashlight", "手电筒"},
    {"timer", "计时器"},
    {"wifi_scan", "WiFi 扫描"},
    {"wifi_files", "WiFi 文件"},
    {"ftp_server", "FTP 服务器"},
    {"music", "音乐"},
    {"usb_disk", "USB 磁盘"},
    {"usb_wifi", "USB 无线网卡"},
    {"hello", "示例"},
    {"psram_test", "PSRAM 测试"},
    {"Sun", "周日"},
    {"Mon", "周一"},
    {"Tue", "周二"},
    {"Wed", "周三"},
    {"Thu", "周四"},
    {"Fri", "周五"},
    {"Sat", "周六"},
    {"RTC unavailable", "RTC 不可用"},
    {"OK", "正常"},
    {"card unavailable", "存储卡不可用"},
    {"none", "无"},
    {"read error", "读取错误"},
    {"run error", "运行错误"},
    {"unknown", "未知"},
    {"code", "代码"},
    {"<missing>", "<缺失>"},
    /* About app */
    {"Chip", "芯片"},
    {"WiFi %s", "WiFi %s"},
    {"BT %s BLE %s", "蓝牙 %s BLE %s"},
    {"Memory", "内存"},
    {"SRAM total %luK", "SRAM 总计 %luK"},
    {"SRAM free  %luK", "SRAM 空闲 %luK"},
    {"SRAM min   %luK", "SRAM 最低 %luK"},
    {"Max block  %luK", "最大块    %luK"},
    {"SRAM is volatile RAM", "SRAM 为易失性内存"},
    {"PSRAM config disabled", "PSRAM 配置未启用"},
    {"Build has no external", "编译时未启用"},
    {"RAM support enabled", "外部内存支持"},
    {"PSRAM init failed", "PSRAM 初始化失败"},
    {"Configured in build", "编译时已配置"},
    {"but absent at runtime", "但运行时未检测到"},
    {"Configured YES", "已配置"},
    {"External volatile RAM", "外部易失性内存"},
    {"Flash", "闪存"},
    {"Sketch info unavailable", "程序信息不可用"},
    {"OTA space unavailable", "OTA 空间不可用"},
    {"Non-volatile storage", "非易失性存储"},
    {"System", "系统"},
    {"Author wanguangmign", "作者 wanguangmign"},
    {"Contributor WaitForWind", "贡献者 WaitForWind"},
    {"Audio", "音频"},
    {"I2S amplifier", "I2S 功放"},
    {"Single speaker output", "单扬声器输出"},
    {"CTRL pin is active high", "CTRL 引脚高电平有效"},
    {"UP/DN page  SEL+ST Exit", "上/下翻页 SEL+ST:退出"},
    /* Log viewer app */
    {"No launcher log on SD", "SD 上无启动器日志"},
    {"No log lines", "无日志行"},
    {"A:Reload UP/DN:Scroll SEL+ST:Exit", "A:刷新 上/下:滚动 SEL+ST:退出"},
};

static const MiaAppNameTranslation EMULATOR_NAMES[] = {
    {"coleco", "ColecoVision", "ColecoVision 游戏机"},
    {"gb", "Game Boy", "任天堂 Game Boy"},
    {"gba", "Game Boy Advance", "任天堂 Game Boy Advance"},
    {"gbc", "Game Boy Color", "任天堂 Game Boy Color"},
    {"gg", "Game Gear", "世嘉 Game Gear"},
    {"gw", "Game & Watch", "任天堂 Game & Watch"},
    {"lynx", "Atari Lynx", "雅达利 Lynx"},
    {"megadrive", "Mega Drive", "世嘉 Mega Drive"},
    {"msx", "MSX", "MSX 电脑"},
    {"nes", "Famicom / NES", "任天堂红白机"},
    {"pce", "PC Engine", "NEC PC Engine"},
    {"sms", "Master System", "世嘉 Master System"},
    {"snes", "Super Famicom / SNES", "任天堂超级任天堂"},
};

static MiaLanguage g_language = MiaLanguage::English;
static bool g_loaded = false;
static bool g_skipPersisted = false;

static const char *languageName(MiaLanguage language) {
  return language == MiaLanguage::Chinese ? "Chinese" : "English";
}

static const char *nvsErrName(esp_err_t err) {
  return esp_err_to_name(err);
}

static esp_err_t openLanguageStore(nvs_open_mode_t mode, nvs_handle_t *handle) {
  esp_err_t err = nvs_open(MIA_I18N_NAMESPACE, mode, handle);
  if (err != ESP_ERR_NVS_NOT_INITIALIZED) {
    return err;
  }

  const esp_err_t init = nvs_flash_init();
  launcherTracef("[i18n] nvs_flash_init retry: %s", nvsErrName(init));
  if (init != ESP_OK) {
    return init;
  }
  return nvs_open(MIA_I18N_NAMESPACE, mode, handle);
}

static MiaLanguage languageFromRaw(uint8_t value) {
  return value == static_cast<uint8_t>(MiaLanguage::Chinese) ? MiaLanguage::Chinese
                                                            : MiaLanguage::English;
}

static bool readStoredLanguage(MiaLanguage *language, uint8_t *raw, esp_err_t *readErr) {
  nvs_handle_t handle;
  esp_err_t err = openLanguageStore(NVS_READONLY, &handle);
  if (err != ESP_OK) {
    if (readErr != nullptr) {
      *readErr = err;
    }
    return false;
  }

  uint8_t value = 0;
  err = nvs_get_u8(handle, MIA_I18N_LANGUAGE_KEY, &value);
  nvs_close(handle);
  if (readErr != nullptr) {
    *readErr = err;
  }
  if (err != ESP_OK) {
    return false;
  }
  if (raw != nullptr) {
    *raw = value;
  }
  if (language != nullptr) {
    *language = languageFromRaw(value);
  }
  return true;
}

static void loadLanguage() {
  if (g_loaded) {
    return;
  }
  if (g_skipPersisted) {
    launcherTracef("[i18n] safe mode, default=%s", languageName(g_language));
    g_loaded = true;
    return;
  }
  MiaLanguage storedLanguage = MiaLanguage::English;
  uint8_t raw = 0;
  esp_err_t err = ESP_OK;
  if (readStoredLanguage(&storedLanguage, &raw, &err)) {
    g_language = storedLanguage;
    launcherTracef("[i18n] load raw=%u -> %s", raw, languageName(g_language));
  } else {
    launcherTracef("[i18n] load default=%s err=%s", languageName(g_language), nvsErrName(err));
  }
  g_loaded = true;
}

MiaLanguage miaLanguage() {
  loadLanguage();
  return g_language;
}

void miaSetLanguage(MiaLanguage language) {
  loadLanguage();
  g_language = language;
  nvs_handle_t handle;
  esp_err_t err = openLanguageStore(NVS_READWRITE, &handle);
  if (err == ESP_OK) {
    const uint8_t val = static_cast<uint8_t>(language);
    const esp_err_t setErr = nvs_set_u8(handle, MIA_I18N_LANGUAGE_KEY, val);
    const esp_err_t commitErr = setErr == ESP_OK ? nvs_commit(handle) : setErr;
    nvs_close(handle);
    MiaLanguage verifiedLanguage = MiaLanguage::English;
    uint8_t verifiedRaw = 0;
    esp_err_t verifyErr = commitErr;
    bool verified = false;
    if (commitErr == ESP_OK) {
      verified = readStoredLanguage(&verifiedLanguage, &verifiedRaw, &verifyErr) &&
                 verifiedLanguage == language;
    }
    launcherTracef("[i18n] save %s raw=%u set=%s commit=%s verify=%s%s%u",
                   languageName(language), val, nvsErrName(setErr), nvsErrName(commitErr),
                   verified ? "ok raw=" : nvsErrName(verifyErr), verified ? "" : " raw=",
                   verifiedRaw);
  } else {
    launcherTracef("[i18n] save open failed: %s", nvsErrName(err));
  }
}

void miaCycleLanguage() {
  miaSetLanguage(miaLanguage() == MiaLanguage::English ? MiaLanguage::Chinese
                                                       : MiaLanguage::English);
}

const char *miaLanguageName(MiaLanguage language) {
  return language == MiaLanguage::Chinese ? "Chinese" : "English";
}

const char *miaTr(const char *english) {
  if (english == nullptr || miaLanguage() == MiaLanguage::English) {
    return english;
  }
  for (const MiaTranslation &entry : TRANSLATIONS) {
    if (strcmp(entry.english, english) == 0) {
      return entry.chinese;
    }
  }
  return english;
}

const char *miaAppDisplayName(const char *category, const char *name) {
  if (category == nullptr || name == nullptr || strcmp(category, "Emulators") != 0) {
    return miaTr(name);
  }
  for (const MiaAppNameTranslation &entry : EMULATOR_NAMES) {
    if (strcmp(entry.name, name) == 0) {
      return miaLanguage() == MiaLanguage::Chinese ? entry.chinese : entry.english;
    }
  }
  return miaTr(name);
}

void miaI18nSkipPersisted(void) {
  g_skipPersisted = true;
}
