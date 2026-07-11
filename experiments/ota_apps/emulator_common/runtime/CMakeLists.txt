cmake_minimum_required(VERSION 3.20)
project(mia_emulator_runtime C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_library(mia_emulator_runtime
  src/mia_runtime_alloc.c
  src/mia_runtime_lifecycle.c
  src/mia_runtime_status.c
  src/mia_runtime_target.c
  src/mia_runtime_time.c
)
target_include_directories(mia_emulator_runtime PUBLIC include)
target_compile_options(mia_emulator_runtime PRIVATE -Wall -Wextra -Werror -pedantic)

add_library(runtime_generated_targets STATIC ${MIA_RUNTIME_TARGETS_C})
target_include_directories(runtime_generated_targets PUBLIC include)
target_compile_options(runtime_generated_targets PRIVATE -Wall -Wextra -Werror -pedantic)

function(mia_runtime_test name source)
  add_executable(${name} ${source} tests/test_support.cpp)
  target_include_directories(${name} PRIVATE include tests)
  target_link_libraries(${name} PRIVATE mia_emulator_runtime runtime_generated_targets)
  target_compile_options(${name} PRIVATE -Wall -Wextra -Werror -pedantic)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

mia_runtime_test(runtime_time_tests tests/runtime_time_tests.cpp)
mia_runtime_test(runtime_alloc_tests tests/runtime_alloc_tests.cpp)
mia_runtime_test(runtime_lifecycle_tests tests/runtime_lifecycle_tests.cpp)
mia_runtime_test(runtime_target_tests tests/runtime_target_tests.cpp)
