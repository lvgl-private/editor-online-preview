#include "runtime.h"
#include "ui.h"
#include <dirent.h>
#include <emscripten/html5.h>
#include <lvgl/lvgl.h>
#include <lvgl/src/lvgl_private.h>
#include <time.h>

#define SDL_HINT_EMSCRIPTEN_CANVAS_SELECTOR "SDL_EMSCRIPTEN_CANVAS_SELECTOR"

bool lvrt_is_not_known_object = false;
char lvrt_unknown_widget_name[256] = {0};  // Buffer to store the unknown widget name

// Expose logs to JavaScript
EM_JS(void, js_log_callback, (const char* message), {
    if (typeof window !== 'undefined') {
        window.dispatchEvent(new CustomEvent('lvgl-log', { detail: UTF8ToString(message) }));
    }
});

EM_JS(void, js_render_error, (const char* message), {
    if (typeof window !== 'undefined') {
        window.dispatchEvent(new CustomEvent('lvgl-render-error', { detail: UTF8ToString(message) }));
    }
});

// Expose XML is rendered to JavaScript
EM_JS(void, js_xml_is_rendered, (void),  {
    if (typeof window !== 'undefined') {
        window.dispatchEvent(new CustomEvent('xml-is-rendered'));
    }
});

void lvrt_log_cb(int8_t level, const char* buf)
{
    // Match format: "'name' is not a known widget, element, or component"
    if (strstr(buf, "not a known widget") != NULL) {
        // Extract name between single quotes
        const char* start = strchr(buf, '\'');
        if (start) {
            start++;  // Skip first quote
            int i = 0;
            while (start[i] && start[i] != '\'' && i < 255) {
                lvrt_unknown_widget_name[i] = start[i];
                i++;
            }
            lvrt_unknown_widget_name[i] = '\0';
        }
        
        lvrt_is_not_known_object = true;
        
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Invalid or unknown element: <%s>", lvrt_unknown_widget_name);
        js_render_error(error_msg);
    }
    js_log_callback(buf);
}

// Add at the top with other globals
static lv_obj_t* g_fullscreen_obj = NULL;

static unsigned int runtime_id = 0;

int lvrt_initialize(const char *canvas_selector){

  lvrt_is_not_known_object = false;
  
  // Generate a new runtime ID for tracking
  runtime_id = (unsigned int)time(NULL);
  LV_LOG_USER("Initializing LVGL runtime with ID: %u", runtime_id);

  /* Initialize LVGL */
  lv_init();
  lv_log_register_print_cb(lvrt_log_cb);

  int monitor_hor_res, monitor_ver_res;
  emscripten_get_canvas_element_size(canvas_selector, &monitor_hor_res, &monitor_ver_res);

  SDL_SetHint(SDL_HINT_EMSCRIPTEN_CANVAS_SELECTOR, canvas_selector);
  SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, canvas_selector);
  SDL_EventState(SDL_TEXTINPUT, SDL_DISABLE);
  SDL_EventState(SDL_KEYDOWN, SDL_DISABLE);
  SDL_EventState(SDL_KEYUP, SDL_DISABLE);
  SDL_EventState(SDL_MOUSEWHEEL, SDL_DISABLE);

  lv_display_t *display = lv_sdl_window_create(monitor_hor_res, monitor_ver_res);
  lv_display_set_default(display);

  lv_sdl_window_set_title(display, "LVGL Editor");

  lv_group_t *group = lv_group_get_default();
  lv_group_set_default(lv_group_create());

  lv_indev_t * mouse = lv_sdl_mouse_create();
  lv_indev_set_group(mouse, group);

  /* lv_indev_t * mousewheel = lv_sdl_mousewheel_create();
  lv_indev_set_group(mousewheel, group); */

  /* Unfortunately when using keyboard support with emscripten, the canvas/app seems to capture
   * all keyboard events, and input cannot be used elsewhere in the webview window.
   */

  /*lv_indev_t * keyboard = lv_sdl_keyboard_create();
  lv_indev_set_group(keyboard, group);*/
  
  emscripten_set_main_loop_arg(do_loop, NULL, 0, 0);

  return 0;

}

