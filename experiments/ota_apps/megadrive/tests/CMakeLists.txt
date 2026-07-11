cmake_minimum_required(VERSION 3.16)
project(mia_megadrive_tests C)

enable_testing()
add_executable(megadrive_policy_tests megadrive_policy_tests.c ../adapter/megadrive_policy.c)
target_include_directories(megadrive_policy_tests PRIVATE ../adapter)
target_compile_options(megadrive_policy_tests PRIVATE -Wall -Wextra -Werror)
add_test(NAME megadrive_policy_tests COMMAND megadrive_policy_tests)
