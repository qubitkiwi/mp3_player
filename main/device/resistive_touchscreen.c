#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#include "resistive_touchscreen.h"

static adc_oneshot_unit_handle_t adc1_handle;
static adc_oneshot_chan_cfg_t adc_config = {
    .bitwidth = ADC_BITWIDTH_DEFAULT,
    .atten = ADC_ATTEN_DB_12,
};

void touch_init()
{
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
}

int read_touch_x()
{
    int result;
    // Y output config
    static gpio_config_t y_io_conf = {
        .intr_type      = GPIO_INTR_DISABLE,
        .mode           = GPIO_MODE_OUTPUT,
        .pin_bit_mask   = (1ULL << TOUCH_Y_P_PIN) | (1ULL << TOUCH_Y_M_PIN),
        .pull_down_en   = 0,
        .pull_up_en     = 0,
    };
    gpio_config(&y_io_conf);

    // X- input config
    static gpio_config_t x_io_conf = {
        .intr_type      = GPIO_INTR_DISABLE,
        .mode           = GPIO_MODE_INPUT,
        .pin_bit_mask   = (1ULL << TOUCH_X_M_PIN),
        .pull_down_en   = 0,
        .pull_up_en     = 0,
    };
    gpio_config(&x_io_conf);

    // Y+ high, Y- low
    gpio_set_level(TOUCH_Y_P_PIN, 1);
    gpio_set_level(TOUCH_Y_M_PIN, 0);


    // X+ config
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, TOUCH_X_P_PIN, &adc_config));

    // X+ ADC
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, TOUCH_X_P_PIN, &result));

    return result;
}

int read_touch_y()
{
    int result;
    // X+ output config, pull up
    static gpio_config_t x_io_conf = {
        .intr_type      = GPIO_INTR_DISABLE,
        .mode           = GPIO_MODE_OUTPUT,
        .pin_bit_mask   = (1ULL << TOUCH_X_P_PIN) | (1ULL << TOUCH_X_M_PIN),
        .pull_down_en   = 0,
        .pull_up_en     = 0,
    };
    gpio_config(&x_io_conf);

    // Y- input config, no pull
    static gpio_config_t y_io_conf = {
        .intr_type      = GPIO_INTR_DISABLE,
        .mode           = GPIO_MODE_INPUT,
        .pin_bit_mask   = (1ULL << TOUCH_Y_M_PIN),
        .pull_down_en   = 0,
        .pull_up_en     = 0,
    };
    gpio_config(&y_io_conf);

    // Y+ high, Y- low
    gpio_set_level(TOUCH_X_P_PIN, 1);
    gpio_set_level(TOUCH_X_M_PIN, 0);


    // Y+ config
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, TOUCH_Y_P_PIN, &adc_config));

    // X+ ADC
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, TOUCH_Y_P_PIN, &result));

    return result;
}