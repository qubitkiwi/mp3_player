#pragma once

#include "driver/spi_master.h"

// GPIO
#define LCD_DC      GPIO_NUM_10
#define LCD_CS      GPIO_NUM_0

// SPI
#define LCD_HOST    SPI2_HOST

#define LCD_CLK     GPIO_NUM_4
#define LCD_MOSI    GPIO_NUM_6
#define LCD_MISO    GPIO_NUM_5

// spi no return cmd
enum LCD_CMD_NO_RETURN {
    NO_OP                   = 0x00,
    
    // sw reset need 5ms wait
    // SW_RESET = 0x01,

    ENTER_SLEEP             = 0x10,
    // SLEEP_OUT need 5ms wait
    SLEEP_OUT               = 0x11,
    PARTIAL_MODE            = 0x12,
    NORMAL_DISPLAY_MODE     = 0x13,
    
    DISPLAY_INVERSION_OFF   = 0x20,
    DISPLAY_INVERSION_ON    = 0x21,

    DISPLAY_OFF             = 0x28,
    DISPLAY_ON              = 0x29,

    TEARING_EFFECT_LINE_OFF = 0x34,
    IDLE_MODE_OFF           = 0x38,
    IDLE_MODE_ON            = 0x39,
};


void lcd_spi_init(void);
void lcd_spi_uninit(void);
void lcd_spi_pre_transfer_callback(spi_transaction_t *t);
void lcd_spi_post_transfer_callback(spi_transaction_t *t);
void lcd_init();

void lcd_cmd(char cmd);
void lcd_tx(char cmd, char *par, int par_len);
void lcd_rx(char cmd, char *rx_data, int rx_len);

//  no return cmd
void lcd_reset();


// get data
int lcd_get_id();
int lcd_get_pixel_format();
char lcd_get_power_mode();


// set data
void lcd_set_pixel_format();
void lcd_set_col_addr(short start_col, short end_col);
void lcd_set_page_addr(short start_page, short end_page);
void lcd_write(char *data, int len);