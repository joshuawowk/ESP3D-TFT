/*
 symbol_button_component.h - ESP3D screens definition

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

#pragma once

#include <lvgl.h>

#include "esp3d_styles.h"

// Size of the symbol buttons that sit next to a button matrix (home all
// axes, all heaters off). Defaults to the matrix button size; a resolution
// with captions can make them larger.
#ifndef ESP3D_MATRIX_SYMBOL_BUTTON_WIDTH
#define ESP3D_MATRIX_SYMBOL_BUTTON_WIDTH ESP3D_MATRIX_BUTTON_WIDTH
#endif
#ifndef ESP3D_MATRIX_SYMBOL_BUTTON_HEIGHT
#define ESP3D_MATRIX_SYMBOL_BUTTON_HEIGHT ESP3D_MATRIX_BUTTON_HEIGHT
#endif

namespace symbolButton {
lv_obj_t *create(lv_obj_t *container, const char *text,
                 int width = ESP3D_SYMBOL_BUTTON_WIDTH,
                 int height = ESP3D_SYMBOL_BUTTON_HEIGHT, bool center = true,
                 bool slash = false, int rotation = 0);
#if ESP3D_BUTTON_CAPTIONS
// Add a one-line text caption at the bottom of a button and move its
// existing content (symbol/value label, slash overlay) up to make room.
void setCaption(lv_obj_t *btn, const char *caption);
#endif  // ESP3D_BUTTON_CAPTIONS
}  // namespace symbolButton

// Captions are enabled per resolution (ESP3D_BUTTON_CAPTIONS in
// esp3d_styles_res.h). When disabled this expands to nothing, so the
// caption labels never need to exist in that firmware's translations list.
#if ESP3D_BUTTON_CAPTIONS
#include "translations/esp3d_translation_service.h"
#define ESP3D_SET_CAPTION(btn, label)                                       \
  symbolButton::setCaption((btn),                                           \
                           esp3dTranslationService.translate(ESP3DLabel::label))
#else
#define ESP3D_SET_CAPTION(btn, label)
#endif  // ESP3D_BUTTON_CAPTIONS
