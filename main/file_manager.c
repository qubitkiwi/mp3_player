#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "stdio.h"
#include "string.h"
#include "esp_log.h"

#include "file_manager.h"
#include "device/sdspi.h"

static char TAG[] = "FILE MANAGER";


void find_mp3(char *path)
{
    return ;
}

void read_mp3(char *path)
{
    return ;
}

static esp_err_t s_example_read_file(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[100];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    return ESP_OK;
}


void file_task(void *pvParameter)
{
    // sd_init();
    sd_mount();

    while (1) {
        // read foo.txt
        esp_err_t ret;
        const char *file_foo = MOUNT_POINT"/foo.txt";

        ret = s_example_read_file(file_foo);
        if (ret != ESP_OK) {
            break;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    sd_unmount();
    sd_uninit();
    vTaskDelete(NULL);
}