idf_component_register(
    SRCS
        "mia_sdl_impl.c"
    INCLUDE_DIRS
        "."
        "../include"
        "../../../../include"
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -fPIC
    -fdata-sections
    -ffunction-sections
    -fvisibility=hidden
    -Wno-error
)
