if(IDF_VERSION_MAJOR GREATER_EQUAL 5)
    set(MIA_PARTITION_COMPONENTS esp_partition spi_flash)
else()
    set(MIA_PARTITION_COMPONENTS spi_flash)
endif()

idf_component_register(
    SRCS
        "app_main.c"
        "../main_lava_native.c"
        "../lava_mem.c"
        "../../common_host/lava_esp32_backend.c"
        "../../common_host/host_platform.cpp"
        "../../common_host/display_host.cpp"
    INCLUDE_DIRS
        "."
        ".."
        "../../common_host"
        "../../../../include"
    REQUIRES
        app_update
        driver
        esp_event
        esp_http_server
        esp_netif
        ${MIA_PARTITION_COMPONENTS}
        esp_timer
        esp_wifi
        fatfs
        lava
        nvs_flash
        sdmmc
    LDFRAGMENTS
        "linker.lf"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LAVA_ESP32=1
    LAVA_DISPLAY_DUAL_CORE=1
    MIA_HC165_ACTIVE_HIGH=1
    MIA_DISPLAY_ASYNC_REGION=1
    MIA_DISPLAY_NO_DELAY_YIELD=1
)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=address
    -Wno-error=array-bounds
    -Wno-error=format
    -Wno-error=format-overflow
    -Wno-error=incompatible-pointer-types
    -Wno-error=int-conversion
    -Wno-error=pointer-sign
    -Wno-error=return-type
    -Wno-error=maybe-uninitialized
)
