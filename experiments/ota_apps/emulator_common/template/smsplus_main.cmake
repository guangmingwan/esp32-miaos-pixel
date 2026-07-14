file(GLOB_RECURSE MIA_SMSPLUS_SOURCES
    "../../emulator_common/vendor/retro-core/components/smsplus/*.c")

if(NOT DEFINED MIA_ROM_DIRECTORY)
    set(MIA_ROM_DIRECTORY ${MIA_TARGET})
endif()

idf_component_register(
    SRCS
        "app_main.c"
        "../../emulator_common/app/mia_emulator_app.c"
        "../../emulator_common/app/src/mia_app_audio.c"
        "../../emulator_common/app/src/mia_app_input.c"
        "../../emulator_common/app/src/mia_app_zip.c"
        "../../emulator_common/app/src/mia_app_save.c"
        "../../emulator_common/app/src/mia_app_video.c"
        "../../emulator_common/app/src/mia_emulator_host.c"
        "../../emulator_common/app/src/mia_emulator_picker.c"
        "../../emulator_common/app/src/mia_emulator_smsplus.c"
        "../../emulator_common/app/src/mia_emulator_smsplus_core.c"
        "../../emulator_common/core/src/mia_emulator_core.c"
        "../../emulator_common/hardware/src/mia_hardware_audio.c"
        "../../emulator_common/hardware/src/mia_hardware_display.c"
        "../../emulator_common/hardware/src/mia_hardware_input.c"
        "../../emulator_common/hardware/src/mia_hardware_status.c"
        "../../emulator_common/hardware/src/mia_hardware_target.c"
        "../../emulator_common/storage/src/mia_storage_paths.c"
        "../../emulator_common/storage/src/mia_storage_picker.c"
        "../../emulator_common/storage/src/mia_storage_save.c"
        "../../emulator_common/storage/src/mia_storage_status.c"
        ${MIA_SMSPLUS_SOURCES}
        "../../common_host/host_platform.cpp"
        "../../common_host/display_host.cpp"
        "../../common_host/droid_gbk_renderer.cpp"
        "../../../../src/fonts/droid_gbk_12.c"
        "../../../../src/gbk_unicode_map.cpp"
        "../../../../src/droid_gbk_index.cpp"
    INCLUDE_DIRS
        "." "../../common_host"
        "../../emulator_common/app/include"
        "../../emulator_common/core/include"
        "../../emulator_common/hardware/include"
        "../../emulator_common/runtime/include"
        "../../emulator_common/storage/include"
        "../../emulator_common/storage/src"
        "../../emulator_common/vendor/retro-core/components/smsplus"
        "../../emulator_common/vendor/retro-core/components/smsplus/cpu"
        "../../emulator_common/vendor/retro-core/components/smsplus/sound"
        "../../../../include")

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    MIA_EMULATOR_TARGET="${MIA_TARGET}"
    MIA_EMULATOR_EXTENSION="${MIA_EXTENSION}"
    MIA_EMULATOR_THIRD_EXTENSION="zip"
    MIA_APP_NAME="${MIA_TARGET}"
    MIA_EMULATOR_ROM_ROOT="/roms/${MIA_ROM_DIRECTORY}"
    MIA_EMULATOR_SAVE_ROOT="/saves/${MIA_TARGET}"
    MIA_EMULATOR_WIDTH=${MIA_WIDTH}
    MIA_EMULATOR_HEIGHT=${MIA_HEIGHT}
    MIA_EMULATOR_SAMPLE_RATE=32000
    MIA_EMULATOR_DUAL_CORE_AUDIO=1
    MIA_DISPLAY_DROID_GBK=1
    MIA_DISPLAY_PRESENT_ROWS=40
    MIA_DISPLAY_NO_DELAY_YIELD=1
    MIA_DISPLAY_RGB565_WIRE_ORDER=1
    MIA_HC165_ACTIVE_HIGH=1
    MIA_EMULATOR_SMSPLUS=1
    MIA_SMSPLUS_MODE=${MIA_SMSPLUS_MODE_VALUE}
    ${MIA_PICKER_EXTRA_DEFINITIONS})

if(DEFINED MIA_SECOND_EXTENSION)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        MIA_EMULATOR_SECOND_EXTENSION="${MIA_SECOND_EXTENSION}")
endif()

if(DEFINED MIA_SECOND_ROM_DIRECTORY)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        MIA_EMULATOR_SECOND_ROM_ROOT="/roms/${MIA_SECOND_ROM_DIRECTORY}")
endif()

if(DEFINED MIA_SMSPLUS_SAVE_INTERVAL_FRAMES)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        MIA_SMSPLUS_SAVE_INTERVAL_FRAMES=${MIA_SMSPLUS_SAVE_INTERVAL_FRAMES})
endif()
