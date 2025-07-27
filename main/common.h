#pragma once

#include "stdint.h"

#define MAX_NAME    256
#define MAX_LIST    20


struct mp3_state {
    bool is_play;
    size_t play_time;
    int hz;
    int chenel;
    char directory[256];
    int play_num;
    int max_list;
    char list[MAX_LIST][MAX_NAME];
};