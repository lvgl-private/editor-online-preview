cmake_minimum_required(VERSION 3.10)


# Enable verbose make output
set(CMAKE_VERBOSE_MAKEFILE ON)

# Normalize paths to avoid inconsistencies
get_filename_component(USER_SRC_DIR "${USER_SRC_DIR}" ABSOLUTE)
get_filename_component(LVGL_SRC_DIR "${LVGL_SRC_DIR}" ABSOLUTE)
get_filename_component(LVGL_CONF_DIR "${LVGL_CONF_DIR}" ABSOLUTE)

# Set project and settings
project(${PROJECT_NAME} LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_SDL=2")
set(CMAKE_EXECUTABLE_SUFFIX ".js")

set(LV_CONF_PATH "${LVGL_CONF_DIR}/lv_conf.h")
set(LV_CONF_BUILD_DISABLE_EXAMPLES true)
set(LV_CONF_BUILD_DISABLE_DEMOS true)

# Define common include directories once
set(COMMON_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lvgl
    ${LVGL_SRC_DIR}
    ${LVGL_SRC_DIR}/src/others/xml
    ${LVGL_SRC_DIR}/src
    ${LVGL_CONF_DIR}
    ${USER_SRC_DIR}/widgets
    ${USER_SRC_DIR}/components
    ${USER_SRC_DIR}
)

# WILLIAM TODO: make all sources captured from auto-generated list of .c files

# Find all user sources
file(GLOB ALL_COMPONENT_FILES "${USER_SRC_DIR}/*/*")

file(GLOB SCREEN_SOURCES "${USER_SRC_DIR}/screens/*.c" "${USER_SRC_DIR}/screens/*/*.c")
file(GLOB WIDGET_SOURCES "${USER_SRC_DIR}/widgets/*.c" "${USER_SRC_DIR}/widgets/*/*.c")
file(GLOB COMPONENT_SOURCES "${USER_SRC_DIR}/components/*.c" "${USER_SRC_DIR}/components/*/*.c")
# fonts converted to .c files will be in /fonts
file(GLOB FONT_SOURCES "${USER_SRC_DIR}/fonts/*.c" "${USER_SRC_DIR}/fonts/*/*.c")
# images converted to .c files will be in /images
file(GLOB IMAGE_SOURCES "${USER_SRC_DIR}/images/*.c" "${USER_SRC_DIR}/images/*/*.c")

file(APPEND "${OUTPUT_DIR}/build.log" "\nAll files in components directory:\n${COMPONENT_SOURCES}\n")

# Find all source files in USER_SRC_DIR and CMAKE_SOURCE_DIR/src
file(GLOB USER_SOURCES "${USER_SRC_DIR}/*.c")
file(GLOB CMAKE_SOURCES "${CMAKE_SOURCE_DIR}/src/*.c")

# Set project sources
set(PROJECT_SOURCES
    ${USER_SOURCES}
    ${CMAKE_SOURCES}
)

# Create the executable with all sources
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${SCREEN_SOURCES}
    ${WIDGET_SOURCES}
    ${COMPONENT_SOURCES}
    ${FONT_SOURCES}
    ${IMAGE_SOURCES}
)

# Set include directories for the main executable
target_include_directories(${PROJECT_NAME} PRIVATE ${COMMON_INCLUDE_DIRS})

# Make sure LVGL configuration is properly set
target_compile_definitions(${PROJECT_NAME} PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    LV_CONF_PATH="${LVGL_CONF_DIR}/lv_conf.h"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    LV_EDITOR_PREVIEW=1
    LV_USE_XML=1
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/lib/liblvgl.a"
    m
)

# Define exported functions before link flags
set(BASE_EXPORTED_FUNCTIONS
    "['_malloc', '_free', \
    '_lvrt_initialize', '_ui_init', '_lvrt_cleanup_runtime', \
    '_lvrt_process_data', '_lvrt_xml_load_component_data', '_lvrt_component_create', \
    '_lvrt_resize_canvas', '_lvrt_get_obj_area', \
    '_lvrt_xml_load_translations', '_lvrt_translation_set_language', \
    '_lvrt_xml_test_register_from_data', '_lvrt_xml_test_run_init', '_lvrt_xml_test_run_next', '_lvrt_xml_test_run_stop', \
    '_lvrt_set_subject_int', '_lvrt_set_subject_string', '_lvrt_set_subject_float', '_lvrt_subscribe_subject', \
    '_lvrt_get_view_type']"
)

# Combine with additional functions if provided and log the information
file(APPEND "${OUTPUT_DIR}/build.log" "\nExported functions:\n")
if(DEFINED ADDITIONAL_EXPORTED_FUNCTIONS)
    # Remove any brackets and trailing commas from ADDITIONAL_EXPORTED_FUNCTIONS
    string(REGEX REPLACE "[][]" "" CLEANED_ADDITIONAL_FUNCTIONS "${ADDITIONAL_EXPORTED_FUNCTIONS}")
    string(REGEX REPLACE ",[ ]*$" "" CLEANED_ADDITIONAL_FUNCTIONS "${CLEANED_ADDITIONAL_FUNCTIONS}")
    string(REPLACE "]" ", ${CLEANED_ADDITIONAL_FUNCTIONS}]" ALL_EXPORTED_FUNCTIONS "${BASE_EXPORTED_FUNCTIONS}")
    file(APPEND "${OUTPUT_DIR}/build.log" "Additional exported functions: ${CLEANED_ADDITIONAL_FUNCTIONS}\n")
else()
    set(ALL_EXPORTED_FUNCTIONS "${BASE_EXPORTED_FUNCTIONS}")
    file(APPEND "${OUTPUT_DIR}/build.log" "No additional exported functions provided\n")
endif()
file(APPEND "${OUTPUT_DIR}/build.log" "All exported functions: ${ALL_EXPORTED_FUNCTIONS}\n")

#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_FUNCTIONS=[${ALL_EXPORTED_FUNCTIONS}]")

# Set target properties with the exported functions
set_target_properties(${PROJECT_NAME} PROPERTIES
    COMPILE_FLAGS "--use-port=sdl2"
    LINK_FLAGS "--no-entry -s WASM=0 -s USE_WEBGL2=0 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=0 \
    -s EXPORTED_FUNCTIONS='${ALL_EXPORTED_FUNCTIONS}' \
    -s EXPORT_NAME='createModule' \
    -s FORCE_FILESYSTEM=1 \
    -s EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\", \"FS\", \"addFunction\", \"getValue\", \"setValue\"]' \
    -s MODULARIZE=1 \
    -s USE_SDL=2 \
    -s ENVIRONMENT=web"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
)
