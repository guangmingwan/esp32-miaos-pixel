idf_component_register(
    SRCS
        "app_main.c"
        "music_i18n.c"
        "music_main.c"
        "music_player.c"
        "third_party/stb_vorbis_impl.c"
        "../../common_host/host_platform.cpp"
        "../../common_host/display_host.cpp"
        "../../common_host/mia_text_runtime.cpp"
    PRIV_REQUIRES
        elf_loader
    INCLUDE_DIRS
        "."
        "third_party"
        "../../common_host"
        "../../../../include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE MIA_DISPLAY_DROID_GBK_SHARED=1)

# MIA_MUSIC_AUTOPLAY_FIRST removed: user should browse and select files manually
