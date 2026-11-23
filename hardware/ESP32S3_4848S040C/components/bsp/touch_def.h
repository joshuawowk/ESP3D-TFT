// Touch definitions for ESP32S3_4848S040C
// Touch driver GT911 (I2C)
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define TOUCH_CONTROLLER "GT911"

#include "gt911.h"

const gt911_config_t gt911_cfg = {
    .i2c_clk_speed = 400 * 1000,
    .i2c_addr =
        (uint8_t[]){
            0x5D, 0X14,
            0},  // Floating or mis-configured INT pin may cause GT911 to come
                 // up at address 0x14 instead of 0x5D, so check there as well.
    .rst_pin = -1,  // RST pin not connected
    .int_pin = -1,  // INT pin not connected
    .swap_xy = false,
    .invert_x = false,
    .invert_y = false,
    .x_max = 0,  // auto detect
    .y_max = 0,  // auto detect
};

#ifdef __cplusplus
} /* extern "C" */
#endif
