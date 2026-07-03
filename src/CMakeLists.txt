set(APP_SOURCES
  "apps/about_app.cpp"
  "apps/diagnostic_app.cpp"
  "apps/flashlight_app.cpp"
  "apps/ftp_server_app.cpp"
  "apps/screen_test_app.cpp"
  "apps/timer_app.cpp"
  "apps/wifi_files_app.cpp"
  "apps/wifi_scan_app.cpp"
)

idf_component_register(
  SRCS
    "main.cpp"
    "lcd_ili9342.cpp"
    "lava_native_display.cpp"
    "rtc_clock.cpp"
    "mia_elf_runner.cpp"
    "mia_host_abi.cpp"
    "sd_app_loader.cpp"
    "wifi_file_config.cpp"
    "wifi_file_http.cpp"
    "wifi_file_paths.cpp"
    ${APP_SOURCES}
  INCLUDE_DIRS "../include"
  REQUIRES framework-arduinoespressif32 elf_loader
)
