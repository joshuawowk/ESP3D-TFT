// Display definitions for ESP32S3_4848S040C
// Guition ESP32-S3-4848S040: 4.0in 480x480 IPS
// Display driver ST7701S (16-bit RGB565 parallel interface, configured over
// 3-wire 9-bit SPI)
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define TFT_DISPLAY_CONTROLLER "ST7701S"

#include "disp_backlight.h"
#include "st7701.h"

#define DISP_HOR_RES_MAX 480
#define DISP_VER_RES_MAX 480

#define DISP_CLK_FREQ (12 * 1000 * 1000)  // 12MHz (~45fps)
#define DISP_AVOID_TEAR_EFFECT_WITH_SEM (true)
#define DISP_USE_BOUNCE_BUFFER (false)
#define DISP_USE_DOUBLE_BUFFER (true)
#define DISP_NUM_FB (1)

#define DISP_PATCH_FS_FREQ (6 * 1000 * 1000)  // 6MHz
#define DISP_PATCH_FS_DELAY (40)

#if DISP_NUM_FB == 2
// Full frame buffer (450KB) in external PSRAM
#define DISP_BUF_SIZE (DISP_HOR_RES_MAX * DISP_VER_RES_MAX)
#else
// 1/4 (120-line) buffer (112.5KB) in external PSRAM
#define DISP_BUF_SIZE (DISP_HOR_RES_MAX * DISP_VER_RES_MAX / 4)
#endif  // DISP_NUM_FB == 2
#define DISP_BUF_SIZE_BYTES (DISP_BUF_SIZE * 2)

