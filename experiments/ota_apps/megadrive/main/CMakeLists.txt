file(GLOB GWENESIS_SOURCES
    "../vendor/gwenesis/src/bus/*.c" "../vendor/gwenesis/src/cpus/M68K/*.c"
    "../vendor/gwenesis/src/cpus/Z80/*.c" "../vendor/gwenesis/src/io/*.c"
    "../vendor/gwenesis/src/savestate/*.c" "../vendor/gwenesis/src/sound/*.c"
    "../vendor/gwenesis/src/vdp/*.c")
idf_component_register(SRCS "app_main.c" "../adapter/megadrive_policy.c" "../adapter/mia_megadrive.c"
    "../../emulator_common/app/mia_emulator_app.c"
    "../../emulator_common/app/src/mia_app_audio.c" "../../emulator_common/app/src/mia_app_input.c"
    "../../emulator_common/app/src/mia_app_zip.c"
    "../../emulator_common/app/src/mia_app_save.c" "../../emulator_common/app/src/mia_app_video.c"
    "../../emulator_common/app/src/mia_emulator_host.c" "../../emulator_common/app/src/mia_emulator_picker.c"
    "../../emulator_common/core/src/mia_emulator_core.c"
    "../../emulator_common/hardware/src/mia_hardware_audio.c" "../../emulator_common/hardware/src/mia_hardware_display.c"
    "../../emulator_common/hardware/src/mia_hardware_input.c" "../../emulator_common/hardware/src/mia_hardware_status.c"
    "../../emulator_common/storage/src/mia_storage_paths.c" "../../emulator_common/storage/src/mia_storage_picker.c"
    "../../emulator_common/storage/src/mia_storage_save.c" "../../emulator_common/storage/src/mia_storage_status.c"
    ${GWENESIS_SOURCES} "../../common_host/host_platform.cpp" "../../common_host/display_host.cpp"
    "../../common_host/mia_text_runtime.cpp"
    INCLUDE_DIRS "." "../adapter" "../vendor/gwenesis" "../vendor/gwenesis/src/bus"
    "../vendor/gwenesis/src/cpus/M68K" "../vendor/gwenesis/src/cpus/Z80" "../vendor/gwenesis/src/io"
    "../vendor/gwenesis/src/savestate" "../vendor/gwenesis/src/sound" "../vendor/gwenesis/src/vdp"
    "../../common_host" "../../emulator_common/app/include" "../../emulator_common/core/include"
    "../../emulator_common/hardware/include" "../../emulator_common/runtime/include"
    "../../emulator_common/storage/include" "../../emulator_common/storage/src" "../../../../include"
    PRIV_REQUIRES
        elf_loader)
target_compile_definitions(${COMPONENT_LIB} PRIVATE MIA_EMULATOR_TARGET="megadrive"
    MIA_EMULATOR_EXTENSION="bin" MIA_EMULATOR_SECOND_EXTENSION="smd"
    MIA_EMULATOR_THIRD_EXTENSION="md" MIA_EMULATOR_FOURTH_EXTENSION="gen"
    MIA_EMULATOR_FIFTH_EXTENSION="zip" MIA_EMULATOR_SMSPLUS=1
    MIA_APP_NAME="megadrive"
    MIA_EMULATOR_ROM_ROOT="/roms/md" MIA_EMULATOR_SECOND_ROM_ROOT="/roms/megadrive"
    MIA_EMULATOR_SAVE_ROOT="/saves/megadrive"
    MIA_EMULATOR_WIDTH=320 MIA_EMULATOR_HEIGHT=224 MIA_EMULATOR_SAMPLE_RATE=26633
    RETRO_GO=1 GNW_TARGET_MARIO=0 GNW_TARGET_ZELDA=0 GW_TARGET=0
    MIA_DISPLAY_DROID_GBK_SHARED=1 MIA_DISPLAY_INDEXED_BUFFER_PSRAM=1 MIA_PICKER_COVERS=1)
target_compile_options(${COMPONENT_LIB} PRIVATE -O3 -fomit-frame-pointer -Wno-unused-variable
    -Wno-unused-function -Wno-incompatible-pointer-types -Wno-implicit-fallthrough)
