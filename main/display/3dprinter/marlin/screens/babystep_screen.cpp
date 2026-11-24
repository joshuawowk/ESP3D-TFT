/*
  babystep_screen.cpp - Z offset (babystepping) during a print

  Sends M290 Z<+/-step> (Marlin BABYSTEPPING) and M500 to save. With
  BABYSTEP_ZPROBE_OFFSET the babysteps also update the probe Z offset, so
  M500 keeps them for the next print.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*/

#include "screens/babystep_screen.h"

#include <lvgl.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>

#include "components/back_button_component.h"
#include "components/symbol_button_component.h"
#include "esp3d_log.h"
#include "esp3d_lvgl.h"
#include "esp3d_styles.h"
#include "esp3d_tft_ui.h"
#include "esp3d_values.h"
#include "rendering/esp3d_rendering_client.h"
#include "screens/main_screen.h"
#include "screens/menu_screen.h"
#include "translations/esp3d_translation_service.h"

namespace babystepScreen {
// Step choices (mm); the selection is kept between visits
const char *babystep_buttons_map[] = {"0.01", "0.02", "0.05", "0.1", ""};
uint8_t babystep_buttons_map_id = 0;
// Total offset sent with M290 since the print started
double babystep_total = 0.0;
lv_obj_t *label_babystep_total = NULL;
lv_timer_t *babystep_screen_delay_timer = NULL;
bool babystep_from_menu = false;

void reset_total() { babystep_total = 0.0; }

static void display_total() {
  if (!lv_obj_is_valid(label_babystep_total)) return;
  char text[16];
  snprintf(text, sizeof(text), "%+.2f", babystep_total);
  lv_label_set_text(label_babystep_total, text);
}

static void send_babystep(double delta) {
  char gcode[24];
  snprintf(gcode, sizeof(gcode), "M290 Z%.3f", delta);
  renderingClient.sendGcode(gcode);
  babystep_total += delta;
  // avoid -0.00 after going up and down
  if (babystep_total > -0.0005 && babystep_total < 0.0005) babystep_total = 0;
  display_total();
}

static double selected_step() {
  return strtod(babystep_buttons_map[babystep_buttons_map_id], NULL);
}

static void babystep_screen_delay_timer_cb(lv_timer_t *timer) {
  if (babystep_screen_delay_timer &&
      lv_timer_is_valid(babystep_screen_delay_timer)) {
    lv_timer_del(babystep_screen_delay_timer);
  }
  babystep_screen_delay_timer = NULL;
  if (babystep_from_menu) {
    menuScreen::create();
  } else {
    mainScreen::create();
  }
}

static void event_button_babystep_back_handler(lv_event_t *e) {
  esp3d_log("back Clicked");
  if (ESP3D_BUTTON_ANIMATION_DELAY) {
    if (babystep_screen_delay_timer) return;
    babystep_screen_delay_timer = lv_timer_create(
        babystep_screen_delay_timer_cb, ESP3D_BUTTON_ANIMATION_DELAY, NULL);
  } else {
    babystep_screen_delay_timer_cb(NULL);
  }
}

static void babystep_btn_up_event_cb(lv_event_t *e) {
  send_babystep(selected_step());  // nozzle away from the bed
}

static void babystep_btn_down_event_cb(lv_event_t *e) {
  send_babystep(-selected_step());  // nozzle closer to the bed
}

static void babystep_btn_save_event_cb(lv_event_t *e) {
  renderingClient.sendGcode("M500");
  esp3dTftValues.set_string_value(
      ESP3DValuesIndex::status_bar_label,
      esp3dTranslationService.translate(ESP3DLabel::z_offset_saved));
}

static void babystep_matrix_buttons_event_cb(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target(e);
  babystep_buttons_map_id = lv_btnmatrix_get_selected_btn(obj);
  esp3d_log("Step %s selected", babystep_buttons_map[babystep_buttons_map_id]);
}

void create(bool from_menu) {
  babystep_from_menu = from_menu;
  esp3dTftui.set_current_screen(ESP3DScreenType::none);
  esp3d_log("Babystep screen creation");
  lv_obj_t *ui_new_screen = lv_obj_create(NULL);
  if (!lv_obj_is_valid(ui_new_screen)) {
    esp3d_log_e("Failed to create babystep screen");
    return;
  }
  lv_obj_t *ui_current_screen = lv_scr_act();
  lv_scr_load(ui_new_screen);
  ESP3DStyle::apply(ui_new_screen, ESP3DStyleType::main_bg);
  if (lv_obj_is_valid(ui_current_screen)) {
    lv_obj_del(ui_current_screen);
  }

  lv_obj_t *btnback = backButton::create(ui_new_screen);
  if (!lv_obj_is_valid(btnback)) {
    esp3d_log_e("Failed to create back button");
    return;
  }
  lv_obj_add_event_cb(btnback, event_button_babystep_back_handler,
                      LV_EVENT_CLICKED, NULL);

  // Step selection
  lv_obj_t *btnm = lv_btnmatrix_create(ui_new_screen);
  if (!lv_obj_is_valid(btnm)) {
    esp3d_log_e("Failed to create button matrix");
    return;
  }
  lv_btnmatrix_set_map(btnm, babystep_buttons_map);
  ESP3DStyle::apply(btnm, ESP3DStyleType::buttons_matrix);
  size_t count =
      (sizeof(babystep_buttons_map) / sizeof(babystep_buttons_map[0])) - 1;
  lv_obj_set_size(btnm, ESP3D_MATRIX_BUTTON_WIDTH * count,
                  ESP3D_MATRIX_BUTTON_HEIGHT);
  lv_obj_align(btnm, LV_ALIGN_TOP_RIGHT, -ESP3D_BUTTON_PRESSED_OUTLINE,
               ESP3D_BUTTON_PRESSED_OUTLINE / 2);
  lv_btnmatrix_set_btn_ctrl(btnm, babystep_buttons_map_id,
                            LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_add_event_cb(btnm, babystep_matrix_buttons_event_cb,
                      LV_EVENT_VALUE_CHANGED, NULL);

  // "Z" label, total offset, unit
  lv_obj_t *label_z = lv_label_create(ui_new_screen);
  if (!lv_obj_is_valid(label_z)) {
    esp3d_log_e("Failed to create Z label");
    return;
  }
  lv_label_set_text(label_z, "Z");
  ESP3DStyle::apply(label_z, ESP3DStyleType::bg_label);
  lv_obj_align(label_z, LV_ALIGN_TOP_LEFT, ESP3D_BUTTON_PRESSED_OUTLINE,
               ESP3D_BUTTON_PRESSED_OUTLINE);
  lv_obj_update_layout(label_z);

  label_babystep_total = lv_label_create(ui_new_screen);
  if (!lv_obj_is_valid(label_babystep_total)) {
    esp3d_log_e("Failed to create offset label");
    return;
  }
  ESP3DStyle::apply(label_babystep_total, ESP3DStyleType::read_only_value);
  lv_obj_set_width(label_babystep_total, LV_HOR_RES / 6);
  lv_obj_align_to(label_babystep_total, label_z, LV_ALIGN_OUT_RIGHT_MID,
                  ESP3D_BUTTON_PRESSED_OUTLINE / 2, 0);
  display_total();

  lv_obj_t *label_unit = lv_label_create(ui_new_screen);
  if (!lv_obj_is_valid(label_unit)) {
    esp3d_log_e("Failed to create unit label");
    return;
  }
  lv_label_set_text(label_unit,
                    esp3dTranslationService.translate(ESP3DLabel::millimeters));
  ESP3DStyle::apply(label_unit, ESP3DStyleType::bg_label);
  lv_obj_align_to(label_unit, label_babystep_total, LV_ALIGN_OUT_RIGHT_MID,
                  ESP3D_BUTTON_PRESSED_OUTLINE / 2, 0);

  // Raise / lower
#if ESP3D_BUTTON_CAPTIONS
  // The caption says it: keep one symbol so the button is not cramped
  lv_obj_t *btn_up = symbolButton::create(ui_new_screen, LV_SYMBOL_UP);
#else
  lv_obj_t *btn_up =
      symbolButton::create(ui_new_screen, LV_SYMBOL_UP "\n" LV_SYMBOL_PLUS);
#endif  // ESP3D_BUTTON_CAPTIONS
  if (!lv_obj_is_valid(btn_up)) {
    esp3d_log_e("Failed to create up button");
    return;
  }
  lv_obj_align_to(btn_up, label_babystep_total, LV_ALIGN_OUT_BOTTOM_MID, 0,
                  ESP3D_BUTTON_PRESSED_OUTLINE);
  ESP3D_SET_CAPTION(btn_up, caption_raise);
  lv_obj_add_event_cb(btn_up, babystep_btn_up_event_cb, LV_EVENT_CLICKED,
                      NULL);

#if ESP3D_BUTTON_CAPTIONS
  lv_obj_t *btn_down = symbolButton::create(ui_new_screen, LV_SYMBOL_DOWN);
#else
  lv_obj_t *btn_down =
      symbolButton::create(ui_new_screen, LV_SYMBOL_MINUS "\n" LV_SYMBOL_DOWN);
#endif  // ESP3D_BUTTON_CAPTIONS
  if (!lv_obj_is_valid(btn_down)) {
    esp3d_log_e("Failed to create down button");
    return;
  }
  lv_obj_align_to(btn_down, btn_up, LV_ALIGN_OUT_BOTTOM_MID, 0,
                  ESP3D_BUTTON_PRESSED_OUTLINE);
  ESP3D_SET_CAPTION(btn_down, caption_lower);
  lv_obj_add_event_cb(btn_down, babystep_btn_down_event_cb, LV_EVENT_CLICKED,
                      NULL);

  // Save (M500)
  lv_obj_t *btn_save = symbolButton::create(ui_new_screen, LV_SYMBOL_SAVE);
  if (!lv_obj_is_valid(btn_save)) {
    esp3d_log_e("Failed to create save button");
    return;
  }
  lv_obj_align_to(btn_save, btn_up, LV_ALIGN_OUT_RIGHT_MID,
                  ESP3D_BUTTON_PRESSED_OUTLINE * 3, 0);
  ESP3D_SET_CAPTION(btn_save, caption_save);
  lv_obj_add_event_cb(btn_save, babystep_btn_save_event_cb, LV_EVENT_CLICKED,
                      NULL);

  esp3dTftui.set_current_screen(ESP3DScreenType::babystep);
}
}  // namespace babystepScreen
