#pragma once

// spi
#define SD_SPI_HOST         CONFIG_MP3_SPI_HOST
#define SD_SPI_CLK          CONFIG_MP3_SPI_PIN_CLK
#define SD_SPI_MISO         CONFIG_MP3_SPI_PIN_MISO
#define SD_SPI_MOSI         CONFIG_MP3_SPI_PIN_MOSI
#define SD_SPI_CS           CONFIG_SDCARD_SPI_PIN_CS


#define MOUNT_POINT "/sdcard"

void sd_spi_init();
esp_err_t sd_mount();
void sd_unmount();
void sd_uninit();