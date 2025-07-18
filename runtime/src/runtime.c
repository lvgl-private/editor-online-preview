#include "runtime.h"
#include "ui.h"
#include <dirent.h>
#include <emscripten/html5.h>
#include <lvgl/lvgl.h>
#include <lvgl/src/lvgl_private.h>
#include <time.h>


#define SDL_HINT_EMSCRIPTEN_CANVAS_SELECTOR "SDL_EMSCRIPTEN_CANVAS_SELECTOR"

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

// Add these EM_JS declarations at the top with other EM_JS functions
EM_JS(void, js_dispatch_subject_event_int, (const char* name, int32_t value), {
  if (typeof window !== 'undefined' && window.previewStore) {
    const {setSubject} = window.previewStore.getState();
    setSubject({
      name: UTF8ToString(name),
      type: 'int',
      value: value
    });
  }
});

EM_JS(void, js_dispatch_subject_event_string, (const char* name, const char* value), {
  if (typeof window !== 'undefined' && window.previewStore) {
    const {setSubject} = window.previewStore.getState();
    setSubject({
      name: UTF8ToString(name),
      type: 'string',
      value: UTF8ToString(value)
    });
  }
});

void lvrt_log_cb(int8_t level, const char* buf)
{
    js_log_callback(buf);
}

// Add at the top with other globals
static lv_obj_t* g_fullscreen_obj = NULL;

static unsigned int runtime_id = 0;

