idf_component_register(
    SRCS
        "app_main.c"
        "music_i18n.c"
        "music_main.c"
        "music_player.c"
        "third_party/stb_vorbis_impl.c"
        "../../common_host/host_platform.cpp"
        "../../common_host/display_host.cpp"
        "../../common_host/droid_gbk_renderer.cpp"
        "../../../../src/fonts/droid_gbk_12.c"
        "../../../../src/gbk_unicode_map.cpp"
        "../../../../src/droid_gbk_index.cpp"
    INCLUDE_DIRS
        "."
        "third_party"
        "../../common_host"
        "../../../../include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE MIA_DISPLAY_DROID_GBK=1)

# MIA_MUSIC_AUTOPLAY_FIRST removed: user should browse and select files manually
