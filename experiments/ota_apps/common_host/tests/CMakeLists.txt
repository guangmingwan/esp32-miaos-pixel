cmake_minimum_required(VERSION 3.20)
project(mia_rgb565_transport_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(rgb565_transport_tests rgb565_transport_tests.cpp ../display_host.cpp)
target_include_directories(rgb565_transport_tests PRIVATE .. ../../../../include)
target_compile_definitions(rgb565_transport_tests PRIVATE MIA_DISPLAY_HOST_NATIVE_TEST=1)
target_compile_options(rgb565_transport_tests PRIVATE -Wall -Wextra -Werror -pedantic)
