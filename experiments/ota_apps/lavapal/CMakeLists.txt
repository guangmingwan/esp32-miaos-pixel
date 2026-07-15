cmake_minimum_required(VERSION 3.16)

list(APPEND EXTRA_COMPONENT_DIRS "../common_host/lava")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(lava_pal)
