#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "stdio.h"
#include "string.h"
#include "esp_log.h"
#include "freertos/queue.h"

#include "file_manager.h"
#include "device/sdspi.h"

#include "dirent.h"
#include "sys/types.h"
#include "sys/stat.h"

#include "common.h"

#include "device/audio.h"

extern struct mp3_state MP3_STATE;
static char TAG[] = "FILE MANAGER";



int update_mp3_list(char *path)
{
    DIR *dp;
    struct dirent *dirt;
    struct stat dir;

    if((dp = opendir(path)) == NULL) {
        ESP_LOGE(TAG, "opendir err");
        return -1;
  	}

    int cnt = 0;
    while ((dirt = readdir(dp)) != NULL) {        
        if (dirt->d_type != 1) continue;
        strcpy(MP3_STATE.list[cnt], dirt->d_name);
        cnt++;
        printf("sd card find file: %s, %d\n", dirt->d_name, dirt->d_type);
    }
    MP3_STATE.max_list = cnt;

    closedir(dp);

    return 0;
}


#define BUF_SIZE 1024

static FILE *music_file;
static char buf[BUF_SIZE];

void set_music_file(char *path)
{
    if (music_file) {
        fclose(music_file);
    }
    music_file = fopen(path, "rb");
    if (music_file == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
    } else {
        ESP_LOGI(TAG, "Opened file: %s", path);
    }
}

void set_wav_cfg()
{
    if (music_file == NULL) {
        ESP_LOGE(TAG, "Music file is not set");
        return;
    }

    wav_header header;
    fgets(&header, sizeof(header), music_file);
    
    // wav file check

    // set i2s channel, sample rate, bits
    i2s_reconfig(header.sample_rate, header.num_channels);
}



void file_task(void *pvParameter)
{
    i2s_init_std_simplex();
    audio_enable();

    while (1) {
        if (MP3_STATE.is_play) {
            // read sd card file
            fgets(buf, BUF_SIZE, music_file);
            
            // pcm write to i2s
            i2s_write(buf, BUF_SIZE);
        } else {
            vTaskDelay( 1 / portTICK_PERIOD_MS);
        }        
    }
    
    // sd_unmount();
    // sd_uninit();
    vTaskDelete(NULL);
}