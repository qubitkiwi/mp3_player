#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "ili9341.h"

static spi_device_handle_t lcd_spi;


void lcd_cmd(const uint8_t cmd, bool keep_cs_active)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = 8;                   //Command is 8 bits
    t.tx_buffer = &cmd;             //The data is the cmd itself
    t.user = (void*)0;              //D/C needs to be set to 0
    if (keep_cs_active) {
        t.flags = SPI_TRANS_CS_KEEP_ACTIVE;   //Keep CS active after data transfer
    }
    ret = spi_device_polling_transmit(lcd_spi, &t); //Transmit!
    assert(ret == ESP_OK);          //Should have had no issues.
}


void lcd_data(const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len == 0) {
        return;    //no need to send anything
    }
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = len * 8;             //Len is in bytes, transaction length is in bits.
    t.tx_buffer = data;             //Data
    t.user = (void*)1;              //D/C needs to be set to 1
    ret = spi_device_polling_transmit(lcd_spi, &t); //Transmit!
    assert(ret == ESP_OK);          //Should have had no issues.
}


void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(LCD_DC, dc);
}

uint32_t lcd_get_id()
{
    // When using SPI_TRANS_CS_KEEP_ACTIVE, bus must be locked/acquired
    spi_device_acquire_bus(lcd_spi, portMAX_DELAY);

    //get_id cmd
    lcd_cmd(0x04, true);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8 * 3;
    t.flags = SPI_TRANS_USE_RXDATA;
    t.user = (void*)1;

    esp_err_t ret = spi_device_polling_transmit(lcd_spi, &t);
    assert(ret == ESP_OK);

    // Release bus
    spi_device_release_bus(lcd_spi);

    printf("LCD module’s manufacturer ID 0x %lx\n", *(uint32_t*)t.rx_data);

    return *(uint32_t*)t.rx_data;
}

//Initialize the display
void lcd_init()
{
    // spi init
    esp_err_t ret;
    // spi_device_handle_t spi;
    spi_bus_config_t buscfg = {
        .miso_io_num = LCD_MISO,
        .mosi_io_num = LCD_MOSI,
        .sclk_io_num = LCD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = PARALLEL_LINES * 320 * 2 + 8
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,     //Clock out at 10 MHz
        .mode = 0,                              //SPI mode 0
        .spics_io_num = LCD_CS,             //CS pin
        .queue_size = 7,                        //We want to be able to queue 7 transactions at a time
        .pre_cb = lcd_spi_pre_transfer_callback, //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret = spi_bus_add_device(LCD_HOST, &devcfg, &lcd_spi);
    ESP_ERROR_CHECK(ret);


    //Initialize non-SPI GPIOs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LCD_DC),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = true,
    };
    gpio_config(&io_conf);

    //detect LCD type
    uint32_t lcd_id = lcd_get_id();
    printf("LCD ID: %08"PRIx32"\n", lcd_id);

    //Send all the commands
    int cmd = 0;
    printf("LCD ILI9341 initialization.\n");
    while (ili_init_cmds[cmd].databytes != 0xff) {
        lcd_cmd(ili_init_cmds[cmd].cmd, false);
        lcd_data(ili_init_cmds[cmd].data, ili_init_cmds[cmd].databytes & 0x1F);
        if (ili_init_cmds[cmd].databytes & 0x80) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        cmd++;
    }
}

/* To send a set of lines we have to send a command, 2 data bytes, another command, 2 more data bytes and another command
 * before sending the line data itself; a total of 6 transactions. (We can't put all of this in just one transaction
 * because the D/C line needs to be toggled in the middle.)
 * This routine queues these commands up as interrupt transactions so they get
 * sent faster (compared to calling spi_device_transmit several times), and at
 * the mean while the lines for next transactions can get calculated.
 */
