#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "ili9341.h"
#include "esp_err.h"
#include "esp_log.h"

static spi_device_handle_t lcd_spi;
static char *TAG = "ili9341";

void lcd_spi_init(void)
{
    esp_err_t ret;

    // gpio init
    gpio_config_t spi_wr_conf = {
        .pin_bit_mask    = (1ULL << LCD_DC) | (1ULL << LCD_CS),
        .mode            = GPIO_MODE_OUTPUT,
        .pull_up_en      = GPIO_PULLUP_ENABLE,
        .pull_down_en    = GPIO_PULLDOWN_DISABLE,
        .intr_type       = GPIO_INTR_DISABLE,
    };

    ret = gpio_config(&spi_wr_conf);
    ESP_ERROR_CHECK(ret);

    gpio_set_level(LCD_CS, 1);

    ESP_LOGI(TAG, "gpio init");

    // spi init
    // spi_device_handle_t spi;
    spi_bus_config_t spi_buscfg = {
        .miso_io_num = LCD_MISO,
        .mosi_io_num = LCD_MOSI,
        .sclk_io_num = LCD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 16 * 320 * 2 + 8
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,     //Clock out at 10 MHz
        .mode = 0,                              //SPI mode 0
        .spics_io_num = GPIO_NUM_NC,
        .queue_size = 7,                        //We want to be able to queue 7 transactions at a time
        .pre_cb = lcd_spi_pre_transfer_callback, //Specify pre-transfer callback to handle D/C line
        .post_cb = lcd_spi_post_transfer_callback,
    };

    ret = spi_bus_initialize(LCD_HOST, &spi_buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret = spi_bus_add_device(LCD_HOST, &devcfg, &lcd_spi);
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "spi init");
}

void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    
    int dc = (int)t->user;
    gpio_set_level(LCD_DC, dc);

    gpio_set_level(LCD_CS, 0);
}

void lcd_spi_post_transfer_callback(spi_transaction_t *t)
{
    gpio_set_level(LCD_CS, 1);
}


/* void ili9341_init(void) {
    // 하드웨어 리셋
    ili9341_write_command(0x01); // Software reset
    
    // 전원 제어 1
    ili9341_write_command(0xC0);
    ili9341_write_data(0x23);
    ili9341_write_data(0x10);

    // 전원 제어 2
    ili9341_write_command(0xC1);
    ili9341_write_data(0x10);

    // VCOM 제어 1
    ili9341_write_command(0xC5);
    ili9341_write_data(0x3e);
    ili9341_write_data(0x28);

    // VCOM 제어 2
    ili9341_write_command(0xC7);
    ili9341_write_data(0x86);

    // 메모리 접근 제어
    ili9341_write_command(0x36);
    ili9341_write_data(0x48);

    // 픽셀 형식 설정
    ili9341_write_command(0x3A);
    ili9341_write_data(0x55);

    // 디스플레이 기능 제어
    ili9341_write_command(0xB6);
    ili9341_write_data(0x0A);
    ili9341_write_data(0xA2);

    // 감마 곡선 설정 1
    ili9341_write_command(0xE0);
    ili9341_write_data(0x0F);
    ili9341_write_data(0x31);
    ili9341_write_data(0x2B);
    ili9341_write_data(0x0C);
    ili9341_write_data(0x0E);
    ili9341_write_data(0x08);
    ili9341_write_data(0x4E);
    ili9341_write_data(0xF1);
    ili9341_write_data(0x37);
    ili9341_write_data(0x07);
    ili9341_write_data(0x10);
    ili9341_write_data(0x03);
    ili9341_write_data(0x0E);
    ili9341_write_data(0x09);
    ili9341_write_data(0x00);

    // 감마 곡선 설정 2
    ili9341_write_command(0xE1);
    ili9341_write_data(0x00);
    ili9341_write_data(0x0E);
    ili9341_write_data(0x14);
    ili9341_write_data(0x03);
    ili9341_write_data(0x11);
    ili9341_write_data(0x07);
    ili9341_write_data(0x31);
    ili9341_write_data(0xC1);
    ili9341_write_data(0x48);
    ili9341_write_data(0x08);
    ili9341_write_data(0x0F);
    ili9341_write_data(0x0C);
    ili9341_write_data(0x31);
    ili9341_write_data(0x36);
    ili9341_write_data(0x0F);

    // 디스플레이 켜기
    ili9341_write_command(0x29);
} */
void lcd_init()
{
    char tx[4];
    lcd_reset();


    lcd_set_pixel_format();
    lcd_cmd(SLEEP_OUT);
    vTaskDelay( 5 / portTICK_PERIOD_MS );

    lcd_cmd(DISPLAY_ON);
}

