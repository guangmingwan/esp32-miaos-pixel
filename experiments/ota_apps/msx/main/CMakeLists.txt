file(GLOB FMSX_EMULIB "../vendor/fmsx/src/EMULib/*.c")
file(GLOB FMSX_CORE "../vendor/fmsx/src/fMSX/*.c")
file(GLOB FMSX_Z80 "../vendor/fmsx/src/Z80/*.c")

if(IDF_VERSION_MAJOR GREATER_EQUAL 5)
    set(MIA_PLATFORM_REQUIRES app_update esp_partition driver esp_driver_gpio esp_driver_spi
        esp_driver_i2s esp_driver_i2c fatfs sdmmc nvs_flash esp_timer esp_event esp_netif
        esp_wifi esp_http_server esp_adc)
else()
    set(MIA_PLATFORM_REQUIRES app_update spi_flash driver fatfs sdmmc nvs_flash esp_timer
        esp_event esp_netif esp_wifi esp_http_server)
endif()

idf_component_register(
    SRCS "app_main.c" "../target/msx_app.c" "../target/msx_machine.c" "../target/msx_policy.c"
         "../target/msx_fs.c" ${FMSX_EMULIB} ${FMSX_CORE} ${FMSX_Z80}
         "../../emulator_common/app/src/mia_emulator_picker.c"
         "../../emulator_common/app/src/mia_app_save.c"
         "../../emulator_common/storage/src/mia_storage_paths.c"
         "../../emulator_common/storage/src/mia_storage_picker.c"
         "../../emulator_common/storage/src/mia_storage_status.c"
         "../../common_host/host_platform.cpp" "../../common_host/display_host.cpp"
         "../../common_host/droid_gbk_renderer.cpp" "../../../../src/fonts/droid_gbk_12.c"
         "../../../../src/gbk_unicode_map.cpp" "../../../../src/droid_gbk_index.cpp"
    INCLUDE_DIRS "." "../target" "../vendor/fmsx" "../vendor/fmsx/src/EMULib"
         "../vendor/fmsx/src/fMSX" "../vendor/fmsx/src/Z80" "../../common_host"
         "../../emulator_common/app/include" "../../emulator_common/storage/include"
         "../../emulator_common/storage/src" "../../emulator_common/core/include"
         "../../emulator_common/hardware/include" "../../emulator_common/runtime/include"
         "../../../../include"
    REQUIRES ${MIA_PLATFORM_REQUIRES})

target_compile_definitions(${COMPONENT_LIB} PRIVATE BPS16 BPP16 UNIX LSB_FIRST NARROW
    GenericSetVideo=SetVideo MIA_DISPLAY_DROID_GBK=1 MIA_PICKER_COVERS=1)
target_compile_options(${COMPONENT_LIB} PRIVATE -O2 -include msxfix.h -Wno-implicit-fallthrough
    -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format-overflow -Wno-stringop-truncation)
