idf_component_register(
    SRCS
        "app_main.c"
        "sd_browser_main.c"
        "../../common_host/host_platform.cpp"
        "../../common_host/display_host.cpp"
    INCLUDE_DIRS
        "."
        "../../common_host"
        "../../../../include"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format-truncation)
