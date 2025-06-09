#include "esp_freertos_hooks.h"
#include "lvgl.h"

#include "./device/ili9341.h"
#include "./device/resistive_touchscreen.h"
#include "gui.h"

void touchpad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    static int32_t last_x = 0;
    static int32_t last_y = 0;

    int x_tmp = read_touch_x();

    if(x_tmp > TOUCH_THRESHOLD_X) {
        last_x = x_tmp;
        last_y = read_touch_y();
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;
}

void IRAM_ATTR lv_tick_task(void)
{
	lv_tick_inc(1);
}

void anim_x_cb(void * var, int32_t v)
{
    lv_obj_set_x(var, v);
}

void anim_size_cb(void * var, int32_t v)
{
    lv_obj_set_size(var, v, v);
}


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

    lv_init();
    
    lv_display_t * disp;
    disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(disp, test_flush);

    size_t buf_size = TFT_HOR_RES * TFT_VER_RES / 10 * lv_color_format_get_size(lv_display_get_color_format(disp));
    lv_color_t *buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    lv_color_t *buf2 = NULL;

    lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);


    // // touchpad
    // indev_touchpad = lv_indev_create();
    // lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    // lv_indev_set_read_cb(indev_touchpad, touchpad_read);

    esp_register_freertos_tick_hook(lv_tick_task);
	lv_example_anim_2();

	while(1) {
		vTaskDelay(20 / portTICK_PERIOD_MS);
		lv_task_handler();
	}

    free(buf1);
    vTaskDelete(NULL);
}