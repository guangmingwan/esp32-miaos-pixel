if(IDF_VERSION_MAJOR GREATER_EQUAL 5)
    set(MIA_PLATFORM_REQUIRES app_update driver esp_driver_gpio esp_driver_i2c
        esp_driver_i2s esp_driver_sdspi esp_driver_spi esp_event esp_netif
        esp_partition esp_rom esp_timer esp_wifi esp_http_server fatfs nvs_flash sdmmc)
else()
    set(MIA_PLATFORM_REQUIRES app_update spi_flash driver esp_event esp_netif esp_rom
        esp_timer esp_wifi esp_http_server fatfs nvs_flash sdmmc)
endif()

idf_component_register(
    SRCS
        "app_main.c"
        "wifi_files_main.c"
        "wifi_files_i18n.c"
        "../../common_host/host_platform.cpp"
        "../../common_host/display_host.cpp"
        "../../common_host/mia_text_runtime.cpp"
    PRIV_REQUIRES ${MIA_PLATFORM_REQUIRES} elf_loader
    INCLUDE_DIRS
        "."
        "../../common_host"
        "../../../../include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE MIA_DISPLAY_DROID_GBK_SHARED=1)
