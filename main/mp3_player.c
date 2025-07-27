#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gui.h"
#include "file_manager.h"

#include "device/ili9341.h"
#include "driver/spi_common.h"
#include "common.h"
#include "string.h"

struct mp3_state MP3_STATE = {0, };

void app_main(void)
{

    spi_bus_config_t spi_buscfg = {
        .miso_io_num = CONFIG_MP3_SPI_PIN_MISO,
        .mosi_io_num = CONFIG_MP3_SPI_PIN_MOSI,
        .sclk_io_num = CONFIG_MP3_SPI_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092,
    };

    spi_bus_initialize(CONFIG_MP3_SPI_HOST, &spi_buscfg, SPI_DMA_CH_AUTO);

    xTaskCreate(gui_task, "gui_task", 4096 * 2, NULL, 5, NULL);
    // xTaskCreate(file_task, "file_task", 4096, NULL, 5, NULL);
}
