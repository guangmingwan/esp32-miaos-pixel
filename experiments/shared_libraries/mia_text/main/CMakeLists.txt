idf_component_register(
    SRCS
        "mia_text_export.c"
        "../../../ota_apps/common_host/droid_gbk_renderer.cpp"
        "../../../../src/fonts/droid_gbk_12.c"
        "../../../../src/gbk_unicode_map.cpp"
        "../../../../src/droid_gbk_index.cpp"
    INCLUDE_DIRS
        "."
        "../../../ota_apps/common_host"
        "../../../../include"
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -fPIC
    -fdata-sections
    -ffunction-sections
    -fvisibility=hidden
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE MIA_DROID_GBK_USE_USLEEP=1)
