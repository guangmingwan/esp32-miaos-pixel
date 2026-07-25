file(GLOB MIA_GBSP_C_SOURCES "../../emulator_common/vendor/gbsp/components/gbsp-libretro/*.c")

if(IDF_VERSION_MAJOR GREATER_EQUAL 5)
    set(MIA_PLATFORM_REQUIRES app_update esp_partition mbedtls driver esp_driver_gpio
        esp_driver_spi esp_driver_i2s esp_driver_i2c fatfs sdmmc nvs_flash esp_timer
        esp_event esp_netif esp_wifi esp_http_server esp_adc)
else()
    set(MIA_PLATFORM_REQUIRES app_update spi_flash mbedtls driver fatfs sdmmc nvs_flash
        esp_timer esp_event esp_netif esp_wifi esp_http_server)
endif()

idf_component_register(
    SRCS
        "app_main.c"
        "gba_policy.c"
        "mia_emulator_gbsp.c"
        "gbsp_cpp_bridge.cpp"
        "../../emulator_common/vendor/gbsp/components/gbsp-libretro/cpu.cpp"
        "../../emulator_common/vendor/gbsp/components/gbsp-libretro/video.cpp"
        "../../emulator_common/app/mia_emulator_app.c"
        "../../emulator_common/app/src/mia_app_audio.c"
        "../../emulator_common/app/src/mia_app_input.c"
        "../../emulator_common/app/src/mia_app_save.c"
        "../../emulator_common/app/src/mia_app_video.c"
        "../../emulator_common/app/src/mia_emulator_host.c"
        "../../emulator_common/app/src/mia_emulator_picker.c"
        "../../emulator_common/core/src/mia_emulator_core.c"
        "../../emulator_common/hardware/src/mia_hardware_audio.c"
        "../../emulator_common/hardware/src/mia_hardware_display.c"
        "../../emulator_common/hardware/src/mia_hardware_input.c"
        "../../emulator_common/hardware/src/mia_hardware_status.c"
        "../../emulator_common/storage/src/mia_storage_paths.c"
        "../../emulator_common/storage/src/mia_storage_picker.c"
        "../../emulator_common/storage/src/mia_storage_save.c"
        "../../emulator_common/storage/src/mia_storage_status.c"
        ${MIA_GBSP_C_SOURCES}
        "../../common_host/host_platform.cpp"
        "../../common_host/display_host.cpp"
        "../../common_host/mia_text_runtime.cpp"
    INCLUDE_DIRS
        "."
        "../../common_host"
        "../../emulator_common/app/include"
        "../../emulator_common/core/include"
        "../../emulator_common/hardware/include"
        "../../emulator_common/runtime/include"
        "../../emulator_common/storage/include"
        "../../emulator_common/storage/src"
        "../../emulator_common/vendor/gbsp/components/gbsp-libretro"
        "../../emulator_common/vendor/gbsp/components/gbsp-libretro/libretro/libretro-common/include"
        "../../../../include"
    REQUIRES ${MIA_PLATFORM_REQUIRES} elf_loader)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    MIA_EMULATOR_TARGET="gba"
    MIA_APP_NAME="gba"
    MIA_EMULATOR_ROM_ROOT="/roms/gba"
    MIA_EMULATOR_SAVE_ROOT="/saves/gba"
    MIA_EMULATOR_WIDTH=240
    MIA_EMULATOR_HEIGHT=160
    MIA_EMULATOR_SAMPLE_RATE=22050
    GBA_SOUND_FREQUENCY=22050
    MIA_EMULATOR_GBA=1
    RETRO_GO=1
    ROM_BUFFER_SIZE=4
    OVERCLOCK_60FPS=1
    MIA_DISPLAY_DROID_GBK_SHARED=1
    MIA_DISPLAY_INDEXED_BUFFER_PSRAM=1
    MIA_DISPLAY_NO_DELAY_YIELD=1
    MIA_HC165_ACTIVE_HIGH=1
    MIA_AUDIO_DMA_BUF_COUNT=10
    MIA_PICKER_COVERS=1
)
target_compile_options(${COMPONENT_LIB} PRIVATE -O3 -fomit-frame-pointer -ffast-math -Wno-unused-variable -Wno-implicit-fallthrough -Wno-missing-field-initializers -Wno-unused-label -Wno-narrowing)
