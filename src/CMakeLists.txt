set(APP_SOURCES
  "apps/serial_transfer_app.cpp"
  "apps/log_viewer_app.cpp"
  "apps/about_app.cpp"
)

idf_component_register(
  SRCS
    "main.cpp"
    "lcd_ili9342.cpp"
    "lava_native_display.cpp"
    "rtc_clock.cpp"
    "mia_elf_runner.cpp"
    "mia_host_abi.cpp"
    "mia_host_abi_system.cpp"
    "mia_host_abi_network.cpp"
    "launcher_log.cpp"
    "serial_file_service.cpp"
    "sd_app_loader.cpp"
    "wifi_file_config.cpp"
    "wifi_file_http.cpp"
    "wifi_file_paths.cpp"
    ${APP_SOURCES}
  INCLUDE_DIRS "../include"
  REQUIRES framework-arduinoespressif32 elf_loader
)
