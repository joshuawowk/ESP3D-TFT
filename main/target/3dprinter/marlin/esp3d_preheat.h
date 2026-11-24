/*
  esp3d_preheat.h - preheat presets ("name,hotend,bed")

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

#include <stdlib.h>
#include <string.h>

#include <string>

#define ESP3D_PREHEAT_PRESET_COUNT 3
#define ESP3D_PREHEAT_NAME_MAX 8
#define ESP3D_PREHEAT_HOTEND_MAX 300
#define ESP3D_PREHEAT_BED_MAX 150
#define SIZE_OF_PREHEAT_PRESET 24

struct ESP3DPreheatPreset {
  std::string name;
  int hotend = 0;
  int bed = 0;
};

// Parse "PLA,200,60" (spaces around fields allowed). Returns false if the
// format or a value is out of range.
inline bool esp3d_parse_preheat_preset(const char *text,
                                       ESP3DPreheatPreset &preset) {
  if (text == nullptr) return false;
  std::string s(text);
  size_t c1 = s.find(',');
  size_t c2 = (c1 == std::string::npos) ? c1 : s.find(',', c1 + 1);
  if (c1 == std::string::npos || c2 == std::string::npos ||
      s.find(',', c2 + 1) != std::string::npos) {
    return false;
  }
  auto trim = [](std::string v) {
    size_t b = v.find_first_not_of(' ');
    size_t e = v.find_last_not_of(' ');
    return (b == std::string::npos) ? std::string() : v.substr(b, e - b + 1);
  };
  auto to_int = [](const std::string &v, int &out) {
    if (v.empty() || v.size() > 3) return false;
    for (char ch : v) {
      if (ch < '0' || ch > '9') return false;
    }
    out = atoi(v.c_str());
    return true;
  };
  std::string name = trim(s.substr(0, c1));
  int hotend = 0, bed = 0;
  if (name.empty() || name.size() > ESP3D_PREHEAT_NAME_MAX ||
      !to_int(trim(s.substr(c1 + 1, c2 - c1 - 1)), hotend) ||
      !to_int(trim(s.substr(c2 + 1)), bed) ||
      hotend > ESP3D_PREHEAT_HOTEND_MAX || bed > ESP3D_PREHEAT_BED_MAX) {
    return false;
  }
  preset.name = name;
  preset.hotend = hotend;
  preset.bed = bed;
  return true;
}
