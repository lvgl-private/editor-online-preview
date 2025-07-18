#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdint.h>

typedef enum {
    RESOURCE_TYPE_IMAGE,
    RESOURCE_TYPE_FONT
} resource_type_t;

typedef struct {
    const char* file_format;
} image_config_t;

typedef struct {
    int32_t font_size;
} font_config_t;

typedef union {
    image_config_t image;
    font_config_t font;
} resource_config_t;

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <src/libs/expat/expat.h>
#include <lv_xml.h>
#include <lv_xml_component.h>
#include <lv_xml_widget.h>
//#include <lv_sdl_window.h>
#include <emscripten.h>
#include <SDL2/SDL.h>
#include <lvgl.h>
#include "lv_conf.h"

//#include "../components/parser/xml/lv_xml.h"

typedef struct {
    uint8_t* buffer;
    size_t size;
    int width;
    int height;
    int bytes_per_pixel;  
} screenshot_data_t;

typedef void (*display_refresh_cb_t)(lv_display_t* disp);

/* Function declarations */
int lvrt_initialize(const char *canvas_selector);
void do_loop(void* arg);
void lvrt_task_handler();
void lvrt_cleanup();
int lvrt_process_data(const char *xml_definition, const char *name, const char *display_style[], const char *xml_type, const char *language);
int lvrt_xml_load_component_data(const char *name, const char *xml_definition);
int lvrt_xml_load_translations(const char *translations_path);
void lvrt_translation_set_language(const char *language);
void lvrt_resize_canvas(int width, int height);
bool lvrt_get_obj_area(const char* name_path, int* x, int* y, int* width, int* height, int* layout_positioned, int* alignment, int* pad_top, int* pad_right, int* pad_bottom, int* pad_left, int* margin_top, int* margin_right, int* margin_bottom, int* margin_left, int* hitarea);
int lvrt_xml_test_register_from_data(const char *xml_definition, const char * ref_image_path_prefix);
void lvrt_xml_test_run_init();
bool lvrt_xml_test_run_next(uint32_t slowdown);
void lvrt_xml_test_run_stop();
void lvrt_set_subject_int(const char* name, int32_t v);
void lvrt_set_subject_string(const char* name, const char* v);
void lvrt_set_subject_float(const char* name, float v);
bool lvrt_subscribe_subject(const char* name);
const char* lvrt_get_view_type(const char* root_element_name);
int lvrt_component_create(const char *name);

#endif /* RUNTIME_H */


