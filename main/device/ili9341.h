#pragma once

#include "driver/spi_master.h"
#include "lvgl.h"

// GPIO
#define LCD_DC      CONFIG_ILI9341_SPI_PIN_DC

// SPI
#define LCD_HOST    CONFIG_MP3_SPI_HOST

#define LCD_CLK     CONFIG_MP3_SPI_PIN_CLK
#define LCD_MOSI    CONFIG_MP3_SPI_PIN_MOSI
#define LCD_MISO    CONFIG_MP3_SPI_PIN_MISO
#define LCD_CS      CONFIG_ILI9341_SPI_PIN_CS


#define TFT_HOR_RES   320
#define TFT_VER_RES   240


typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes;
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
void lcd_dma_data(const uint8_t *data, int data_len);
void lcd_spi_pre_transfer_callback(spi_transaction_t *t);
uint32_t lcd_get_id();

void lcd_init();
void ili9341_flush( lv_display_t *disp, const lv_area_t *area, uint8_t * px_map);