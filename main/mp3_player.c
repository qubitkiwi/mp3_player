#include <stdio.h>
#include "ili9341.h"
#include "lvgl.h"

static void IRAM_ATTR lv_tick_task(void)
{
	lv_tick_inc(portTICK_RATE_MS);
}

void gui_task(void *pvParameter)
{
    lcd_init();

    static lv_disp_buf_t disp_buf;
    // lv_color_t buf1[DISP_BUF_SIZE];
    lv_color_t *buf1 = (lv_color_t *) heap_caps_malloc(display_buffer_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_disp_buf_init(&disp_buf, buf1, NULL, DISP_BUF_SIZE);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = ili9341_flush;
	disp_drv.buffer = &disp_buf;
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 320;
    disp_drv.rotated = LV_DISP_ROT_270;
	lv_disp_drv_register(&disp_drv);


    esp_register_freertos_tick_hook(lv_tick_task);
	demo_create();

	while(1) {
		vTaskDelay(1);
		lv_task_handler();
	}

    free(buf1);
    vTaskDelete(NULL);
}

void app_main(void)
{


}
