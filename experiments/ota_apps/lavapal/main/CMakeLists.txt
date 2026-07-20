if(IDF_VERSION_MAJOR GREATER_EQUAL 5)
    set(MIA_PARTITION_COMPONENTS esp_partition spi_flash)
else()
    set(MIA_PARTITION_COMPONENTS spi_flash)
endif()

set(LAVAPAL_SDL_SOURCES
    ../audio.c
    ../battle.c
    ../ending.c
    ../fight.c
    ../font.c
    ../game.c
    ../global.c
    ../input.c
    ../itemmenu.c
    ../magicmenu.c
    ../main.c
    ../map.c
    ../midi.c
    ../midi_stubs.c
    ../midi_tsf.c
    ../palcfg.c
    ../palcommon.c
    ../paldebug.c
    ../palette.c
    ../play.c
    ../res.c
    ../resampler.c
    ../rixplay.cpp
    ../rngplay.c
    ../scene.c
    ../script.c
    ../sound.c
    ../text.c
    ../ui.c
    ../uibattle.c
    ../uigame.c
    ../unix.cpp
    ../util.c
    ../video.c
    ../yj1.c
    ../vendor/rix_adplug/emuopl.cpp
    ../vendor/rix_adplug/fmopl.c
    ../vendor/rix_adplug/fprovide.cpp
    ../vendor/rix_adplug/player.cpp
    ../vendor/rix_adplug/rix.cpp
    ../vendor/rix_binio/binfile.cpp
    ../vendor/rix_binio/binio.cpp
)

idf_component_register(
    SRCS
        "app_main.c"
        "sdl_wrapper.c"
        "lavapal_stubs.c"
        ${LAVAPAL_SDL_SOURCES}
        "../../common_host/host_platform.cpp"
        "../../common_host/display_host.cpp"
    INCLUDE_DIRS
        "."
        ".."
        "../vendor/rix_adplug"
        "../vendor/rix_binio"
        "../../common_host"
        "../../../../include"
        "../../../shared_libraries/mia_sdl/include"
    REQUIRES
        app_update
        driver
        elf_loader
        esp_event
        esp_http_server
        esp_netif
        ${MIA_PARTITION_COMPONENTS}
        esp_timer
        esp_wifi
        fatfs
        nvs_flash
        sdmmc
)

# SDLPAL's main() must be callable as lavapal_main_impl from app_main.c.
set_source_files_properties("../main.c" PROPERTIES
    COMPILE_DEFINITIONS "main=lavapal_main_impl")

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    PAL_HAS_GLSL=0
    PAL_HAS_NATIVEMIDI=0
    PAL_HAS_JOYSTICKS=0
    PAL_HAS_SDLCD=0
    PAL_HAS_OGG=0
    PAL_HAS_OPUS=0
    PAL_HAS_MP3=0
    PAL_HAS_TIMIDITY=0
    PAL_HAS_LIBMP3LAME=0
    PAL_HAS_TSF=1
    SDL_CDROM_DISABLED=1
    PAL_NO_LAUNCH_UI=1
    PAL_HAS_PLATFORM_SPECIFIC_UTILS=1
    MIA_SD_MAX_FILES=16
    MIA_HC165_ACTIVE_HIGH=1
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -w
    -Wno-error
    -Wno-error=address
    -Wno-error=array-bounds
    -Wno-error=format
    -Wno-error=format-overflow
    -Wno-error=incompatible-pointer-types
    -Wno-error=int-conversion
    -Wno-error=pointer-sign
    -Wno-error=return-type
    -Wno-error=maybe-uninitialized
    -Wno-error=missing-braces
    -Wno-error=unused-function
    -Wno-error=unused-variable
    -Wno-error=unused-but-set-variable
    -Wno-error=deprecated-declarations
    -Wno-error=sign-compare
    -Wno-stringop-truncation
    -Wno-stringop-overflow
    -Wno-format-truncation
    -Wno-format-overflow
    -Wno-maybe-uninitialized
    -fno-strict-aliasing
)
