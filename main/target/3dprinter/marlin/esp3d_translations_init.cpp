/*
  esp3d_translation_service
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

#include <cstdlib>
#include <map>
#include <ranges>

#include "translations/esp3d_translation_service.h"

void ESP3DTranslationService::init() {
  _translations = {
      {ESP3DLabel::language, "English"},
      {ESP3DLabel::version, "Version"},
      {ESP3DLabel::size_for_update, "Size for updates"},
      {ESP3DLabel::screen, "Screen"},
      {ESP3DLabel::architecture, "Arch"},
      {ESP3DLabel::sdk_version, "SDK"},
      {ESP3DLabel::cpu_freq, "Freq"},
      {ESP3DLabel::flash_size, "Flash Size"},
      {ESP3DLabel::free_heap, "Free Heap"},
      {ESP3DLabel::total_psram, "PSRAM Size"},
      {ESP3DLabel::sd_updater, "SD updater"},
      {ESP3DLabel::on, "On"},
      {ESP3DLabel::off, "Off"},
      {ESP3DLabel::millimeters, "mm"},
      {ESP3DLabel::celsius, "°C"},
      {ESP3DLabel::flash_type, "Flash Filesystem"},
      {ESP3DLabel::confirmation, "Please confirm"},
      {ESP3DLabel::stop_print, "Do you want to stop current print?"},
      {ESP3DLabel::error, "Error"},
      {ESP3DLabel::error_applying_mode, "Apply mode failed!"},
      {ESP3DLabel::error_applying_setting, "Apply setting failed!"},
      {ESP3DLabel::hostname, "Hostname"},
      {ESP3DLabel::extensions, "Extensions"},
      {ESP3DLabel::please_wait, "Please wait..."},
      {ESP3DLabel::output_client, "Output port"},
      {ESP3DLabel::serial_baud_rate, "Serial baud rate"},
      {ESP3DLabel::usb_baud_rate, "USB serial baud rate"},
      {ESP3DLabel::usb, "USB"},
      {ESP3DLabel::connecting, "Connecting to wifi..."},
      {ESP3DLabel::not_connected, "Not connected"},
      {ESP3DLabel::ip_lost, "IP lost"},
      {ESP3DLabel::ap_mode, "Set as access point"},
      {ESP3DLabel::absolute_short, "abs"},
      {ESP3DLabel::absolute, "Absolute"},
      {ESP3DLabel::relative_short, "rel"},
      {ESP3DLabel::relative, "Relative"},
      {ESP3DLabel::jog_type, "Jog type"},
      {ESP3DLabel::polling, "Polling"},
      {ESP3DLabel::enabled, "Enabled"},
      {ESP3DLabel::disabled, "Disabled"},
      {ESP3DLabel::motors_disabled, "Motors disabled"},
      {ESP3DLabel::information, "Information"},
      {ESP3DLabel::fan_controls, "Fan controls"},
      {ESP3DLabel::serial, "Serial"},
      {ESP3DLabel::auto_leveling, "Auto leveling"},
      {ESP3DLabel::manual_leveling_help,
       "Adjust each knobs corresponding to active position until a sheet of "
       "paper just slides amid nozzle and plate.\n"
       "Repeat operation for each point until no correction is needed."},
      {ESP3DLabel::manual_leveling_text,
       "Adjust knobs to level, then click on next.\nYou can also select "
       "manually each point."},
      {ESP3DLabel::bed_width, "Bed width"},
      {ESP3DLabel::bed_depth, "Bed depth"},
      {ESP3DLabel::auto_bed_probing, "Probing location: %s"},
      {ESP3DLabel::invert_axis, "Invert %s axis"},
      {ESP3DLabel::start_probing, "Start probing"},
      {ESP3DLabel::ui_language, "Language"},
      {ESP3DLabel::no_sd_card, "No SD card"},
      {ESP3DLabel::communication_lost, "Communication lost"},
      {ESP3DLabel::communication_recovered, "Communication recovered"},
      {ESP3DLabel::ms, "ms"},
      {ESP3DLabel::days, "days"},
      {ESP3DLabel::streaming_error, "Streaming error, operation aborted!"},
      {ESP3DLabel::command_error, "Command failed: '%s'"},
      {ESP3DLabel::stream_error, "Stream failed: '%s'"},
      {ESP3DLabel::target_firmware, "Target firmware"},
      {ESP3DLabel::caption_nozzle, "Nozzle"},
      {ESP3DLabel::caption_nozzle_2, "Nozzle 2"},
      {ESP3DLabel::caption_bed, "Bed"},
      {ESP3DLabel::caption_position, "Position"},
      {ESP3DLabel::caption_fan, "Fan"},
      {ESP3DLabel::caption_speed, "Speed"},
      {ESP3DLabel::caption_pause, "Pause"},
      {ESP3DLabel::caption_resume, "Resume"},
      {ESP3DLabel::caption_stop, "Stop"},
      {ESP3DLabel::caption_files, "Files"},
      {ESP3DLabel::caption_menu, "Menu"},
      {ESP3DLabel::caption_filament, "Filament"},
      {ESP3DLabel::caption_leveling, "Leveling"},
      {ESP3DLabel::caption_motors_off, "Motors off"},
      {ESP3DLabel::caption_wifi, "Wi-Fi"},
      {ESP3DLabel::caption_settings, "Settings"},
      {ESP3DLabel::caption_info, "Info"},
      {ESP3DLabel::caption_back, "Back"},
      {ESP3DLabel::caption_apply, "Apply"},
      {ESP3DLabel::caption_off, "Off"},
      {ESP3DLabel::caption_all_off, "All off"},
      {ESP3DLabel::caption_home, "Home"},
      {ESP3DLabel::caption_home_all, "Home all"},
      {ESP3DLabel::caption_reset, "Reset"},
      {ESP3DLabel::caption_edit, "Edit"},
      {ESP3DLabel::caption_manual, "Manual"},
      {ESP3DLabel::caption_auto, "Auto"},
      {ESP3DLabel::caption_previous, "Previous"},
      {ESP3DLabel::caption_next, "Next"},
      {ESP3DLabel::caption_help, "Help"},
      {ESP3DLabel::caption_start, "Start"},
      {ESP3DLabel::caption_access_point, "Hotspot"},
      {ESP3DLabel::caption_client, "Client"},
      {ESP3DLabel::caption_no_wifi, "Off"},
      {ESP3DLabel::caption_save, "Save"},
      {ESP3DLabel::caption_scan, "Scan"},
      {ESP3DLabel::caption_refresh, "Refresh"},
      {ESP3DLabel::caption_connect, "Connect"},
      {ESP3DLabel::screen_brightness, "Brightness"},
      {ESP3DLabel::screen_sleep, "Screen sleep"},
      {ESP3DLabel::never, "Never"},
      {ESP3DLabel::minutes_value, "%s min"},
      {ESP3DLabel::preheating, "Preheating %s"},
      {ESP3DLabel::preheat_preset, "Preset %s"},
      {ESP3DLabel::caption_raise, "Raise"},
      {ESP3DLabel::caption_lower, "Lower"},
      {ESP3DLabel::caption_z_offset, "Z offset"},
      {ESP3DLabel::z_offset_saved, "Z offset saved"},
      {ESP3DLabel::time_left, "%s left"},
  };
}
