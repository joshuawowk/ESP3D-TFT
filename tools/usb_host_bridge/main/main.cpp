/*
  USB host to UART bridge for ESP3D-TFT

  Runs on an ESP32-S3 with a free native USB port (e.g. ESP32-S3-DevKitC-1).
  The printer's USB port (CH34x / CP210x / FTDI USB-serial bridge) plugs into
  the ESP32-S3 native USB port, which runs as a USB host. Everything received
  from the printer is written to a UART that is wired to the ESP3D-TFT display
  serial port, and everything received on that UART is sent to the printer.

  The bridge never changes DTR/RTS by default: on most printer boards DTR is
  wired to the MCU reset, and toggling it would reset the printer.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*/

#include <atomic>
#include <cstdio>
#include <memory>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "usb/cdc_acm_host.h"
#include "usb/usb_host.h"
#include "usb/vcp.hpp"
#include "usb/vcp_ch34x.hpp"
#include "usb/vcp_cp210x.hpp"
#include "usb/vcp_ftdi.hpp"

using namespace esp_usb;

static const char *TAG = "bridge";

#define BRIDGE_UART ((uart_port_t)CONFIG_BRIDGE_UART_NUM)
#ifdef CONFIG_BRIDGE_ASSERT_DTR
#define BRIDGE_ASSERT_DTR 1
#else
#define BRIDGE_ASSERT_DTR 0
#endif
#define UART_BUF_SIZE 4096
#define USB_BUF_SIZE 512
#define UART_CHUNK 256

static SemaphoreHandle_t s_disconnected_sem;
static SemaphoreHandle_t s_dev_mutex;
static CdcAcmDevice *s_dev = nullptr;  // guarded by s_dev_mutex
static std::atomic<uint32_t> s_usb_to_uart_bytes{0};
static std::atomic<uint32_t> s_uart_to_usb_bytes{0};
static std::atomic<uint32_t> s_dropped_bytes{0};
static std::atomic<bool> s_connected{false};
static led_strip_handle_t s_led = nullptr;

#ifdef CONFIG_BRIDGE_CONSOLE_PASSTHROUGH
#define CONSOLE_UART UART_NUM_0
static std::atomic<uint32_t> s_console_bytes{0};

// PC (console port) -> display, only while no printer is connected
static void console_rx_task(void *arg) {
  static uint8_t buf[UART_CHUNK];
  while (true) {
    int len = uart_read_bytes(CONSOLE_UART, buf, sizeof(buf), pdMS_TO_TICKS(5));
    if (len > 0 && !s_connected) {
      uart_write_bytes(BRIDGE_UART, buf, len);
    }
  }
}
#endif

enum class LedState { waiting, connected };

static void led_set(LedState state) {
  if (!s_led) {
    return;
  }
  // Low brightness: the WS2812 on the DevKit is very bright
  if (state == LedState::connected) {
    led_strip_set_pixel(s_led, 0, 0, 16, 0);
  } else {
    led_strip_set_pixel(s_led, 0, 0, 0, 16);
  }
  led_strip_refresh(s_led);
}

static void led_init() {
#if CONFIG_BRIDGE_LED_GPIO >= 0
  led_strip_config_t strip_config = {};
  strip_config.strip_gpio_num = CONFIG_BRIDGE_LED_GPIO;
  strip_config.max_leds = 1;
  strip_config.led_pixel_format = LED_PIXEL_FORMAT_GRB;
  strip_config.led_model = LED_MODEL_WS2812;
  led_strip_rmt_config_t rmt_config = {};
  rmt_config.resolution_hz = 10 * 1000 * 1000;
  if (led_strip_new_rmt_device(&strip_config, &rmt_config, &s_led) != ESP_OK) {
    ESP_LOGW(TAG, "Status LED init failed, continuing without it");
    s_led = nullptr;
  }
#endif
}

// USB -> UART: called from the CDC-ACM driver task for every IN transfer
static bool handle_usb_rx(const uint8_t *data, size_t data_len, void *arg) {
  uart_write_bytes(BRIDGE_UART, data, data_len);
  s_usb_to_uart_bytes += data_len;
#ifdef CONFIG_BRIDGE_LOG_TRAFFIC
  printf("[printer->tft] %.*s", (int)data_len, (const char *)data);
  if (data_len && data[data_len - 1] != '\n') {
    printf("\n");
  }
#endif
  return true;
}

