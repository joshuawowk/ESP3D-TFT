/*
  esp3d_screen_snapshot.cpp - GET /snapshot : current screen as a BMP image

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

#include "http/esp3d_http_service.h"

#if ESP3D_SCREEN_SNAPSHOT_FEATURE
#include <string.h>

#include "authentication/esp3d_authentication.h"
#include "esp3d_log.h"
#include "esp3d_tft_ui.h"
#include "esp_heap_caps.h"
#include "lvgl.h"

#define SNAPSHOT_LOCK_TIMEOUT_MS 2000
#define SNAPSHOT_ROWS_PER_CHUNK 16

static void put_le16(uint8_t *p, uint16_t v) {
  p[0] = v & 0xFF;
  p[1] = (v >> 8) & 0xFF;
}

static void put_le32(uint8_t *p, uint32_t v) {
  p[0] = v & 0xFF;
  p[1] = (v >> 8) & 0xFF;
  p[2] = (v >> 16) & 0xFF;
  p[3] = (v >> 24) & 0xFF;
}

// Render the active screen with lv_snapshot and stream it as a top-down,
// 16-bit RGB565 (BI_BITFIELDS) BMP.
esp_err_t ESP3DHttpService::screen_snapshot_handler(httpd_req_t *req) {
  esp3d_log("Uri: %s", req->uri);
  ESP3DAuthenticationLevel authentication_level = getAuthenticationLevel(req);
  httpd_resp_set_http_hdr(req);
#if ESP3D_AUTHENTICATION_FEATURE
  if (authentication_level == ESP3DAuthenticationLevel::guest) {
    return not_authenticated_handler(req);
  }
#else
  (void)authentication_level;
#endif  // ESP3D_AUTHENTICATION_FEATURE

  if (!esp3dTftui.lockGui(SNAPSHOT_LOCK_TIMEOUT_MS)) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Display busy");
    return ESP_FAIL;
  }
  lv_obj_t *screen = lv_scr_act();
  uint32_t buf_size =
      lv_snapshot_buf_size_needed(screen, LV_IMG_CF_TRUE_COLOR);
  uint8_t *buf = (uint8_t *)heap_caps_malloc(
      buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  lv_img_dsc_t dsc;
  lv_res_t res = LV_RES_INV;
  if (buf) {
    res = lv_snapshot_take_to_buf(screen, LV_IMG_CF_TRUE_COLOR, &dsc, buf,
                                  buf_size);
  }
  esp3dTftui.unlockGui();

  if (!buf || res != LV_RES_OK) {
    esp3d_log_e("Snapshot failed (buffer %u bytes)", (unsigned int)buf_size);
    if (buf) heap_caps_free(buf);
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        buf ? "Snapshot failed" : "Not enough memory");
    return ESP_FAIL;
  }

  const uint32_t width = dsc.header.w;
  const uint32_t height = dsc.header.h;
  const uint32_t src_stride = width * sizeof(lv_color_t);
  const uint32_t row_size = (width * 2 + 3) & ~3u;  // BMP rows are 4-byte aligned
  const uint32_t image_size = row_size * height;
  const uint32_t data_offset = 14 + 40 + 12;  // file + info headers + masks

  uint8_t header[14 + 40 + 12];
  memset(header, 0, sizeof(header));
  header[0] = 'B';
  header[1] = 'M';
  put_le32(&header[2], data_offset + image_size);
  put_le32(&header[10], data_offset);
  put_le32(&header[14], 40);                      // BITMAPINFOHEADER size
  put_le32(&header[18], width);
  put_le32(&header[22], (uint32_t)(-(int32_t)height));  // top-down
  put_le16(&header[26], 1);                       // planes
  put_le16(&header[28], 16);                      // bits per pixel
  put_le32(&header[30], 3);                       // BI_BITFIELDS
  put_le32(&header[34], image_size);
  put_le32(&header[38], 2835);                    // 72 dpi
  put_le32(&header[42], 2835);
  put_le32(&header[54], 0xF800);                  // red mask
  put_le32(&header[58], 0x07E0);                  // green mask
  put_le32(&header[62], 0x001F);                  // blue mask

  httpd_resp_set_type(req, "image/bmp");
  httpd_resp_set_hdr(req, "Content-Disposition",
                     "inline; filename=\"esp3d-tft-screen.bmp\"");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache");

  esp_err_t err = httpd_resp_send_chunk(req, (const char *)header,
                                        sizeof(header));
  uint8_t *chunk = (uint8_t *)heap_caps_malloc(
      row_size * SNAPSHOT_ROWS_PER_CHUNK, MALLOC_CAP_8BIT);
  if (!chunk) {
    err = ESP_ERR_NO_MEM;
  }
  for (uint32_t y = 0; err == ESP_OK && y < height;
       y += SNAPSHOT_ROWS_PER_CHUNK) {
    uint32_t rows = height - y;
    if (rows > SNAPSHOT_ROWS_PER_CHUNK) rows = SNAPSHOT_ROWS_PER_CHUNK;
    memset(chunk, 0, row_size * rows);
    for (uint32_t r = 0; r < rows; r++) {
      const lv_color_t *src =
          (const lv_color_t *)(buf + (y + r) * src_stride);
      uint8_t *dst = chunk + r * row_size;
      for (uint32_t x = 0; x < width; x++) {
        uint16_t px = lv_color_to16(src[x]);
#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP
        px = (uint16_t)((px >> 8) | (px << 8));
#endif
        put_le16(dst + x * 2, px);
      }
    }
    err = httpd_resp_send_chunk(req, (const char *)chunk, row_size * rows);
  }
  if (chunk) heap_caps_free(chunk);
  heap_caps_free(buf);
  if (err != ESP_OK) {
    esp3d_log_e("Snapshot send failed: %s", esp_err_to_name(err));
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_FAIL;
  }
  return httpd_resp_send_chunk(req, NULL, 0);
}
#endif  // ESP3D_SCREEN_SNAPSHOT_FEATURE

#if ESP3D_REMOTE_TOUCH_FEATURE
#include <stdlib.h>
#include <string.h>

#include "authentication/esp3d_authentication.h"
#include "esp3d_log.h"
#include "esp3d_tft_ui.h"

// Read an integer from the URL query string (GET parameters)
static bool query_int(httpd_req_t *req, const char *key, int32_t *value) {
  char query[96];
  char field[16];
  size_t len = httpd_req_get_url_query_len(req);
  if (len == 0 || len >= sizeof(query)) {
    return false;
  }
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
      httpd_query_key_value(query, key, field, sizeof(field)) != ESP_OK) {
    return false;
  }
  char *end = nullptr;
  long v = strtol(field, &end, 10);
  if (end == field) {
    return false;
  }
  *value = (int32_t)v;
  return true;
}

// GET /touch?x=<px>&y=<px>[&ms=<press duration>][&x2=<px>&y2=<px>] : tap
// (or swipe to x2,y2) the screen remotely
esp_err_t ESP3DHttpService::screen_touch_handler(httpd_req_t *req) {
  esp3d_log("Uri: %s", req->uri);
  ESP3DAuthenticationLevel authentication_level = getAuthenticationLevel(req);
  httpd_resp_set_http_hdr(req);
#if ESP3D_AUTHENTICATION_FEATURE
  if (authentication_level == ESP3DAuthenticationLevel::guest) {
    return not_authenticated_handler(req);
  }
#else
  (void)authentication_level;
#endif  // ESP3D_AUTHENTICATION_FEATURE
  int32_t x = 0, y = 0, ms = 120;
  if (!query_int(req, "x", &x) || !query_int(req, "y", &y)) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing x or y");
    return ESP_FAIL;
  }
  bool has_ms = query_int(req, "ms", &ms);  // optional press duration
  if (ms < 0) ms = 0;
  // optional end point: swipe from (x, y) to (x2, y2)
  int32_t x2 = -1, y2 = -1;
  bool swipe = query_int(req, "x2", &x2) && query_int(req, "y2", &y2);
  if (swipe && !has_ms) {
    ms = 400;  // default swipe duration
  }
  if (!esp3dTftui.remoteTouch(x, y, (uint32_t)ms, swipe ? x2 : -1,
                              swipe ? y2 : -1)) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid position");
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "text/plain");
  return httpd_resp_sendstr(req, "ok");
}
#endif  // ESP3D_REMOTE_TOUCH_FEATURE
