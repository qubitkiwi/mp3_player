#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gui.h"

void app_main(void)
{
    xTaskCreate(gui_task, "gui_task", 4096, NULL, 5, NULL);
}
