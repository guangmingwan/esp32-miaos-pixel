cmake_minimum_required(VERSION 3.20)
project(mia_emulator_hardware C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_library(mia_emulator_hardware
  src/mia_hardware_audio.c
  src/mia_hardware_display.c
  src/mia_hardware_input.c
  src/mia_hardware_status.c
  src/mia_hardware_target.c
)
target_include_directories(mia_emulator_hardware PUBLIC include)
target_compile_options(mia_emulator_hardware PRIVATE -Wall -Wextra -Werror -pedantic)

add_library(hardware_generated_targets STATIC ${MIA_HARDWARE_TARGETS_C})
target_include_directories(hardware_generated_targets PUBLIC include)
target_compile_options(hardware_generated_targets PRIVATE -Wall -Wextra -Werror -pedantic)

function(mia_hardware_test name source)
  add_executable(${name} ${source} tests/test_support.cpp)
  target_include_directories(${name} PRIVATE include tests)
  target_link_libraries(${name} PRIVATE mia_emulator_hardware hardware_generated_targets)
  target_compile_options(${name} PRIVATE -Wall -Wextra -Werror -pedantic)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

mia_hardware_test(hardware_display_tests tests/hardware_display_tests.cpp)
mia_hardware_test(hardware_audio_tests tests/hardware_audio_tests.cpp)
mia_hardware_test(hardware_input_tests tests/hardware_input_tests.cpp)
mia_hardware_test(hardware_template_tests tests/hardware_template_tests.cpp)
mia_hardware_test(hardware_malformed_tests tests/hardware_malformed_tests.cpp)
