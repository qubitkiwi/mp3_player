#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ili9341.h"
#include "lvgl.h"

void app_main(void)
{
    xTaskCreate(gui_task, "gui_task", 2048, NULL, 5, NULL);
}
