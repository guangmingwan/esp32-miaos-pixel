file(GLOB APP_SOURCES "apps/*.cpp")

idf_component_register(
  SRCS
    "main.cpp"
    "lcd_ili9342.cpp"
    "lava_native_display.cpp"
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
