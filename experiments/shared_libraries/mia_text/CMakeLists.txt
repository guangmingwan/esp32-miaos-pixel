cmake_minimum_required(VERSION 3.16)

set(COMPONENTS main)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(libmia_text_v1)

set(MIA_TEXT_SO "${CMAKE_BINARY_DIR}/libmia_text_v1.so")
string(REPLACE "-elf-gcc" "-elf-strip" MIA_TEXT_STRIP ${CMAKE_C_COMPILER})

add_custom_command(
    OUTPUT ${MIA_TEXT_SO}
    COMMAND ${CMAKE_C_COMPILER}
        -shared -fPIC -static-libgcc -nostdlib -nostartfiles
        -fdata-sections -ffunction-sections -Wl,--gc-sections
        -Wl,--strip-all -Wl,--strip-debug -Wl,--strip-discarded
        -o ${MIA_TEXT_SO}
        -Wl,--whole-archive ${CMAKE_BINARY_DIR}/esp-idf/main/libmain.a
        -Wl,--no-whole-archive -Wl,--allow-shlib-undefined
    COMMAND ${MIA_TEXT_STRIP}
        --strip-unneeded
        --remove-section=.comment
        --remove-section=.got.loc
        --remove-section=.dynamic
        --remove-section=.xt.lit
        --remove-section=.xt.prop
        --remove-section=.xtensa.info
        ${MIA_TEXT_SO}
    DEPENDS idf::main
    COMMENT "Build shared library: libmia_text_v1.so"
)

add_custom_target(so DEPENDS ${MIA_TEXT_SO})