void do_loop(void* arg){
  lv_task_handler();
}

void lvrt_task_handler(){
  lv_task_handler();
}

int lvrt_process_data(const char *xml_definition, const char *display_style[]){

  lvrt_is_not_known_object = false;

  lv_display_t *display = lv_display_get_default();

  lv_xml_component_unregister("thisview");
  lv_result_t result = lv_xml_component_register_from_data("thisview", xml_definition);

  if(result != LV_RESULT_OK){
    LV_LOG_WARN("Error processing data.");
    LV_LOG_WARN("XML definition: %s", xml_definition);
    return 1;
  }

  lv_obj_t * screen = lv_xml_create(NULL, "lv_obj", display_style);
  lv_obj_t * scr_prev = lv_screen_active();
  lv_screen_load(screen);
  lv_obj_delete(scr_prev);

  lv_obj_t * ui = lv_xml_create(screen, "thisview", NULL);
  
  if(ui == NULL){
    LV_LOG_WARN("Ouch! UI is null.");
    return 1;
  }
  else if (lvrt_is_not_known_object) {
    LV_LOG_WARN("Not a known widget, element, or component");
    
    // Clean screen first
    lv_obj_clean(screen);
    

    lvrt_is_not_known_object = false;
    lvrt_unknown_widget_name[0] = '\0';
    
    return 1;
  }

  lv_refr_now(NULL);

  js_xml_is_rendered();

  return 0;
}


int lvrt_xml_load_component_data(const char *name, const char *xml_definition) {
  
  if(xml_definition==NULL){
    LV_LOG_WARN("Cannot register %s. No data.", name);
    return 1;
  }

  lv_result_t result = lv_xml_component_register_from_data(name, xml_definition);
  if(result != LV_RESULT_OK) {
    LV_LOG_WARN("Failed to register component from data %s", name);
    return 1;
  }

  return 0;
}

EMSCRIPTEN_KEEPALIVE
bool lvrt_get_obj_area(const char* name_path, int* x, int* y, int* width, int* height, int* layout_positioned, int* alignment) {
    if (name_path == NULL || x == NULL || y == NULL || width == NULL || height == NULL || layout_positioned == NULL || alignment == NULL) {
        LV_LOG_WARN("lvrt_get_obj_area: Invalid parameters");
        return false;
    }
    
    lv_obj_t* screen = lv_screen_active();
    lv_obj_t* obj = lv_obj_get_child_by_name(screen, name_path);

    if (obj == NULL) {
        return false;
    }
    
    *x = obj->coords.x1;
    *y = obj->coords.y1;
    *width = obj->coords.x2 - obj->coords.x1 + 1;
    *height = obj->coords.y2 - obj->coords.y1 + 1;

     // Store whether the object is layout positioned
    *layout_positioned = lv_obj_is_layout_positioned(obj) ? 1 : 0;
    *alignment = (int)lv_obj_get_style_align(obj, LV_PART_MAIN);
    
    return true;
}

EMSCRIPTEN_KEEPALIVE
void lvrt_resize_canvas(int width, int height) {
  lv_display_t *display = lv_display_get_default();
  lv_display_set_resolution(display, width, height);
}

EMSCRIPTEN_KEEPALIVE
void lvrt_cleanup_runtime(){
  LV_LOG_USER("Cleaning up runtime with ID: %u", runtime_id);
  
  // Cancel the main loop first
  emscripten_cancel_main_loop();

  // Clean the active screen
  lv_obj_t* screen = lv_screen_active();
  if (screen) {
    lv_obj_clean(screen);
    LV_LOG_USER("Active screen cleaned successfully");
  } else {
    LV_LOG_USER("No active screen to clean");
  }
  
  // Deinitialize LVGL
  lv_deinit();
  
  LV_LOG_USER("Cleaning up runtime is complete ID: %u", runtime_id);
}
