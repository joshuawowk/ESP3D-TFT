/*
  esp3d_tft

  Copyright (c) 2022 Luc Lebosse. All rights reserved.

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

#include "esp3d_tft_ui.h"

#include <string.h>

#include <string>

#include "esp3d_hal.h"
#include "esp3d_log.h"
#include "esp3d_settings.h"  // ESP3D_SCREEN_POWER_SETTINGS
#include "esp3d_values.h"
#include "esp3d_version.h"
#include "esp_freertos_hooks.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "rendering/esp3d_rendering_client.h"
#include "tasks_def.h"

#define STACKDEPTH UI_STACK_DEPTH
#define TASKPRIORITY UI_TASK_PRIORITY
#define TASKCORE UI_TASK_CORE

/**********************
 *  STATIC PROTOTYPES
 **********************/

ESP3DTftUi esp3dTftui;
#if !LV_TICK_CUSTOM
static void lv_tick_task(void *arg);
#endif
static void guiTask(void *pvParameter);
extern void create_application(void);
#if !LV_TICK_CUSTOM
static void lv_tick_task(void *arg) {
  (void)arg;

  lv_tick_inc(LV_TICK_PERIOD_MS);
}
#endif  // !LV_TICK_CUSTOM

/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
SemaphoreHandle_t xGuiSemaphore = nullptr;

/**********************
 *  Screen brightness and sleep
 **********************/
// Provided by boards that can drive their backlight (see bsp.h). Weak, so
// boards without it link unchanged and get no brightness / sleep control.
extern "C" esp_err_t bsp_set_backlight(uint8_t percent)
    __attribute__((weak));

#if ESP3D_SCREEN_POWER_SETTINGS
#include <atomic>

#define SCREEN_POWER_CHECK_PERIOD_MS 500
static std::atomic<bool> screen_power_dirty{true};
static uint8_t screen_brightness = 100;
static uint32_t screen_sleep_ms = 0;
static bool screen_asleep = false;
static lv_obj_t *wake_overlay = nullptr;

static void set_backlight(uint8_t percent) {
  if (bsp_set_backlight != nullptr) {
    bsp_set_backlight(percent);
  }
}

// While asleep a transparent overlay on the top layer catches the first
// touch: it wakes the screen without pressing what is underneath.
static void wake_overlay_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    set_backlight(screen_brightness);
    screen_asleep = false;
    esp3d_log("Screen wakes up");
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    lv_obj_del_async(lv_event_get_target(e));
    wake_overlay = nullptr;
  }
}

static void screen_go_to_sleep() {
  wake_overlay = lv_obj_create(lv_layer_top());
  if (!lv_obj_is_valid(wake_overlay)) {
    wake_overlay = nullptr;
    return;
  }
  lv_obj_remove_style_all(wake_overlay);
  lv_obj_set_size(wake_overlay, LV_HOR_RES, LV_VER_RES);
  lv_obj_add_flag(wake_overlay, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(wake_overlay, wake_overlay_event_cb, LV_EVENT_ALL,
                      nullptr);
  set_backlight(0);
  screen_asleep = true;
  esp3d_log("Screen goes to sleep");
}

static bool printer_is_printing() {
  const char *status =
      esp3dTftValues.get_string_value(ESP3DValuesIndex::job_status);
  return status != nullptr &&
         (strcmp(status, "processing") == 0 || strcmp(status, "paused") == 0);
}
#endif  // ESP3D_SCREEN_POWER_SETTINGS

bool ESP3DTftUi::hasBacklightControl() { return bsp_set_backlight != nullptr; }

void ESP3DTftUi::applyScreenPowerSettings() {
#if ESP3D_SCREEN_POWER_SETTINGS
  screen_power_dirty = true;  // applied by the UI task
#endif  // ESP3D_SCREEN_POWER_SETTINGS
}

void ESP3DTftUi::handleScreenPower() {
#if ESP3D_SCREEN_POWER_SETTINGS
  if (bsp_set_backlight == nullptr) {
    return;
  }
  if (screen_power_dirty) {
    screen_power_dirty = false;
    screen_brightness =
        esp3dTftsettings.readByte(ESP3DSettingIndex::esp3d_screen_brightness);
    if (screen_brightness < 10 || screen_brightness > 100) {
      screen_brightness = 100;
    }
    screen_sleep_ms =
        (uint32_t)esp3dTftsettings.readByte(
            ESP3DSettingIndex::esp3d_screen_sleep) *
        60 * 1000;
    if (!screen_asleep) {
      set_backlight(screen_brightness);
    }
  }
  static int64_t last_check = 0;
  int64_t now = esp3d_hal::millis();
  if (now - last_check < SCREEN_POWER_CHECK_PERIOD_MS) {
    return;
  }
  last_check = now;
  if (!screen_asleep && wake_overlay == nullptr && screen_sleep_ms > 0 &&
      lv_disp_get_inactive_time(NULL) >= screen_sleep_ms &&
      !printer_is_printing()) {
    screen_go_to_sleep();
  }
#endif  // ESP3D_SCREEN_POWER_SETTINGS
}

#if ESP3D_REMOTE_TOUCH_FEATURE
#include <atomic>
// Virtual pointer fed by GET /touch. Read by LVGL in the UI task.
static std::atomic<int32_t> remote_touch_x{0};
static std::atomic<int32_t> remote_touch_y{0};
static std::atomic<int64_t> remote_touch_release_at{0};
static std::atomic<bool> remote_touch_released{true};

static void remote_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  (void)drv;
  data->point.x = remote_touch_x;
  data->point.y = remote_touch_y;
  if (esp3d_hal::millis() < remote_touch_release_at) {
    data->state = LV_INDEV_STATE_PR;
  } else {
    data->state = LV_INDEV_STATE_REL;
    remote_touch_released = true;
  }
}

