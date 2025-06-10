#include "esp_freertos_hooks.h"
#include "lvgl.h"

#include "./device/ili9341.h"
#include "./device/resistive_touchscreen.h"
#include "gui.h"

static lv_display_t * lcd_disp;
static lv_indev_t *indev_touchpad;

void touchpad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    static int32_t last_x = 0;
    static int32_t last_y = 0;


    int x_tmp = read_touch_x();
    int y_tmp = read_touch_y();

    if(x_tmp > 410) {
        // x max = 3010, x min = 440
        last_x = ((x_tmp - 440) * TFT_HOR_RES) / (3010 - 440) ;
        // y max = 2800, min = 510
        last_y = ((y_tmp - 510) * TFT_VER_RES) / (2800 - 510);
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    printf("raw x %d, raw y %d, x %ld y %ld\n", x_tmp, y_tmp, last_x, last_y);

    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;
}

void IRAM_ATTR lv_tick_task(void)
{
	lv_tick_inc(1);
}

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;

        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
}

/**
 * Create a button with a label and react on click event.
 */
void lv_example_get_started_2(void)
{
    lv_obj_t * btn = lv_button_create(lv_screen_active());     /*Add a button the current screen*/
    lv_obj_set_align(btn, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

    lv_obj_t * label = lv_label_create(btn);          /*Add a label to the button*/
    lv_label_set_text(label, "Button");                     /*Set the labels text*/
    lv_obj_center(label);
}

static void anim_x_cb(void * var, int32_t v)
{
    lv_obj_set_x(var, v);
}

static void anim_size_cb(void * var, int32_t v)
{
    lv_obj_set_size(var, v, v);
}

/**
 * Create a playback animation
 */
void lv_example_anim_2(void)
{

    lv_obj_t * obj = lv_obj_create(lv_screen_active());
    lv_obj_set_style_bg_color(obj, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);

    lv_obj_align(obj, LV_ALIGN_LEFT_MID, 10, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 10, 50);
    lv_anim_set_duration(&a, 1000);
    lv_anim_set_playback_delay(&a, 100);
    lv_anim_set_playback_duration(&a, 300);
    lv_anim_set_repeat_delay(&a, 500);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);

    lv_anim_set_exec_cb(&a, anim_size_cb);
    lv_anim_start(&a);
    lv_anim_set_exec_cb(&a, anim_x_cb);
    lv_anim_set_values(&a, 10, 240);
    lv_anim_start(&a);
}

void gui_task(void *pvParameter)
{
    lcd_init();
    touch_init();
    
    lv_init();
        
    lcd_disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(lcd_disp, ili9341_flush);
    
    size_t buf_size = TFT_HOR_RES * TFT_VER_RES / 10 * lv_color_format_get_size(lv_display_get_color_format(lcd_disp));
    uint8_t *buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    uint8_t *buf2 = NULL;
    lv_display_set_buffers(lcd_disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);


    // // touchpad
    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read);

    esp_register_freertos_tick_hook(lv_tick_task);
	lv_example_get_started_2();
    // lv_example_anim_2();

	while(1) {
		lv_task_handler();
		vTaskDelay(20 / portTICK_PERIOD_MS);
	}

    free(buf1);
    vTaskDelete(NULL);
}