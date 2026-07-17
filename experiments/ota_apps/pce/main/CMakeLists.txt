file(GLOB PCE_GO_SOURCES "../vendor/pce-go/*.c")
idf_component_register(
    SRCS "app_main.c" "pce_adapter.c"
        "../../emulator_common/app/mia_emulator_app.c"
        "../../emulator_common/app/src/mia_app_audio.c" "../../emulator_common/app/src/mia_app_input.c"
        "../../emulator_common/app/src/mia_app_zip.c"
        "../../emulator_common/app/src/mia_app_save.c" "../../emulator_common/app/src/mia_app_video.c"
        "../../emulator_common/app/src/mia_emulator_host.c" "../../emulator_common/app/src/mia_emulator_picker.c"
        "../../emulator_common/core/src/mia_emulator_core.c"
        "../../emulator_common/hardware/src/mia_hardware_audio.c" "../../emulator_common/hardware/src/mia_hardware_display.c"
        "../../emulator_common/hardware/src/mia_hardware_input.c" "../../emulator_common/hardware/src/mia_hardware_status.c"
        "../../emulator_common/hardware/src/mia_hardware_target.c"
        "../../emulator_common/storage/src/mia_storage_paths.c" "../../emulator_common/storage/src/mia_storage_picker.c"
        "../../emulator_common/storage/src/mia_storage_save.c" "../../emulator_common/storage/src/mia_storage_status.c"
        ${PCE_GO_SOURCES} "../../common_host/host_platform.cpp" "../../common_host/display_host.cpp"
        "../../common_host/mia_text_runtime.cpp"
    INCLUDE_DIRS "." "../vendor/pce-go" "../../common_host" "../../emulator_common/app/include"
        "../../emulator_common/core/include" "../../emulator_common/hardware/include" "../../emulator_common/runtime/include"
        "../../emulator_common/storage/include" "../../emulator_common/storage/src" "../../../../include"
    PRIV_REQUIRES
        elf_loader)
target_compile_definitions(${COMPONENT_LIB} PRIVATE MIA_EMULATOR_TARGET="pce" MIA_EMULATOR_EXTENSION="pce"
    MIA_EMULATOR_SECOND_EXTENSION="zip"
    MIA_APP_NAME="pce" MIA_EMULATOR_ROM_ROOT="/roms/pce" MIA_EMULATOR_SAVE_ROOT="/saves/pce"
    MIA_EMULATOR_WIDTH=256 MIA_EMULATOR_HEIGHT=224 MIA_EMULATOR_SAMPLE_RATE=22050 MIA_EMULATOR_GNUBOY=1
    MIA_DISPLAY_DROID_GBK_SHARED=1 MIA_DISPLAY_INDEXED_BUFFER_PSRAM=1 MIA_PICKER_COVERS=1
    MIA_HC165_ACTIVE_HIGH=1 MIA_DISPLAY_ASYNC_REGION=1 MIA_DISPLAY_NO_DELAY_YIELD=1
    MIA_DISPLAY_PRESENT_ROWS=32)
