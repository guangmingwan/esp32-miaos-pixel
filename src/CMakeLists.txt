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
    "lava_text.cpp"
    "gbk_unicode_map.cpp"
    "droid_gbk_index.cpp"
    "fonts/basic8x8.c"
    "fonts/dejavu_12.c"
    "fonts/dejavu_15.c"
    "fonts/droid_gbk_12.c"
    "fonts/vera_bold_11.c"
    "fonts/vera_bold_14.c"
    "../experiments/ota_apps/common_host/mia_i18n.cpp"
    "rtc_clock.cpp"
    "audio_host.cpp"
    "ota_app_flash.cpp"
    "launcher_log.cpp"
    "serial_file_service.cpp"
    "sd_app_loader.cpp"
    "wifi_file_config.cpp"
    "wifi_file_http.cpp"
    "wifi_file_paths.cpp"
    ${APP_SOURCES}
  INCLUDE_DIRS "../include"
  REQUIRES framework-arduinoespressif32
)