static void remote_touch_register() {
  static lv_indev_drv_t remote_indev_drv;
  lv_indev_drv_init(&remote_indev_drv);
  remote_indev_drv.type = LV_INDEV_TYPE_POINTER;
  remote_indev_drv.read_cb = remote_touch_read;
  if (lv_indev_drv_register(&remote_indev_drv) == nullptr) {
    esp3d_log_e("Remote touch input registration failed");
  }
}

bool ESP3DTftUi::remoteTouch(int32_t x, int32_t y, uint32_t duration_ms) {
  if (x < 0 || y < 0 || x >= LV_HOR_RES || y >= LV_VER_RES) {
    return false;
  }
  if (duration_ms < 50) duration_ms = 50;
  if (duration_ms > 5000) duration_ms = 5000;
  // Wait for a previous tap to be released first
  for (int i = 0; i < 100 && !remote_touch_released; i++) {
    esp3d_hal::wait(10);
  }
  remote_touch_x = x;
  remote_touch_y = y;
  remote_touch_released = false;
  remote_touch_release_at = esp3d_hal::millis() + duration_ms;
  // Hold the caller until LVGL has seen the release (input read period 30ms)
  esp3d_hal::wait(duration_ms);
  for (int i = 0; i < 50 && !remote_touch_released; i++) {
    esp3d_hal::wait(10);
  }
  return remote_touch_released;
}
#endif  // ESP3D_REMOTE_TOUCH_FEATURE

static void guiTask(void *pvParameter) {
  (void)pvParameter;
  xGuiSemaphore = xSemaphoreCreateMutex();
#if !LV_TICK_CUSTOM
  /* Create and start a periodic timer interrupt to call lv_tick_inc */
  const esp_timer_create_args_t periodic_timer_args = {
      .callback = &lv_tick_task,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "periodic_gui",
      .skip_unhandled_events = false};
  esp_timer_handle_t periodic_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
  ESP_ERROR_CHECK(
      esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));
#endif  // !LV_TICK_CUSTOM
#if ESP3D_REMOTE_TOUCH_FEATURE
  remote_touch_register();
#endif  // ESP3D_REMOTE_TOUCH_FEATURE
  create_application();

  while (1) {
    /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
    esp3d_hal::wait(10);

    /* Try to take the semaphore, call lvgl related function on success */
    if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
      esp3dTftValues.handle();
      lv_task_handler();
      esp3dTftui.handleScreenPower();
      xSemaphoreGive(xGuiSemaphore);
    }
  }

  /* A task should NEVER return */
  vTaskDelete(NULL);
}

ESP3DTftUi::ESP3DTftUi() {}

bool ESP3DTftUi::lockGui(uint32_t timeout_ms) {
  if (xGuiSemaphore == nullptr) {
    return false;
  }
  return xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void ESP3DTftUi::unlockGui() {
  if (xGuiSemaphore != nullptr) {
    xSemaphoreGive(xGuiSemaphore);
  }
}

ESP3DTftUi::~ESP3DTftUi() {}

bool ESP3DTftUi::begin() {
  if (!renderingClient.begin()) {
    esp3d_log_e("Rendering client not started");
    return false;
  }

  // Ui creation
  TaskHandle_t xHandle = NULL;
  BaseType_t res = xTaskCreatePinnedToCore(guiTask, "tftUI", STACKDEPTH, NULL,
                                           TASKPRIORITY, &xHandle, TASKCORE);
  if (res == pdPASS && xHandle) {
    esp3d_log("Created UI Task");
    return true;
  } else {
    esp3d_log_e("UI Task creation failed");
    return false;
  }
}

void ESP3DTftUi::handle() {}

bool ESP3DTftUi::end() {
  // TODO : stop TFT task and delete it
  renderingClient.end();
  return true;
}
