idf_component_register(
    SRCS
        "app_main.c"
        "music_main.c"
        "music_player.c"
        "host_platform.cpp"
        "display_host.cpp"
    INCLUDE_DIRS
        "."
        "third_party"
        "../../../../include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE MIA_MUSIC_AUTOPLAY_FIRST=1)
