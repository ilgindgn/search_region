cmake_minimum_required(VERSION 2.6)

project(SearchRegion)

add_definitions (-DMINIMIZE)

include_directories ("${PROJECT_BINARY_DIR}")
include_directories ("${PROJECT_SOURCE_DIR}/common")
add_subdirectory (common)
set (EXTRA_LIBS ${EXTRA_LIBS} common)

add_subdirectory (neighborhood)
add_subdirectory (classic_filtering)
add_subdirectory (classic)
