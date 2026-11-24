/*
  esp3d_tft_ui

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
#include <stdio.h>

#include "lvgl.h"
#include "screens/esp3d_screen_type.h"

#if LV_USE_SNAPSHOT && ESP3D_HTTP_FEATURE
// Remote screen: GET /snapshot (screen capture) and GET /touch (tap)
#define ESP3D_REMOTE_TOUCH_FEATURE 1
#endif  // LV_USE_SNAPSHOT && ESP3D_HTTP_FEATURE

class ESP3DTftUi final {
 public:
  ESP3DTftUi();
  ~ESP3DTftUi();
  bool begin();
  void handle();
  bool end();
  void set_current_screen(ESP3DScreenType screen) { _current_screen = screen; }
  ESP3DScreenType get_current_screen() { return _current_screen; }
  // Lock/unlock LVGL for calls made outside the UI task
  bool lockGui(uint32_t timeout_ms);
  void unlockGui();
#if ESP3D_REMOTE_TOUCH_FEATURE
  // Press the screen at (x, y) for duration_ms through a virtual pointer
  // device, then wait until LVGL has read the release.
  bool remoteTouch(int32_t x, int32_t y, uint32_t duration_ms);
#endif  // ESP3D_REMOTE_TOUCH_FEATURE

 private:
  ESP3DScreenType _current_screen = ESP3DScreenType::none;
  bool _started;
};

extern ESP3DTftUi esp3dTftui;
