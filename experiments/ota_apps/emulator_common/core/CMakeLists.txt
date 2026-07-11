cmake_minimum_required(VERSION 3.16)
project(mia_emulator_core_tests LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED MIA_CORE_TARGETS_C)
    message(FATAL_ERROR "MIA_CORE_TARGETS_C is required")
endif()

add_library(mia_emulator_core STATIC
    src/mia_emulator_core.c
)
target_include_directories(mia_emulator_core PUBLIC include ../runtime/include)

add_library(core_generated_targets STATIC ${MIA_CORE_TARGETS_C})
target_include_directories(core_generated_targets PUBLIC include ../runtime/include)

enable_testing()

add_executable(core_adapter_tests tests/core_adapter_tests.cpp)
target_link_libraries(core_adapter_tests PRIVATE mia_emulator_core core_generated_targets)
add_test(NAME core_adapter_tests COMMAND core_adapter_tests)
