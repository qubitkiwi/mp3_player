#include <stdio.h>
#include <ili9341.h>

void app_main(void)
{
    lcd_spi_init();
    lcd_init();
    int lcd_id = lcd_get_id();
    printf("lcd id %x\n", lcd_id);

    short data[100];
    for (int i=0; i<100; ++i) {
        data[i] = 0x70;
    }
    

    while (1) {
        lcd_get_pixel_format();

        printf("1\n");
        lcd_set_col_addr(0, 10);
        printf("2\n");
        lcd_set_page_addr(0, 10);
        printf("3\n");
        lcd_write(data, 100 * 2);
        printf("4\n");

        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }

}
