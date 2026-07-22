set(GMU_SRC ../vendor/gmu-src)

idf_component_register(
    SRCS
        app_main.c
        gmu_audio.c
        gmu_decoder.c
        gmu_extra_decoders.c
        gmu_frontend.c
        gmu_feloader.c
        gmu_decloader.c
        gmu_hw.c
        gmu_posix_stubs.c
        ${GMU_SRC}/core.c
        ${GMU_SRC}/charset.c
        ${GMU_SRC}/debug.c
        ${GMU_SRC}/dir.c
        ${GMU_SRC}/dirparser.c
        ${GMU_SRC}/eventqueue.c
        ${GMU_SRC}/fileplayer.c
        ${GMU_SRC}/fmath.c
        ${GMU_SRC}/gmuerror.c
        ${GMU_SRC}/id3.c
        ${GMU_SRC}/m3u.c
        ${GMU_SRC}/metadatareader.c
        ${GMU_SRC}/playlist.c
        ${GMU_SRC}/pls.c
        ${GMU_SRC}/pthread_helper.c
        ${GMU_SRC}/reader.c
        ${GMU_SRC}/ringbuffer.c
        ${GMU_SRC}/trackinfo.c
        ${GMU_SRC}/util.c
        ${GMU_SRC}/wejconfig.c
        ${GMU_SRC}/bmp.c
        ${GMU_SRC}/jpeg.c
        ${GMU_SRC}/png.c
        ../../common_host/host_platform.cpp
        ../../common_host/display_host.cpp
        ../../common_host/mia_text_runtime.cpp
    INCLUDE_DIRS
        "."
        "../vendor/gmu-src"
        "../../common_host"
        "../../../../include"
        "../third_party"
    REQUIRES app_update esp_partition driver fatfs sdmmc nvs_flash esp_timer
        esp_event esp_netif esp_wifi esp_http_server esp_adc esp_driver_spi
        esp_driver_i2s esp_driver_i2c elf_loader
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    STATIC=1
    main=gmu_main
    FILE_HW_H="hw_esp32s3.h"
    GMU_INSTALL_PREFIX="/sd/MiaOS/Library/gmu"
    GMU_DISABLE_OSS_MIXER=1
    MIA_DISPLAY_DROID_GBK_SHARED=1
)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -O2 -ffast-math -fno-strict-aliasing
    -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-function
    -Wno-unused-parameter -Wno-sign-compare -Wno-missing-field-initializers
    -Wno-format-truncation
)
