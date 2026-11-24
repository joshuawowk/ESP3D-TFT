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

#include "symbol_button_component.h"

#
#include "esp3d_log.h"
#include "esp3d_tft_ui.h"
#include "symbol_button_component.h"

/**********************
 *  Namespace
 **********************/
namespace symbolButton {

lv_obj_t *create(lv_obj_t *container, const char *text, int width, int height,
                 bool center, bool slash, int rotation) {
  lv_obj_t *btn = lv_btn_create(container);
  if (!lv_obj_is_valid(btn)) {
    esp3d_log_e("Failed to create button");
    return nullptr;
  }
  ESP3DStyle::apply(btn, ESP3DStyleType::button);
  if (width != -1) lv_obj_set_width(btn, width);
  if (height != -1) lv_obj_set_height(btn, height);
  lv_obj_t *label = lv_label_create(btn);
  if (!lv_obj_is_valid(label)) {
    esp3d_log_e("Failed to create label");
    return nullptr;
  }
  lv_label_set_text(label, text);
  lv_obj_update_layout(label);

  int32_t label_width = lv_obj_get_width(label);
  int32_t label_height = lv_obj_get_height(label);

  lv_obj_center(label);
  if (center) lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

  if (rotation != 0) {
    lv_obj_set_style_transform_pivot_x(label, label_width / 2, 0);
    lv_obj_set_style_transform_pivot_y(label, label_height / 2, 0);
    lv_obj_set_style_transform_angle(label, rotation * 10, 0);
    lv_obj_center(label);
  }

  if (slash) {
    lv_obj_t *label2 = lv_label_create(btn);
    if (!lv_obj_is_valid(label2)) {
      esp3d_log_e("Failed to create label");
      return nullptr;
    }
    lv_label_set_text(label2, LV_SYMBOL_SLASH);
    lv_obj_center(label2);
  }
  return btn;
}

#if ESP3D_BUTTON_CAPTIONS
void setCaption(lv_obj_t *btn, const char *caption) {
  if (!lv_obj_is_valid(btn) || caption == nullptr || caption[0] == '\0') {
    return;
  }
  uint32_t count = lv_obj_get_child_cnt(btn);
  // Content is centered, so side padding only steals caption width
  lv_obj_set_style_pad_left(btn, 1, LV_PART_MAIN);
  lv_obj_set_style_pad_right(btn, 1, LV_PART_MAIN);
  // The caption is the last child: live values keep updating child 0.
  // It wraps onto a second line if it does not fit the button width.
  lv_obj_t *label = lv_label_create(btn);
  if (!lv_obj_is_valid(label)) {
    esp3d_log_e("Failed to create caption label");
    return;
  }
  lv_obj_set_style_text_font(label, ESP3D_BUTTON_CAPTION_FONT, 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(label, lv_pct(100));
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_label_set_text(label, caption);
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_update_layout(label);
  lv_coord_t caption_height = lv_obj_get_height(label);
  // Shift the existing content (symbol/value, slash overlay) up to make room
  for (uint32_t i = 0; i < count; i++) {
    lv_obj_align(lv_obj_get_child(btn, i), LV_ALIGN_CENTER, 0,
                 -(caption_height + ESP3D_BUTTON_CAPTION_GAP) / 2);
  }
}
#endif  // ESP3D_BUTTON_CAPTIONS
}  // namespace symbolButton