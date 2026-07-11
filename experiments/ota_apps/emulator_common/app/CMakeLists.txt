cmake_minimum_required(VERSION 3.16)
project(mia_emulator_app_tests LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED MIA_APP_TARGETS_C)
    message(FATAL_ERROR "MIA_APP_TARGETS_C is required")
endif()

add_library(app_deps STATIC
    ${MIA_APP_TARGETS_C}
    src/mia_app_audio.c
    src/mia_app_input.c
    src/mia_app_save.c
    src/mia_app_video.c
    src/mia_emulator_smsplus.c
    ../hardware/src/mia_hardware_audio.c
    ../hardware/src/mia_hardware_display.c
    ../hardware/src/mia_hardware_input.c
    ../hardware/src/mia_hardware_status.c
    ../hardware/src/mia_hardware_target.c
    ../storage/src/mia_storage_paths.c
    ../storage/src/mia_storage_picker.c
    ../storage/src/mia_storage_save.c
    ../storage/src/mia_storage_status.c
)
target_include_directories(app_deps PUBLIC include ../hardware/include ../runtime/include ../storage/include ../storage/src ../../../../include)

enable_testing()

add_executable(app_adapter_tests tests/app_adapter_tests.cpp)
target_include_directories(app_adapter_tests PRIVATE include)
target_link_libraries(app_adapter_tests PRIVATE app_deps)
add_test(NAME app_adapter_tests COMMAND app_adapter_tests)

add_executable(smsplus_bios_pipeline_tests tests/smsplus_bios_pipeline_tests.cpp)
target_include_directories(smsplus_bios_pipeline_tests PRIVATE include)
target_link_libraries(smsplus_bios_pipeline_tests PRIVATE app_deps)
add_test(NAME smsplus_bios_pipeline_tests COMMAND smsplus_bios_pipeline_tests)
