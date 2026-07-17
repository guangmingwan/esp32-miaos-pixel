idf_component_register(
    SRCS
        "app_main.c"
        "minesweeper_main.c"
        "minesweeper_i18n.c"
        "../../common_host/host_platform.cpp"
        "../../common_host/display_host.cpp"
        "../../common_host/mia_text_runtime.cpp"
    PRIV_REQUIRES
        elf_loader
    INCLUDE_DIRS
        "."
        "../../common_host"
        "../../../../include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE MIA_DISPLAY_DROID_GBK_SHARED=1)
