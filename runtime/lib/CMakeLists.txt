# Minimum CMake version required
cmake_minimum_required(VERSION 3.14)

# Project name and settings
project(lvgl_static_library LANGUAGES C VERSION 1.0)

# Options to control the build
option(LV_BUILD_TESTS "Build LVGL tests" OFF)
option(LV_BUILD_EXAMPLES "Build LVGL examples" OFF)
option(LVGL_STATIC_LIB "Build LVGL as a static library" ON)

# Set LVGL paths
set(LVGL_SRC_DIR "${CMAKE_SOURCE_DIR}/../lvgl/src")
set(LVGL_INC_DIR "${CMAKE_SOURCE_DIR}/../lvgl")
set(LVGL_CONF_PATH "${CMAKE_SOURCE_DIR}/../conf/lv_conf.h")
set(LVGL_CONF_DIR "${CMAKE_SOURCE_DIR}/../conf")

# Collect all LVGL source files
file(GLOB_RECURSE LVGL_SOURCES
    ${LVGL_SRC_DIR}/*.c
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Create static library
add_library(lvgl STATIC ${LVGL_SOURCES})

# Set include directories
target_include_directories(lvgl PUBLIC
    ${LVGL_INC_DIR}
    ${LVGL_CONF_DIR}
)

# Set compile definitions
target_compile_definitions(lvgl PUBLIC
    LV_CONF_INCLUDE_SIMPLE
)