int lvrt_initialize(const char *canvas_selector){
  
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

  lv_indev_t *mouse = lv_sdl_mouse_create();

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

int lvrt_process_data(const char *xml_definition, const char *name, const char *display_style[], const char *xml_type, const char *language){

  lv_display_t *display = lv_display_get_default();

  lv_xml_component_unregister("thisview");


  lv_result_t result = lv_xml_component_register_from_data("thisview", xml_definition);


  if(result != LV_RESULT_OK){
    LV_LOG_WARN("Error processing data.");
    LV_LOG_WARN("XML definition: %s", xml_definition);
    return 1;
  }

  lv_translation_set_language(language);

  if (lv_streq(xml_type, "test")) {
    lv_xml_component_scope_t * scope = lv_xml_component_get_scope("thisview");
    lv_xml_component_scope_t * extends_scope = lv_xml_component_get_scope(scope->extends);

    if(extends_scope && extends_scope->is_screen) {
      xml_type = "screen";
      name = scope->extends;
    }
}

  lv_obj_t * screen;
  lv_obj_t * ui;

  lv_obj_t * scr_prev = lv_screen_active();
  
  /*Delete the already created permanent screen as we want use the
   current xml_definition not the earlier registered one*/
  lv_obj_t * scr_permanent_prev = NULL;
  if(lv_streq(xml_type, "screen") && name) {
	  scr_permanent_prev = lv_display_get_screen_by_name(NULL, name);
  }
  
  if(lv_streq(xml_type, "screen")) {
  	screen = lv_xml_create(NULL, "thisview", display_style);
    if (name) {
      lv_obj_set_name(screen, name);
    }
    ui = screen;
  } else {
    screen = lv_xml_create(NULL, "lv_obj", display_style);
    ui = lv_xml_create(screen, "thisview", NULL);
  }

  lv_screen_load(screen);

  if(scr_permanent_prev != scr_prev && scr_permanent_prev != NULL) {
    lv_obj_delete(scr_permanent_prev);
  }
  lv_obj_delete(scr_prev);
  
  if(ui == NULL){
    LV_LOG_WARN("Ouch! UI is null.");
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
int lvrt_component_create(const char *name) {
  lv_obj_t * component = lv_xml_create(NULL, name, NULL);
  return component? 0 : 1;
}

EMSCRIPTEN_KEEPALIVE
int lvrt_xml_load_translations(const char *translations_path){
  lv_result_t result = lv_xml_translation_register_from_file(translations_path);
  if(result != LV_RESULT_OK) {
    LV_LOG_WARN("Failed to register translations from file %s", translations_path);
    return 1;
  }
  return 0;
}

EMSCRIPTEN_KEEPALIVE
void lvrt_translation_set_language(const char *language){
  lv_translation_set_language(language);
  lv_obj_t* screen = lv_screen_active();
  lv_obj_clean(screen);
  lv_xml_create(screen, "thisview", NULL);  // We don't need to pass styles here since the component is already registered with its styles
}

int32_t editor_obj_get_click_area(const lv_obj_t * obj)
{
    if(obj->spec_attr == NULL)  return 0;
    return obj->spec_attr->ext_click_pad;
}

EMSCRIPTEN_KEEPALIVE
const char* lvrt_get_view_type(const char* root_element_name)
{
	/*Returning const char * local variables is not safe as they can be only in the stack*/
	static const char * component_str = "component";
	static const char * screen_str = "screen";

	/*Decide if a test extends a screen or a component*/
	if (lv_streq(root_element_name, "test")) {
		lv_xml_component_scope_t * scope = lv_xml_component_get_scope("thisview");
		if(scope == NULL) return component_str; /*Fallback if the XMLs are not loaded yet*/

		lv_xml_component_scope_t * extends_scope = lv_xml_component_get_scope(scope->extends);
		if(extends_scope && extends_scope->is_screen) return screen_str;
		else return component_str;
	} else {
		return component_str;
	}
}

EMSCRIPTEN_KEEPALIVE
bool lvrt_get_obj_area(const char* name_path, int* x, int* y, int* width, int* height, int* layout_positioned, int* alignment, int* pad_top, int* pad_right, int* pad_bottom, int* pad_left, int* margin_top, int* margin_right, int* margin_bottom, int* margin_left, int* hitarea) {
    if (name_path == NULL || x == NULL || y == NULL || width == NULL || height == NULL || layout_positioned == NULL || alignment == NULL || pad_top == NULL || pad_right == NULL || pad_bottom == NULL || pad_left == NULL || margin_top == NULL || margin_right == NULL || margin_bottom == NULL || margin_left == NULL || hitarea == NULL) {
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

    *pad_top = (int)lv_obj_get_style_pad_top(obj, LV_PART_MAIN);
    *pad_right = (int)lv_obj_get_style_pad_right(obj, LV_PART_MAIN);
    *pad_bottom = (int)lv_obj_get_style_pad_bottom(obj, LV_PART_MAIN);
    *pad_left = (int)lv_obj_get_style_pad_left(obj, LV_PART_MAIN);
    
    *margin_top = (int)lv_obj_get_style_margin_top(obj, LV_PART_MAIN);
    *margin_right = (int)lv_obj_get_style_margin_right(obj, LV_PART_MAIN);
    *margin_bottom = (int)lv_obj_get_style_margin_bottom(obj, LV_PART_MAIN);
    *margin_left = (int)lv_obj_get_style_margin_left(obj, LV_PART_MAIN);

    *hitarea = editor_obj_get_click_area(obj);
    
    return true;
}

EMSCRIPTEN_KEEPALIVE
void lvrt_set_subject_int(const char* name, int32_t v) {
  lv_subject_t* subject =  lv_xml_get_subject(NULL, name);
  lv_subject_set_int(subject, v);
}

EMSCRIPTEN_KEEPALIVE
void lvrt_set_subject_string(const char* name, const char* v) {
  lv_subject_t* subject =  lv_xml_get_subject(NULL, name);
  lv_subject_copy_string(subject, v);
}

EMSCRIPTEN_KEEPALIVE
void lvrt_set_subject_float(const char* name, float v) {
  lv_subject_t* subject =  lv_xml_get_subject(NULL, name);
  lv_subject_set_float(subject, v);
}

// Observer callback that fires a JS event
static void subject_changed_cb(lv_observer_t* observer, lv_subject_t* subject) {
    const char* name = lv_observer_get_user_data(observer);
    if (subject->type == LV_SUBJECT_TYPE_STRING) {
        const char* str_value = lv_subject_get_string(subject);
        js_dispatch_subject_event_string(name, str_value);
    } else if (subject->type == LV_SUBJECT_TYPE_INT) {
        int32_t int_value = lv_subject_get_int(subject);
        js_dispatch_subject_event_int(name, int_value);
    }
}

EMSCRIPTEN_KEEPALIVE
bool lvrt_subscribe_subject(const char* name) {
    lv_subject_t* subject = lv_xml_get_subject(NULL, name);
    if (!subject) {
        return false;
    }
    
    // Create observer for the subject
    lv_observer_t* observer = lv_subject_add_observer(subject, subject_changed_cb, lv_strdup(name));
    if (!observer) {
        return false;
    }
    
    return true;
}

EMSCRIPTEN_KEEPALIVE
void lvrt_resize_canvas(int width, int height) {
  lv_display_t *display = lv_display_get_default();
  lv_display_set_resolution(display, width, height);
  lv_refr_now(NULL);
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


EMSCRIPTEN_KEEPALIVE
int lvrt_xml_test_register_from_data(const char *xml_definition, const char * ref_image_path_prefix){

  lv_result_t result = lv_xml_test_register_from_data(xml_definition, ref_image_path_prefix);

  if(result != LV_RESULT_OK){
    LV_LOG_WARN("Error processing test data.");
    LV_LOG_WARN("XML definition: %s", xml_definition);
    return 1;
  }

  return 0;
}

EMSCRIPTEN_KEEPALIVE
void lvrt_xml_test_run_init(){
  lv_xml_test_run_init();
}

EMSCRIPTEN_KEEPALIVE
bool lvrt_xml_test_run_next(uint32_t slowdown){
  return lv_xml_test_run_next(slowdown);
}

EMSCRIPTEN_KEEPALIVE
void lvrt_xml_test_run_stop(){
  lv_xml_test_run_stop();
}