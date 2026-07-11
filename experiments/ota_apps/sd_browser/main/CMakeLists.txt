idf_component_register(
    SRCS
        "app_main.c"
        "sd_browser_main.c"
        "../../common_host/host_platform.cpp"
        "../../common_host/display_host.cpp"
        "../../common_host/droid_gbk_renderer.cpp"
        "../../../../src/fonts/droid_gbk_12.c"
        "../../../../src/gbk_unicode_map.cpp"
        "../../../../src/droid_gbk_index.cpp"
    INCLUDE_DIRS
        "."
        "../../common_host"
        "../../../../include"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format-truncation)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    MIA_DISPLAY_DROID_GBK=1
    MIA_SD_BROWSER_FEATURES=1
    MIA_HOST_DIRENT_NAME_SIZE=256
)
