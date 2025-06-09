#pragma once

#define TOUCH_THRESHOLD_X      100
#define TOUCH_THRESHOLD_Y      100

#define TOUCH_X_P_PIN          ADC_CHANNEL_2    // GPIO_NUM_2
#define TOUCH_Y_P_PIN          ADC_CHANNEL_3    // GPIO_NUM_3

#define TOUCH_X_M_PIN          GPIO_NUM_19
#define TOUCH_Y_M_PIN          GPIO_NUM_20

void touch_init();
int read_touch_x();
int read_touch_y();