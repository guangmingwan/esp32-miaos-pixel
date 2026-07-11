file(GLOB_RECURSE MIA_SMSPLUS_SOURCES
    "../../emulator_common/vendor/retro-core/components/smsplus/*.c")

idf_component_register(
    SRCS
        "app_main.c"
        "../../emulator_common/app/mia_emulator_app.c"
        "../../emulator_common/app/src/mia_app_audio.c"
        "../../emulator_common/app/src/mia_app_input.c"
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
    MIA_APP_NAME="${MIA_TARGET}"
    MIA_EMULATOR_ROM_ROOT="/roms/${MIA_TARGET}"
    MIA_EMULATOR_SAVE_ROOT="/saves/${MIA_TARGET}"
    MIA_EMULATOR_WIDTH=${MIA_WIDTH}
    MIA_EMULATOR_HEIGHT=${MIA_HEIGHT}
    MIA_EMULATOR_SAMPLE_RATE=32000
    MIA_EMULATOR_SMSPLUS=1
    MIA_SMSPLUS_MODE=${MIA_SMSPLUS_MODE_VALUE})

if(DEFINED MIA_SECOND_EXTENSION)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        MIA_EMULATOR_SECOND_EXTENSION="${MIA_SECOND_EXTENSION}")
endif()
