# Screen snapshot

Targets built with `LV_USE_SNAPSHOT 1` in their `lv_conf.h` (currently
`ESP32S3_4848S040C`) can capture what is on the display.

## Over HTTP

```bash
curl -o screen.bmp http://<esp3d-tft-ip>/snapshot
```

`GET /snapshot` renders the active screen with `lv_snapshot` under the GUI
lock and returns it as a 16-bit RGB565 BMP (480x480 is about 450 KB, about 1.5 s
over Wi-Fi). It needs no SD card. When authentication is enabled, it needs the
same login as the other web endpoints.

Useful for documentation, remote support, and checking UI changes without
looking at the panel.

## To the SD card

`[ESP216]SNAP` writes the same image as a raw `snapshot<millis>.raw` file to
the display's SD card.

Before this change, `LV_USE_SNAPSHOT` was tested in `esp3d_commands.h` and
`esp216.cpp` before `lvgl.h` was included, so `[ESP216]` was never compiled on
any target. Both now include `lvgl.h` first on display builds.
