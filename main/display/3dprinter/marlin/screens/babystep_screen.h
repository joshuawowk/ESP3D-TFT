/*
  babystep_screen.h - Z offset (babystepping) during a print

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*/

#pragma once

namespace babystepScreen {
// from_menu: Back returns to the menu instead of the main screen
void create(bool from_menu = false);
// Forget the offset applied so far (called when a new print starts)
void reset_total();
}  // namespace babystepScreen
