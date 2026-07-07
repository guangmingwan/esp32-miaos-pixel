idf_component_register(
    SRCS
        "app_main.c"
        "music_main.c"
        "music_player.c"
        "../../common_host/host_platform.cpp"
        "../../common_host/display_host.cpp"
    INCLUDE_DIRS
        "."
        "third_party"
        "../../common_host"
        "../../../../include"
)

# MIA_MUSIC_AUTOPLAY_FIRST removed: user should browse and select files manually
