cmake_minimum_required(VERSION 3.16)
project(mia_gba_tests C)

enable_testing()
add_executable(gba_policy_tests gba_policy_tests.c ../main/gba_policy.c)
target_include_directories(gba_policy_tests PRIVATE ../main)
target_compile_options(gba_policy_tests PRIVATE -Wall -Wextra -Werror)
add_test(NAME gba_policy_tests COMMAND gba_policy_tests)
