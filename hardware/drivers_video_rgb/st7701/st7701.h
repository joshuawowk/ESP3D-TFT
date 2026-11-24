/*
  st7701.h - ST7701(S) RGB panel driver

  The ST7701 is an RGB (DPI) panel controller that must be configured over a
  3-wire, 9-bit SPI bus (D/C bit + 8 data bits) before the RGB interface is
  started. The init SPI is bit-banged here so its pins can be handed to another
  peripheral (for example the SD card SPI bus) once the panel is configured.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief One entry of the ST7701 init sequence.
 *
 * `cmd` is sent with D/C=0, then `data_bytes` bytes of `data` with D/C=1,
 * then the driver waits `delay_ms` milliseconds.
 */
typedef struct {
  uint8_t cmd;
  const uint8_t *data;
  uint8_t data_bytes;
  uint16_t delay_ms;
} st7701_lcd_init_cmd_t;

/**
 * @brief 3-wire (9-bit) SPI pins used to configure the ST7701.
 */
typedef struct {
  int cs_gpio_num;
  int scl_gpio_num;
  int sda_gpio_num;
  int rst_gpio_num;  // -1 if the panel reset line is not wired
} st7701_spi_config_t;

typedef struct {
  esp_lcd_rgb_panel_config_t panel_config; /**< RGB interface config */
  st7701_spi_config_t spi_config;          /**< Init SPI pins */
  const st7701_lcd_init_cmd_t *init_cmds;  /**< Init sequence */
  uint16_t init_cmds_size;                 /**< Number of init entries */
  uint16_t hor_res;
  uint16_t ver_res;
} esp_rgb_st7701_config_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Configure the ST7701 over 3-wire SPI, then create and initialize the
 * RGB panel.
 *
 * The init SPI GPIOs are reset to their default state afterwards so they can
 * be reused (for example by the SD card SPI bus).
 *
 * @param disp_panel_cfg Panel configuration
 * @param disp_panel Returned panel handle
 * @return ESP_OK on success
 */
esp_err_t esp_lcd_new_panel_st7701(const esp_rgb_st7701_config_t *disp_panel_cfg,
                                   esp_lcd_panel_handle_t *disp_panel);

#ifdef __cplusplus
} /* extern "C" */
#endif
