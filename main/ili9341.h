#pragma once

#include "driver/spi_master.h"
#include "lvgl.h"

// GPIO
#define LCD_DC      GPIO_NUM_10

// SPI
#define LCD_HOST    SPI2_HOST

#define LCD_CLK     GPIO_NUM_4
#define LCD_MOSI    GPIO_NUM_6
#define LCD_MISO    GPIO_NUM_5
#define LCD_CS      GPIO_NUM_0

//To speed up transfers, every SPI transfer sends a bunch of lines. This define specifies how many. More means more memory use,
//but less overhead for setting up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16

/*
 The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct.
*/
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;


DRAM_ATTR static const lcd_init_cmd_t ili_init_cmds[] = {

    /* Memory access contorl, MX=MY=0, MV=1, ML=0, BGR=1, MH=0 */
    {0x36, {0x28}, 1},
    /* Pixel format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x55}, 1},
    /* Entry mode set, Low vol detect disabled, normal display */
    {0xB7, {0x07}, 1},
    /* Sleep out */
    {0x11, {0}, 0x80},
    /* Display on */
    {0x29, {0}, 0x80},
    {0, {0}, 0xff},
};

void lcd_cmd(const uint8_t cmd, bool keep_cs_active);
void lcd_data(const uint8_t *data, int len);
void lcd_spi_pre_transfer_callback(spi_transaction_t *t);
uint32_t lcd_get_id();
void lcd_init();
void send_lines(int ypos, uint16_t *linedata);
void send_line_finish();
void ili9341_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map);