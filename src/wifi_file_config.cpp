#include "wifi_file_config.h"

#include <SD.h>

static constexpr const char *CONFIG_PATH = "/wifi.txt";
static constexpr const char *DEFAULT_AP_SSID = "MiaOS";

static void applyConfigLine(WifiFileConfig &config, const String &line) {
  const int separator = line.indexOf('=');
  if (separator <= 0) {
    return;
  }
  const String key = line.substring(0, separator);
  const String value = line.substring(separator + 1);
  if (key == "ssid") {
    config.stationSsid = value;
    return;
  }
  if (key == "password") {
    config.stationPassword = value;
    return;
  }
  if (key == "ap_ssid") {
    config.accessPointSsid = value;
    return;
  }
  if (key == "ap_password") {
    config.accessPointPassword = value;
  }
}

WifiFileConfig loadWifiFileConfig() {
  WifiFileConfig config = {"", "", DEFAULT_AP_SSID, ""};
  File file = SD.open(CONFIG_PATH, FILE_READ);
  if (!file) {
    return config;
  }
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0 || line.startsWith("#")) {
      continue;
    }
    applyConfigLine(config, line);
  }
  file.close();
  if (config.accessPointSsid.length() == 0) {
    config.accessPointSsid = DEFAULT_AP_SSID;
  }
  return config;
}