void send_lines(int ypos, uint16_t *linedata)
{
    esp_err_t ret;
    int x;
    //Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
    //function is finished because the SPI driver needs access to it even while we're already calculating the next line.
    static spi_transaction_t trans[6];

    //In theory, it's better to initialize trans and data only once and hang on to the initialized
    //variables. We allocate them on the stack, so we need to re-init them each call.
    for (x = 0; x < 6; x++) {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x & 1) == 0) {
            //Even transfers are commands
            trans[x].length = 8;
            trans[x].user = (void*)0;
        } else {
            //Odd transfers are data
            trans[x].length = 8 * 4;
            trans[x].user = (void*)1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0] = 0x2A;         //Column Address Set
    trans[1].tx_data[0] = 0;            //Start Col High
    trans[1].tx_data[1] = 0;            //Start Col Low
    trans[1].tx_data[2] = (320 - 1) >> 8;   //End Col High
    trans[1].tx_data[3] = (320 - 1) & 0xff; //End Col Low
    trans[2].tx_data[0] = 0x2B;         //Page address set
    trans[3].tx_data[0] = ypos >> 8;    //Start page high
    trans[3].tx_data[1] = ypos & 0xff;  //start page low
    trans[3].tx_data[2] = (ypos + PARALLEL_LINES - 1) >> 8; //end page high
    trans[3].tx_data[3] = (ypos + PARALLEL_LINES - 1) & 0xff; //end page low
    trans[4].tx_data[0] = 0x2C;         //memory write
    trans[5].tx_buffer = linedata;      //finally send the line data
    trans[5].length = 320 * 2 * 8 * PARALLEL_LINES;  //Data length, in bits
    trans[5].flags = 0; //undo SPI_TRANS_USE_TXDATA flag

    //Queue all transactions.
    for (x = 0; x < 6; x++) {
        ret = spi_device_queue_trans(lcd_spi, &trans[x], portMAX_DELAY);
        assert(ret == ESP_OK);
    }

    //When we are here, the SPI driver is busy (in the background) getting the transactions sent. That happens
    //mostly using DMA, so the CPU doesn't have much to do here. We're not going to wait for the transaction to
    //finish because we may as well spend the time calculating the next line. When that is done, we can call
    //send_line_finish, which will wait for the transfers to be done and check their status.
}

void send_line_finish()
{
    spi_transaction_t *rtrans;
    esp_err_t ret;
    //Wait for all 6 transactions to be done and get back the results.
    for (int x = 0; x < 6; x++) {
        ret = spi_device_get_trans_result(lcd_spi, &rtrans, portMAX_DELAY);
        assert(ret == ESP_OK);
        //We could inspect rtrans now if we received any info back. The LCD is treated as write-only, though.
    }
}


void ili9341_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map)
{
    esp_err_t ret;
    int x;
    //Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
    //function is finished because the SPI driver needs access to it even while we're already calculating the next line.
    static spi_transaction_t trans[6];

    //In theory, it's better to initialize trans and data only once and hang on to the initialized
    //variables. We allocate them on the stack, so we need to re-init them each call.
    for (x = 0; x < 6; x++) {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x & 1) == 0) {
            //Even transfers are commands
            trans[x].length = 8;
            trans[x].user = (void*)0;
        } else {
            //Odd transfers are data
            trans[x].length = 8 * 4;
            trans[x].user = (void*)1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0] = 0x2A;         //Column Address Set
    trans[1].tx_data[0] = area->x1 >> 8;            //Start Col High
    trans[1].tx_data[1] = area->x1 & 0xff;            //Start Col Low
    trans[1].tx_data[2] = area->x2 >> 8 ;   //End Col High
    trans[1].tx_data[3] = area->x2 & 0xff; //End Col Low
    trans[2].tx_data[0] = 0x2B;         //Page address set
    trans[3].tx_data[0] = area->y1 >> 8;    //Start page high
    trans[3].tx_data[1] = area->y1 & 0xff;  //start page low
    trans[3].tx_data[2] = area->y2 >> 8; //end page high
    trans[3].tx_data[3] = area->y2 & 0xff; //end page low
    trans[4].tx_data[0] = 0x2C;         //memory write
    trans[5].tx_buffer = color_map;      //finally send the line data
    trans[5].length = lv_area_get_width(area) * lv_area_get_height(area) * 2;  //Data length, in bits
    trans[5].flags = 0; //undo SPI_TRANS_USE_TXDATA flag

    //Queue all transactions.
    for (x = 0; x < 6; x++) {
        ret = spi_device_queue_trans(lcd_spi, &trans[x], portMAX_DELAY);
        assert(ret == ESP_OK);
    }
}