static void handle_usb_event(const cdc_acm_host_dev_event_data_t *event,
                             void *user_ctx) {
  switch (event->type) {
    case CDC_ACM_HOST_ERROR:
      ESP_LOGE(TAG, "CDC-ACM error, err_no = %d", event->data.error);
      break;
    case CDC_ACM_HOST_DEVICE_DISCONNECTED:
      ESP_LOGI(TAG, "Printer USB device disconnected");
      xSemaphoreGive(s_disconnected_sem);
      break;
    case CDC_ACM_HOST_SERIAL_STATE:
      ESP_LOGD(TAG, "Serial state 0x%04X", event->data.serial_state.val);
      break;
    default:
      break;
  }
}

static void usb_lib_task(void *arg) {
  while (true) {
    uint32_t event_flags;
    usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
      usb_host_device_free_all();
    }
  }
}

// UART -> USB
static void uart_rx_task(void *arg) {
  static uint8_t buf[UART_CHUNK];
  while (true) {
    int len = uart_read_bytes(BRIDGE_UART, buf, sizeof(buf), pdMS_TO_TICKS(5));
    if (len <= 0) {
      continue;
    }
#ifdef CONFIG_BRIDGE_LOG_TRAFFIC
    printf("[tft->printer] %.*s", len, (const char *)buf);
    if (buf[len - 1] != '\n') {
      printf("\n");
    }
#endif
    bool sent = false;
    if (xSemaphoreTake(s_dev_mutex, portMAX_DELAY) == pdTRUE) {
      if (s_dev) {
        sent = (s_dev->tx_blocking(buf, len, 1000) == ESP_OK);
      }
      xSemaphoreGive(s_dev_mutex);
    }
    if (sent) {
      s_uart_to_usb_bytes += len;
    } else {
#ifdef CONFIG_BRIDGE_CONSOLE_PASSTHROUGH
      if (!s_connected) {
        // Bench mode: the PC on the console port plays the printer
        uart_write_bytes(CONSOLE_UART, buf, len);
        s_console_bytes += len;
        continue;
      }
#endif
      // Printer not connected (or USB error): drop, the host will retry
      s_dropped_bytes += len;
    }
  }
}

static void stats_task(void *arg) {
  uint32_t last_in = 0, last_out = 0, last_drop = 0;
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(10000));
    uint32_t in = s_usb_to_uart_bytes, out = s_uart_to_usb_bytes,
             drop = s_dropped_bytes;
    if (in != last_in || out != last_out || drop != last_drop) {
      ESP_LOGI(TAG,
               "printer->tft %lu B, tft->printer %lu B, dropped (no printer) "
               "%lu B",
               (unsigned long)in, (unsigned long)out, (unsigned long)drop);
      last_in = in;
      last_out = out;
      last_drop = drop;
    }
  }
}

