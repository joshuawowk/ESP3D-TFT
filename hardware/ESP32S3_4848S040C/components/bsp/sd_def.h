// Pins definition for ESP32S3_4848S040C
// SD SPI
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ESP32S3_4848S040C SD SPI GPIO definition
 * Note: MOSI/CLK are shared with the ST7701S 3-wire init bus, which is only
 * used once at boot (before the SD card is initialized). The ST7701S CS
 * (GPIO 39) is kept high afterwards.
 */
#define ESP3D_SD_IS_SPI 1
#define ESP3D_SD_MOSI_PIN (47)  // GPIO 47
#define ESP3D_SD_MISO_PIN (41)  // GPIO 41
#define ESP3D_SD_CLK_PIN (48)   // GPIO 48
#define ESP3D_SD_CS_PIN (42)    // GPIO 42

// #define ESP3D_SD_DETECT_PIN (-1) //GPIO -1
// #define ESP3D_SD_DETECT_VALUE (0) //LOW
#define MAX_TRANSFER_SZ (4096)

#define SPI_ALLOCATION_SIZE (16 * 1024)
//(range 400kHz - 20MHz for SDSPI, less for other devices)
// default is 20MHz
#define ESP3D_SD_FREQ (20000)

#define SD_SPI_HOST SPI2_HOST  // 1
// #define SD_SPI_HOST SPI3_HOST  // 2

#ifdef __cplusplus
} /* extern "C" */
#endif
