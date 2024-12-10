/**
 * @file slider_box_gen.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "slider_box_private_gen.h"
#include "lvgl/src/core/lv_obj_class_private.h"
#include "ui.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  GLOBAL PROTOTYPES
 **********************/

void slider_box_constructor_hook(lv_obj_t * obj);
void slider_box_destructor_hook(lv_obj_t * obj);
void slider_box_event_hook(lv_event_t * e);

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void slider_box_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void slider_box_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void slider_box_event(const lv_obj_class_t * class_p, lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t slider_box_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = slider_box_constructor,
    .destructor_cb = slider_box_destructor,
    .event_cb = slider_box_event,
    .instance_size = sizeof(slider_box_t),
    .editable = 1,
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/


lv_obj_t * slider_box_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(&slider_box_class, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/


static void slider_box_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    
    static lv_style_t style_main;
    static bool style_main_inited = false;

    if(!style_main_inited) {
        lv_style_init(&style_main);
                lv_style_set_bg_color(&style_main, lv_color_hex(0x666666));
        lv_style_set_bg_opa(&style_main, 255);
        lv_style_set_border_width(&style_main, 2);
        lv_style_set_border_color(&style_main, lv_color_hex(0xaaaaaa));
        lv_style_set_radius(&style_main, 20);
        lv_style_set_width(&style_main, 250);
        lv_style_set_height(&style_main, 100);
        lv_style_set_pad_all(&style_main, 10);
        style_main_inited = true;
    }

    lv_obj_add_style(obj, &style_main, LV_PART_MAIN | LV_STATE_DEFAULT);

    static lv_style_t style_transp_cont;
    static bool style_transp_cont_inited = false;

    if(!style_transp_cont_inited) {
        lv_style_init(&style_transp_cont);
                lv_style_set_bg_opa(&style_transp_cont, 0);
        lv_style_set_border_width(&style_transp_cont, 2);
        style_transp_cont_inited = true;
    }

    lv_obj_add_style(obj, &style_transp_cont, LV_PART_MAIN | LV_STATE_DEFAULT);

    static lv_style_t style_button;
    static bool style_button_inited = false;

    if(!style_button_inited) {
        lv_style_init(&style_button);
                lv_style_set_radius(&style_button, 100);
        style_button_inited = true;
    }

    lv_obj_add_style(obj, &style_button, LV_PART_MAIN | LV_STATE_DEFAULT);

    slider_box_t * slider_box = (slider_box_t *)obj;
    
    slider_box->lv_label = lv_label_create(obj);
    lv_label_set_text(slider_box->lv_label, "$title");
    lv_label_set_long_mode(slider_box->lv_label, LV_LABEL_LONG_MODE_SCROLL);

    slider_box->lv_obj = lv_obj_create(obj);

    slider_box->lv_button = lv_button_create(slider_box->lv_obj);

    slider_box->lv_label2 = lv_label_create(slider_box->lv_button);
    lv_label_set_text(slider_box->lv_label2, "-");

    slider_box->lv_button2 = lv_button_create(slider_box->lv_obj);

    slider_box->lv_label3 = lv_label_create(slider_box->lv_button2);
    lv_label_set_text(slider_box->lv_label3, "+");

    slider_box->dark_slider = dark_slider_create(slider_box->lv_obj);


    slider_box_constructor_hook(obj);

    LV_TRACE_OBJ_CREATE("finished");
}

static void slider_box_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);

    slider_box_destructor_hook(obj);
}

static void slider_box_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_result_t res;

    /* Call the ancestor's event handler */
    res = lv_obj_event_base(&slider_box_class, e);
    if(res != LV_RESULT_OK) return;

    slider_box_event_hook(e);
}