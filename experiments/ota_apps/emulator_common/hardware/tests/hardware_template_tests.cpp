#include "test_support.h"

#include <string>

int main() {
    const std::string wrapper = read_text_file("experiments/ota_apps/emulator_common/template/main/app_main.c.in");
    const std::string main_cmake = read_text_file("experiments/ota_apps/emulator_common/template/main/CMakeLists.txt.in");
    const std::string root_cmake = read_text_file("experiments/ota_apps/emulator_common/template/CMakeLists.txt.in");
    require_true(wrapper.find("set_entry(&entries[0], 1, ESP_OTA_IMG_VALID)") != std::string::npos, "wrapper must restore ota_0 seq=1 valid entry");
    require_true(wrapper.find("set_entry(&entries[1], 3, ESP_OTA_IMG_VALID)") != std::string::npos, "wrapper must restore ota_0 seq=3 valid mirror entry");
    require_true(wrapper.find("esp_partition_erase_range") != std::string::npos && wrapper.find("esp_partition_write(otap, 4096") != std::string::npos, "wrapper must write both otadata sectors");
    require_true(wrapper.find("MiaLauncherSwitchResult") != std::string::npos, "wrapper must return typed launcher switch success or failure");
    require_true(wrapper.find("MIA_LAUNCHER_SWITCH_OK") != std::string::npos, "wrapper must expose typed success result");
    require_true(wrapper.find("switch_to_launcher();\n    esp_restart();") == std::string::npos, "wrapper must not restart unconditionally after switch attempt");
    require_true(wrapper.find("if (switch_result.code == MIA_LAUNCHER_SWITCH_OK)") != std::string::npos, "wrapper must gate restart on successful otadata writes");
    require_true(wrapper.find("esp_restart()") != std::string::npos, "wrapper must restart after successful clean app exit");
    require_true(wrapper.find("ESP_LOGE") != std::string::npos && wrapper.find("switch_result.stage") != std::string::npos, "wrapper must log actionable switch failures");
    require_true(wrapper.find("return esp_partition_write(otap, 4096") == std::string::npos, "wrapper must not return second write without typing its failure");
    require_true(wrapper.find("err = esp_partition_write(otap, 4096") != std::string::npos && wrapper.find("MIA_LAUNCHER_SWITCH_ERR_WRITE_MIRROR") != std::string::npos, "wrapper must check and type second otadata write failure");
    require_true(wrapper.find("mia_host_abi_version") == std::string::npos, "wrapper must not change host ABI");
    require_true(wrapper.find("partitions_dual") == std::string::npos, "wrapper must not encode partition table paths");
    require_true(main_cmake.find("../../common_host/host_platform.cpp") != std::string::npos, "template should use shared host platform once");
    require_true(main_cmake.find("../../common_host/display_host.cpp") != std::string::npos, "template should use shared display host once");
    require_true(root_cmake.find("include($ENV{IDF_PATH}/tools/cmake/project.cmake)") != std::string::npos, "root template should remain a standard ESP-IDF app project");
}