static void uart_init() {
  uart_config_t uart_config = {};
  uart_config.baud_rate = CONFIG_BRIDGE_UART_BAUDRATE;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.source_clk = UART_SCLK_DEFAULT;
  ESP_ERROR_CHECK(uart_driver_install(BRIDGE_UART, UART_BUF_SIZE,
                                      UART_BUF_SIZE, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(BRIDGE_UART, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(BRIDGE_UART, CONFIG_BRIDGE_UART_TX_GPIO,
                               CONFIG_BRIDGE_UART_RX_GPIO, UART_PIN_NO_CHANGE,
                               UART_PIN_NO_CHANGE));
}

extern "C" void app_main(void) {
  ESP_LOGI(TAG,
           "ESP3D-TFT USB host bridge: UART%d TX=GPIO%d RX=GPIO%d @%d, USB "
           "@%d, DTR %s",
           CONFIG_BRIDGE_UART_NUM, CONFIG_BRIDGE_UART_TX_GPIO,
           CONFIG_BRIDGE_UART_RX_GPIO, CONFIG_BRIDGE_UART_BAUDRATE,
           CONFIG_BRIDGE_USB_BAUDRATE,
           BRIDGE_ASSERT_DTR ? "asserted" : "untouched");

  s_disconnected_sem = xSemaphoreCreateBinary();
  s_dev_mutex = xSemaphoreCreateMutex();
  assert(s_disconnected_sem && s_dev_mutex);

  led_init();
  led_set(LedState::waiting);
  uart_init();

  usb_host_config_t host_config = {};
  host_config.skip_phy_setup = false;
  host_config.intr_flags = ESP_INTR_FLAG_LEVEL1;
  ESP_ERROR_CHECK(usb_host_install(&host_config));
  xTaskCreate(usb_lib_task, "usb_lib", 4096, NULL, 10, NULL);

  ESP_ERROR_CHECK(cdc_acm_host_install(NULL));
  VCP::register_driver<FT23x>();
  VCP::register_driver<CP210x>();
  VCP::register_driver<CH34x>();

  xTaskCreate(uart_rx_task, "uart_rx", 4096, NULL, 9, NULL);
#ifdef CONFIG_BRIDGE_CONSOLE_PASSTHROUGH
  ESP_LOGI(TAG, "Bench mode: console port stands in for the printer; logs off");
  vTaskDelay(pdMS_TO_TICKS(20));
  esp_log_level_set("*", ESP_LOG_NONE);
  ESP_ERROR_CHECK(uart_driver_install(CONSOLE_UART, UART_BUF_SIZE,
                                      UART_BUF_SIZE, 0, NULL, 0));
  xTaskCreate(console_rx_task, "console_rx", 4096, NULL, 9, NULL);
#else
  xTaskCreate(stats_task, "stats", 3072, NULL, 1, NULL);
#endif

  cdc_acm_line_coding_t line_coding = {
      .dwDTERate = CONFIG_BRIDGE_USB_BAUDRATE,
      .bCharFormat = 0,  // 1 stop bit
      .bParityType = 0,  // none
      .bDataBits = 8,
  };

  while (true) {
    const cdc_acm_host_device_config_t dev_config = {
        .connection_timeout_ms = 5000,
        .out_buffer_size = USB_BUF_SIZE,
        .in_buffer_size = USB_BUF_SIZE,
        .event_cb = handle_usb_event,
        .data_cb = handle_usb_rx,
        .user_arg = NULL,
    };

    ESP_LOGI(TAG, "Waiting for the printer USB device...");
    xSemaphoreTake(s_disconnected_sem, 0);  // clear any stale signal
    CdcAcmDevice *dev = nullptr;
    try {
      dev = VCP::open(&dev_config);
    } catch (esp_err_t err) {
      ESP_LOGW(TAG, "VCP open failed: %s", esp_err_to_name(err));
      dev = nullptr;
    }
    if (dev == nullptr) {
      continue;  // timeout, nothing plugged in (or printer off)
    }
    ESP_LOGI(TAG, "Printer USB device opened");
    vTaskDelay(pdMS_TO_TICKS(10));

    esp_err_t err = dev->line_coding_set(&line_coding);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to set line coding: %s", esp_err_to_name(err));
    }
#if BRIDGE_ASSERT_DTR
    err = dev->set_control_line_state(true, true);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to set DTR/RTS: %s", esp_err_to_name(err));
    }
#endif

    // Discard anything the display sent while the printer was absent
    uart_flush_input(BRIDGE_UART);
    if (xSemaphoreTake(s_dev_mutex, portMAX_DELAY) == pdTRUE) {
      s_dev = dev;
      s_connected = true;
      xSemaphoreGive(s_dev_mutex);
    }
    led_set(LedState::connected);
    ESP_LOGI(TAG, "Bridge active");

    xSemaphoreTake(s_disconnected_sem, portMAX_DELAY);

    if (xSemaphoreTake(s_dev_mutex, portMAX_DELAY) == pdTRUE) {
      s_dev = nullptr;
      s_connected = false;
      xSemaphoreGive(s_dev_mutex);
    }
    delete dev;  // closes the CDC-ACM handle
    led_set(LedState::waiting);
  }
}
