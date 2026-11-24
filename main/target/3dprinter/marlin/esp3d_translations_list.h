/*
  esp3d_translation_service

  Copyright (c) 2023 Luc Lebosse. All rights reserved.

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

#ifdef __cplusplus
extern "C" {
#endif
// Do not change order and avoid comment
enum class ESP3DLabel : uint16_t {
  language = 0,
  size_for_update,
  screen,
  architecture,
  sdk_version,
  cpu_freq,
  flash_size,
  free_heap,
  total_psram,
  version,
  sd_updater,
  on,
  off,
  millimeters,
  celsius,
  flash_type,
  confirmation,
  stop_print,
  error,
  error_applying_mode,
  error_applying_setting,
  hostname,
  extensions,
  please_wait,
  output_client,
  usb,
  serial,
  serial_baud_rate,
  usb_baud_rate,
  connecting,
  not_connected,
  ap_mode,
  ip_lost,
  jog_type,
  absolute_short,
  absolute,
  relative_short,
  relative,
  polling,
  enabled,
  disabled,
  motors_disabled,
  fan_controls,
  information,
  auto_leveling,
  manual_leveling_help,
  manual_leveling_text,
  bed_width,
  bed_depth,
  auto_bed_probing,
  start_probing,
  invert_axis,
  ui_language,
  no_sd_card,
  communication_lost,
  communication_recovered,
  ms,
  days,
  streaming_error,
  command_error,
  stream_error,
  target_firmware,
  // Button captions (480x480 UI)
  caption_nozzle,
  caption_nozzle_2,
  caption_bed,
  caption_position,
  caption_fan,
  caption_speed,
  caption_pause,
  caption_resume,
  caption_stop,
  caption_files,
  caption_menu,
  caption_filament,
  caption_leveling,
  caption_motors_off,
  caption_wifi,
  caption_settings,
  caption_info,
  caption_back,
  caption_apply,
  caption_off,
  caption_all_off,
  caption_home,
  caption_home_all,
  caption_reset,
  caption_edit,
  caption_manual,
  caption_auto,
  caption_previous,
  caption_next,
  caption_help,
  caption_start,
  caption_access_point,
  caption_client,
  caption_no_wifi,
  caption_save,
  caption_scan,
  caption_refresh,
  caption_connect,
  unknown_index,  // must be last
};

#ifdef __cplusplus
}  // extern "C"
#endif