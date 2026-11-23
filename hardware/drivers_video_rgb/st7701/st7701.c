/*
  st7701.c - ST7701(S) RGB panel driver

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

/*********************
 *      INCLUDES
 *********************/
#include "st7701.h"

#include <driver/gpio.h>
#include <rom/ets_sys.h>

#include "esp3d_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*********************
 *      DEFINES
 *********************/
// Half clock period of the bit-banged init SPI. ST7701 accepts up to ~15MHz,
// so 1us per half period is very conservative and only runs once at boot.
#define ST7701_SPI_HALF_PERIOD_US 1

/**********************
 *  STATIC VARIABLES
 **********************/
static st7701_spi_config_t _spi;

/**********************
 *  STATIC FUNCTIONS
 **********************/

/**
 * @brief Clock out one 9-bit word: D/C bit first, then 8 bits MSB first.
 * Data is latched by the panel on the rising edge of SCL (SPI mode 0).
 */
static void st7701_spi_write9(bool is_data, uint8_t value) {
  uint16_t word = ((is_data ? 1 : 0) << 8) | value;
  for (int bit = 8; bit >= 0; bit--) {
    gpio_set_level(_spi.scl_gpio_num, 0);
    gpio_set_level(_spi.sda_gpio_num, (word >> bit) & 0x01);
    ets_delay_us(ST7701_SPI_HALF_PERIOD_US);
    gpio_set_level(_spi.scl_gpio_num, 1);
    ets_delay_us(ST7701_SPI_HALF_PERIOD_US);
  }
}

static void st7701_send(uint8_t cmd, const uint8_t *data, uint8_t len) {
  gpio_set_level(_spi.cs_gpio_num, 0);
  ets_delay_us(ST7701_SPI_HALF_PERIOD_US);
  st7701_spi_write9(false, cmd);
  for (uint8_t i = 0; i < len; i++) {
    st7701_spi_write9(true, data[i]);
  }
  gpio_set_level(_spi.scl_gpio_num, 0);
  ets_delay_us(ST7701_SPI_HALF_PERIOD_US);
  gpio_set_level(_spi.cs_gpio_num, 1);
  ets_delay_us(ST7701_SPI_HALF_PERIOD_US);
}

static esp_err_t st7701_spi_init(const st7701_spi_config_t *cfg) {
  _spi = *cfg;
  uint64_t mask = (1ULL << cfg->cs_gpio_num) | (1ULL << cfg->scl_gpio_num) |
                  (1ULL << cfg->sda_gpio_num);
  if (cfg->rst_gpio_num >= 0) {
    mask |= (1ULL << cfg->rst_gpio_num);
  }
  gpio_config_t io_conf = {
      .pin_bit_mask = mask,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  esp_err_t err = gpio_config(&io_conf);
  if (err != ESP_OK) {
    return err;
  }
  gpio_set_level(cfg->cs_gpio_num, 1);
  gpio_set_level(cfg->scl_gpio_num, 0);
  gpio_set_level(cfg->sda_gpio_num, 0);
  if (cfg->rst_gpio_num >= 0) {
    gpio_set_level(cfg->rst_gpio_num, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(cfg->rst_gpio_num, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
  }
  return ESP_OK;
}

static void st7701_spi_release(void) {
  // Keep CS actively driven high so the panel ignores any later traffic on
  // the shared SCL/SDA lines (e.g. SD card SPI), then free SCL/SDA.
  gpio_set_level(_spi.cs_gpio_num, 1);
  gpio_reset_pin(_spi.scl_gpio_num);
  gpio_reset_pin(_spi.sda_gpio_num);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

esp_err_t esp_lcd_new_panel_st7701(const esp_rgb_st7701_config_t *disp_panel_cfg,
                                   esp_lcd_panel_handle_t *disp_panel) {
  if (disp_panel_cfg == NULL || disp_panel == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  // 1 - Configure the panel controller over 3-wire SPI
  if (st7701_spi_init(&disp_panel_cfg->spi_config) != ESP_OK) {
    esp3d_log_e("Failed to configure ST7701 init SPI pins");
    return ESP_FAIL;
  }
  esp3d_log("Sending %d ST7701 init commands", disp_panel_cfg->init_cmds_size);
  for (uint16_t i = 0; i < disp_panel_cfg->init_cmds_size; i++) {
    const st7701_lcd_init_cmd_t *c = &disp_panel_cfg->init_cmds[i];
    st7701_send(c->cmd, c->data, c->data_bytes);
    if (c->delay_ms > 0) {
      vTaskDelay(pdMS_TO_TICKS(c->delay_ms));
    }
  }
  st7701_spi_release();

  // 2 - Start the RGB interface
  if (esp_lcd_new_rgb_panel(&(disp_panel_cfg->panel_config), disp_panel) !=
      ESP_OK) {
    esp3d_log_e("Failed to create new RGB panel");
    return ESP_FAIL;
  }
  if (esp_lcd_panel_reset(*disp_panel) != ESP_OK) {
    esp3d_log_e("Failed to reset panel");
    return ESP_FAIL;
  }
  if (esp_lcd_panel_init(*disp_panel) != ESP_OK) {
    esp3d_log_e("Failed to initialize panel");
    return ESP_FAIL;
  }
  return ESP_OK;
}
