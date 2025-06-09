#pragma once

#include "stdint.h"

void IRAM_ATTR lv_tick_task(void);

void anim_x_cb(void * var, int32_t v);
void anim_size_cb(void * var, int32_t v);
void lv_example_anim_2(void);
void gui_task(void *pvParameter);