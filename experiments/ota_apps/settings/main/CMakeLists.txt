idf_component_register(
    SRCS
        "app_main.c"
        "settings_main.c"
        "settings_i18n.c"
        "../../common_host/host_platform.cpp"
        "../../common_host/display_host.cpp"
        "../../common_host/lava_font_renderer.cpp"
        "../../common_host/mia_text_runtime.cpp"
        "../../../../src/fonts/basic8x8.c"
        "../../../../src/fonts/dejavu_12.c"
        "../../../../src/fonts/dejavu_15.c"
        "../../../../src/fonts/vera_bold_11.c"
        "../../../../src/fonts/vera_bold_14.c"
    PRIV_REQUIRES
        elf_loader
    INCLUDE_DIRS
        "."
        "../../common_host"
        "../../../../include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    MIA_DISPLAY_DROID_GBK_SHARED=1
    MIA_DISPLAY_MULTI_FONT=1
    MIA_HC165_ACTIVE_HIGH=1
)
