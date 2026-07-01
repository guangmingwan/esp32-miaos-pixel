#pragma once

#include <Arduino.h>

struct WifiFileConfig {
  String stationSsid;
  String stationPassword;
  String accessPointSsid;
  String accessPointPassword;
};

WifiFileConfig loadWifiFileConfig();
