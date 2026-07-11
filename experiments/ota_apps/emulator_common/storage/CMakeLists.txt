cmake_minimum_required(VERSION 3.20)
project(mia_emulator_storage C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_library(mia_emulator_storage
  src/mia_storage_paths.c
  src/mia_storage_picker.c
  src/mia_storage_save.c
  src/mia_storage_status.c
)
target_include_directories(mia_emulator_storage PUBLIC include)
target_compile_options(mia_emulator_storage PRIVATE -Wall -Wextra -Werror -pedantic)

add_library(storage_generated_targets STATIC ${MIA_STORAGE_TARGETS_C})
target_include_directories(storage_generated_targets PUBLIC include)
target_compile_options(storage_generated_targets PRIVATE -Wall -Wextra -Werror -pedantic)

function(mia_storage_test name source)
  add_executable(${name} ${source} tests/test_support.cpp)
  target_include_directories(${name} PRIVATE include tests)
  target_link_libraries(${name} PRIVATE mia_emulator_storage storage_generated_targets)
  target_compile_options(${name} PRIVATE -Wall -Wextra -Werror -pedantic)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

mia_storage_test(storage_picker_tests tests/storage_picker_tests.cpp)
mia_storage_test(storage_save_tests tests/storage_save_tests.cpp)
mia_storage_test(storage_invalid_tests tests/storage_invalid_tests.cpp)
