file(GLOB MIA_NOFRENDO_SOURCES "../components/nofrendo/*.c" "../components/nofrendo/nes/*.c" "../components/nofrendo/mappers/*.c")

idf_component_register(
    SRCS "app_main.c" "../../emulator_common/app/mia_emulator_app.c"
        "../../emulator_common/app/src/mia_app_audio.c" "../../emulator_common/app/src/mia_app_input.c"
        "../../emulator_common/app/src/mia_app_save.c" "../../emulator_common/app/src/mia_app_video.c"
        "../../emulator_common/app/src/mia_emulator_host.c" "../../emulator_common/app/src/mia_emulator_picker.c"
        "../../emulator_common/core/src/mia_emulator_core.c" "../../emulator_common/cores/nes/mia_nes_contract.c"
        "../../emulator_common/cores/nes/mia_nes_core.c" "../../emulator_common/hardware/src/mia_hardware_audio.c"
        "../../emulator_common/hardware/src/mia_hardware_display.c" "../../emulator_common/hardware/src/mia_hardware_input.c"
        "../../emulator_common/hardware/src/mia_hardware_status.c" "../../emulator_common/hardware/src/mia_hardware_target.c"
        "../../emulator_common/storage/src/mia_storage_paths.c" "../../emulator_common/storage/src/mia_storage_picker.c"
        "../../emulator_common/storage/src/mia_storage_save.c" "../../emulator_common/storage/src/mia_storage_status.c"
        ${MIA_NOFRENDO_SOURCES} "../../common_host/host_platform.cpp" "../../common_host/display_host.cpp"
        "../../common_host/droid_gbk_renderer.cpp" "../../../../src/fonts/droid_gbk_12.c"
        "../../../../src/gbk_unicode_map.cpp" "../../../../src/droid_gbk_index.cpp"
    INCLUDE_DIRS "." "../../common_host" "../../emulator_common/app/include" "../../emulator_common/core/include"
        "../../emulator_common/cores/nes" "../../emulator_common/hardware/include" "../../emulator_common/runtime/include"
        "../../emulator_common/storage/include" "../../emulator_common/storage/src" "../components/nofrendo" "../../../../include")
target_compile_definitions(${COMPONENT_LIB} PRIVATE MIA_EMULATOR_TARGET="nes" MIA_EMULATOR_EXTENSION="nes"
    MIA_EMULATOR_SECOND_EXTENSION="fds" MIA_EMULATOR_THIRD_EXTENSION="fc"
    MIA_EMULATOR_FOURTH_EXTENSION="nsf" MIA_APP_NAME="nes" MIA_EMULATOR_CATEGORY="Emulators"
    MIA_EMULATOR_ROM_ROOT="/roms/nes" MIA_EMULATOR_SECOND_ROM_ROOT="/roms/FC"
    MIA_EMULATOR_SAVE_ROOT="/saves/nes" MIA_EMULATOR_WIDTH=256
    MIA_EMULATOR_HEIGHT=240 MIA_EMULATOR_SAMPLE_RATE=32000 MIA_EMULATOR_SMSPLUS=1
    MIA_DISPLAY_DROID_GBK=1 MIA_PICKER_COVERS=1 MIA_HC165_ACTIVE_HIGH=1)
target_compile_options(${COMPONENT_LIB} PRIVATE -O3 -Wno-array-bounds -Wno-format)