// ST7701S init sequence for the Guition ESP32-S3-4848S040 panel.
// Matches Arduino_GFX st7701_type9_init_operations ("480x480 square (86 Box)
// GUITION ESP32-4848S040") and the ESPHome GUITION-4848S040 model.
// The panel has no reset line, so a software reset is sent first.
static const st7701_lcd_init_cmd_t st7701_4848s040_init_cmds[] = {
    {0x01, NULL, 0, 120},  // SWRESET
    // Command2 BK0
    {0xFF, (const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (const uint8_t[]){0x3B, 0x00}, 2, 0},
    {0xC1, (const uint8_t[]){0x0D, 0x02}, 2, 0},
    {0xC2, (const uint8_t[]){0x31, 0x05}, 2, 0},
    {0xCD, (const uint8_t[]){0x00}, 1, 0},
    {0xB0,
     (const uint8_t[]){0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07,
                       0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18},
     16, 0},  // Positive gamma
    {0xB1,
     (const uint8_t[]){0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08,
                       0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18},
     16, 0},  // Negative gamma
    // Command2 BK1
    {0xFF, (const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (const uint8_t[]){0x60}, 1, 0},
    {0xB1, (const uint8_t[]){0x32}, 1, 0},
    {0xB2, (const uint8_t[]){0x07}, 1, 0},
    {0xB3, (const uint8_t[]){0x80}, 1, 0},
    {0xB5, (const uint8_t[]){0x49}, 1, 0},
    {0xB7, (const uint8_t[]){0x85}, 1, 0},
    {0xB8, (const uint8_t[]){0x21}, 1, 0},
    {0xC1, (const uint8_t[]){0x78}, 1, 0},
    {0xC2, (const uint8_t[]){0x78}, 1, 0},
    {0xE0, (const uint8_t[]){0x00, 0x1B, 0x02}, 3, 0},
    {0xE1,
     (const uint8_t[]){0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00,
                       0x44, 0x44},
     11, 0},
    {0xE2,
     (const uint8_t[]){0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC,
                       0xA0, 0x00, 0x00},
     12, 0},
    {0xE3, (const uint8_t[]){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE4, (const uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE5,
     (const uint8_t[]){0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E,
                       0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0},
     16, 0},
    {0xE6, (const uint8_t[]){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE7, (const uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE8,
     (const uint8_t[]){0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D,
                       0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0},
     16, 0},
    {0xEB, (const uint8_t[]){0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40}, 7, 0},
    {0xEC, (const uint8_t[]){0x3C, 0x00}, 2, 0},
    {0xED,
     (const uint8_t[]){0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF,
                       0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA},
     16, 0},
    // Command2 BK3
    {0xFF, (const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xE5, (const uint8_t[]){0xE4}, 1, 0},
    // Back to the standard command set
    {0xFF, (const uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    {0x3A, (const uint8_t[]){0x60}, 1, 10},  // COLMOD: 18-bit (RGB666) input
    {0x11, NULL, 0, 120},                    // SLPOUT
    {0x29, NULL, 0, 20},                     // DISPON
};

const esp_rgb_st7701_config_t disp_panel_cfg = {
    .panel_config =
        {.clk_src = LCD_CLK_SRC_DEFAULT,
         .timings =
             {
                 .pclk_hz = DISP_CLK_FREQ,
                 .h_res = DISP_HOR_RES_MAX,
                 .v_res = DISP_VER_RES_MAX,
                 .hsync_pulse_width = 8,
                 .hsync_back_porch = 20,
                 .hsync_front_porch = 10,
                 .vsync_pulse_width = 8,
                 .vsync_back_porch = 10,
                 .vsync_front_porch = 10,
                 .flags =
                     {
                         .hsync_idle_low = 0,
                         .vsync_idle_low = 0,
                         .de_idle_high = 0,
                         .pclk_active_neg = false,
                         .pclk_idle_high = 0,
                     },
             },
         .data_width = 16,  // RGB565 in parallel mode
         .bits_per_pixel = 0,
         .num_fbs = DISP_NUM_FB,
#if DISP_USE_BOUNCE_BUFFER
         .bounce_buffer_size_px = DISP_BUF_SIZE,
#else
         .bounce_buffer_size_px = 0,
#endif
         .sram_trans_align = 0,
         .psram_trans_align = 64,
         .hsync_gpio_num = 16,  // GPIO 16
         .vsync_gpio_num = 17,  // GPIO 17
         .de_gpio_num = 18,     // GPIO 18
         .pclk_gpio_num = 21,   // GPIO 21
         .disp_gpio_num = -1,   // EN pin not connected
         .data_gpio_nums =
             {
                 4,   // D0  (B0) - GPIO 4
                 5,   // D1  (B1) - GPIO 5
                 6,   // D2  (B2) - GPIO 6
                 7,   // D3  (B3) - GPIO 7
                 15,  // D4  (B4) - GPIO 15
                 8,   // D5  (G0) - GPIO 8
                 20,  // D6  (G1) - GPIO 20
                 3,   // D7  (G2) - GPIO 3
                 46,  // D8  (G3) - GPIO 46
                 9,   // D9  (G4) - GPIO 9
                 10,  // D10 (G5) - GPIO 10
                 11,  // D11 (R0) - GPIO 11
                 12,  // D12 (R1) - GPIO 12
                 13,  // D13 (R2) - GPIO 13
                 14,  // D14 (R3) - GPIO 14
                 0,   // D15 (R4) - GPIO 0
             },
         .flags =
             {
                 .disp_active_low = (uint32_t)NULL,
                 .refresh_on_demand = (uint32_t)NULL,
                 .fb_in_psram =
                     true,  // Do not change this, as it is mandatory for RGB
                            // parallel interface and octal PSRAM
                 .double_fb = (uint32_t)NULL,
                 .no_fb = (uint32_t)NULL,
                 .bb_invalidate_cache = (uint32_t)NULL,
             }},
    // 3-wire SPI used only to send the init sequence. SCL/SDA are shared with
    // the SD card SPI bus and are released after init; CS stays driven high.
    .spi_config =
        {
            .cs_gpio_num = 39,   // GPIO 39
            .scl_gpio_num = 48,  // GPIO 48
            .sda_gpio_num = 47,  // GPIO 47
            .rst_gpio_num = -1,  // Not connected
        },
    .init_cmds = st7701_4848s040_init_cmds,
    .init_cmds_size =
        sizeof(st7701_4848s040_init_cmds) / sizeof(st7701_lcd_init_cmd_t),
    .hor_res = DISP_HOR_RES_MAX,
    .ver_res = DISP_VER_RES_MAX,
};

#define DISP_BCKL_DEFAULT_DUTY 100  //%

const disp_backlight_config_t disp_bcklt_cfg = {.pwm_control = true,
                                                .output_invert = false,
                                                .gpio_num = 38,
                                                .timer_idx = 0,
                                                .channel_idx = 0};

#ifdef __cplusplus
} /* extern "C" */
#endif
