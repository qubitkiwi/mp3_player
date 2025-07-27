#include "string.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"

#include "./device/ili9341.h"
#include "./device/resistive_touchscreen.h"
#include "device/sdspi.h"
#include "file_manager.h"
#include "gui.h"

#include "common.h"

extern struct mp3_state MP3_STATE;

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





void play_music(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    int num = (int) lv_event_get_user_data(e);
    if(code == LV_EVENT_CLICKED) {
        char file_path[300];
        snprintf(file_path, sizeof(file_path), "%s%s", MOUNT_POINT, MP3_STATE.list[num]);

        set_music_file(file_path);
        set_wav_cfg();
        MP3_STATE.is_play = true;
    }

}

void sd_cb(lv_event_t * e)
{
    static int cnt = 0;
    lv_obj_t * sw = lv_event_get_target(e);
    lv_obj_t * label = lv_event_get_user_data(e);

    cnt++;
    // sd card mount
    if(lv_obj_has_state(sw, LV_STATE_CHECKED)) {
        esp_err_t ret = sd_mount();
        if (ret == ESP_OK) { 
            lv_obj_add_state(sw, LV_STATE_CHECKED);
            update_mp3_list(MOUNT_POINT);
            lv_label_set_text_fmt(label, "sd unmount");
            return ;
        } else {
            lv_obj_clear_state(sw, LV_STATE_CHECKED);
            lv_label_set_text_fmt(label, "sd mount try: %d", cnt);
            return ;
        }

    } else {
        // sd card unmount
        sd_unmount();
        cnt = 0;
        return ;
    }

}


void update_list(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    lv_obj_t * playlist = (int) lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED) {

        for (int i = 0; i < MP3_STATE.max_list; i++) {
            lv_obj_t *btn = lv_list_add_button(playlist, LV_SYMBOL_PLAY, MP3_STATE.list[i]);
            lv_obj_add_event_cb(btn, play_music, LV_EVENT_CLICKED, (void*) i);
        }
    }
}

void create_main_screen(void)
{
    lv_obj_t *root = lv_obj_create(NULL);
    lv_scr_load(root);

    lv_obj_set_size(root, 320, 240);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    lv_obj_t *btn_container = lv_obj_create(root);
    lv_obj_set_width(btn_container, lv_pct(100));
    lv_obj_set_layout(btn_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);

    lv_obj_t *label = lv_label_create(btn_container);
    lv_label_set_text(label, "SD Card Mount");

    lv_obj_t *sw = lv_switch_create(btn_container);
    lv_obj_add_event_cb(sw, sd_cb, LV_EVENT_VALUE_CHANGED, label);

    // play list update button
    lv_obj_t *update_btn = lv_btn_create(btn_container);
    lv_obj_set_flex_grow(update_btn, 1);
    lv_obj_t *update_label = lv_label_create(update_btn);
    lv_label_set_text(update_label, "Update Play List");

    lv_obj_t *playlist;
    playlist = lv_list_create(root);
    
    lv_obj_add_event_cb(update_btn, update_list, LV_EVENT_CLICKED, (void*) playlist);
    
    lv_obj_set_height(playlist, 200);
    lv_list_add_text(playlist, "play list");

    for (int i = 0; i < MP3_STATE.max_list; i++) {
        lv_obj_t *btn = lv_list_add_button(playlist, LV_SYMBOL_PLAY, MP3_STATE.list[i]);
        lv_obj_add_event_cb(btn, play_music, LV_EVENT_CLICKED, (void*) i);
    }

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


    // touchpad
    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read);

    // lv_tick periodic setup
    const esp_timer_create_args_t lv_tick_timer_args = {
            .callback = &lv_tick_task,
            .name = "lv_tick"
    };
    esp_timer_handle_t lv_tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&lv_tick_timer_args, &lv_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lv_tick_timer, 5 * 1000)); // 5ms


    create_main_screen();
    
    printf("gui free heap size: %ld\n", esp_get_free_heap_size());

    while(1) {
        lv_task_handler();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    ESP_ERROR_CHECK(esp_timer_stop(lv_tick_timer));
    ESP_ERROR_CHECK(esp_timer_delete(lv_tick_timer));
    free(buf1);
    vTaskDelete(NULL);
}