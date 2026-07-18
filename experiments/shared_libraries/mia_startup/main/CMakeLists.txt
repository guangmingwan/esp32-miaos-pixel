idf_component_register(
    SRCS
        "mia_startup_export.c"
        "../../../../src/mia_startup_menu.c"
    INCLUDE_DIRS
        "."
        "../../../../include"
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -fPIC
    -fdata-sections
    -ffunction-sections
    -fvisibility=hidden
)