void lcd_cmd(char cmd)
{
    spi_device_acquire_bus(lcd_spi, portMAX_DELAY);

    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    
    // send cmd
    t.length = 8;
    t.tx_buffer = &cmd;
    t.user = (void*)0;

    ret = spi_device_polling_transmit(lcd_spi, &t);
    assert(ret == ESP_OK);
 
    spi_device_release_bus(lcd_spi);
}

void lcd_tx(char cmd, char *par, int par_len)
{
    spi_device_acquire_bus(lcd_spi, portMAX_DELAY);

    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    
    // send cmd
    t.length = 8;
    t.tx_buffer = &cmd;
    t.user = (void*)0;

    ret = spi_device_polling_transmit(lcd_spi, &t);
    assert(ret == ESP_OK);

    if (par_len == 0)
        return ;

    // send Parameter
    memset(&t, 0, sizeof(t));
    t.length = 8 * par_len;
    t.tx_buffer = &par;
    t.user = (void*)1;

    ret = spi_device_polling_transmit(lcd_spi, &t);
    assert(ret == ESP_OK);

    spi_device_release_bus(lcd_spi);
}

void lcd_rx(char cmd, char *rx_data, int rx_len)
{
    spi_device_acquire_bus(lcd_spi, portMAX_DELAY);

    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    
    // send cmd
    t.length = 8;
    t.tx_buffer = &cmd;
    t.user = (void*)0;

    ret = spi_device_polling_transmit(lcd_spi, &t);
    assert(ret == ESP_OK);

    if (rx_len == 0)
        return ;

    // receive data
    memset(&t, 0, sizeof(t));
    t.length = 8 * rx_len;
    t.rx_buffer = &rx_data;
    t.user = (void*)1;

    ret = spi_device_polling_transmit(lcd_spi, &t);
    assert(ret == ESP_OK);

    spi_device_release_bus(lcd_spi);
}


//// no return data

void lcd_reset()
{
    // sw reset
    lcd_cmd(0x1);

    // 5ms wait
    vTaskDelay( 5 / portTICK_PERIOD_MS );
}



//// get data


int lcd_get_id()
{
    char rx_data[4];
    lcd_rx(0x01, rx_data, 4);

    ESP_LOGI(TAG, "LCD module’s manufacturer ID %x", rx_data[1]);
    ESP_LOGI(TAG, "LCD module/driver version ID %x", rx_data[2]);
    ESP_LOGI(TAG, "LCD module/driver ID %x", rx_data[3]);

    return ( *((int*) rx_data) & 0xFFFFFFFF);
}


int lcd_get_pixel_format()
{
    char rx_data[2];
    lcd_rx(0x0c, rx_data, 2);
    
    // RGB Interface Format 
    

    // MCU Interface Format
    if (rx_data[1] & 0b00000101) {
        ESP_LOGI(TAG, "MCU Interface Format : 16 bit/pixel");
    } else if (rx_data[1] & 0b00000110) {
        ESP_LOGI(TAG, "MCU Interface Format : 18 bit/pixel");
    } else {
        ESP_LOGW(TAG, "MCU Interface Format : unknown 0x%x", rx_data[1]);
    }

    return rx_data[1];
}

char lcd_get_power_mode()
{
    char rx[2];
    lcd_rx(0x0a, rx, 2);
    ESP_LOGI(TAG, "Read Display Power Mode %x", rx[1]);

    return rx[1];
}






// // set data



// Column Address Set
void lcd_set_col_addr(short start_col, short end_col)
{
    char parameter[4];
    
    parameter[0] = (start_col & 0xff00) >> 8;
    parameter[1] = start_col & 0x00ff;
    parameter[2] = (end_col & 0xff00) >> 8;
    parameter[3] = end_col & 0x00ff;

    lcd_tx(0x2A, parameter, 4);
}

// Page Address Set
void lcd_set_page_addr(short start_page, short end_page)
{
    char parameter[4];

    parameter[0] = (start_page & 0xff00) >> 8;
    parameter[1] = start_page & 0x00ff;
    parameter[2] = (end_page & 0xff00) >> 8;
    parameter[3] = end_page & 0x00ff;

    lcd_tx(0x2B, parameter, 4);
}


void lcd_set_pixel_format()
{
    // 16 bit / pixel
    // DPI = 0x05, DBI = 0x05
    char par = 0x55;
    lcd_tx(0x3a ,&par, 1);
}



// todo
void lcd_write(char *data, int len)
{
    lcd_tx(0x2e, data, len);
    
    // send memory
    // spi_transaction_t t;
    // memset(&t, 0, sizeof(t));
    // t.length = 8 * len;
    // t.tx_buffer = data;
    // t.user = (void*)1;
    // t.flags = 0;

    // spi_device_queue_trans(spi, &t, portMAX_DELAY);

}