# Remote screen: snapshot and touch

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

## Remote touch

```bash
curl "http://<esp3d-tft-ip>/touch?x=380&y=377"          # tap
curl "http://<esp3d-tft-ip>/touch?x=240&y=240&ms=1500"  # long press
curl "http://<esp3d-tft-ip>/touch?x=240&y=330&x2=240&y2=60"  # swipe up (scroll)
```

`GET /touch?x=&y=[&ms=][&x2=&y2=]` presses the screen at `(x, y)` for `ms`
milliseconds (default 120, range 50 to 5000) through a second, virtual LVGL
pointer device, then returns `ok` once LVGL has read the release. With
`x2`/`y2` the press moves to that point over `ms` (default 400): a swipe,
for example to scroll a list. Taps are
serialised, so a script can chain them:

```bash
H=http://<esp3d-tft-ip>
curl -s "$H/touch?x=380&y=377" && curl -s -o menu.bmp "$H/snapshot"
```

Together with `/snapshot`, this drives and checks the whole UI from a PC:
useful for remote support, documentation screenshots and automated UI tests.
It is compiled on the same targets as `/snapshot` (display builds with web
services and `LV_USE_SNAPSHOT 1`). Like `/command`, it is open to anyone on
the network unless ESP3D authentication is enabled.

## Helper script

`tools/remote_screen/esp3d_remote.py` wraps these endpoints (standard library
only):

```bash
python3 tools/remote_screen/esp3d_remote.py <ip> snapshot screen.bmp
python3 tools/remote_screen/esp3d_remote.py <ip> tap 380 378
python3 tools/remote_screen/esp3d_remote.py <ip> swipe 240 330 240 60
python3 tools/remote_screen/esp3d_remote.py <ip> cmd "[ESP420]"
python3 tools/remote_screen/esp3d_remote.py <ip> ota build/ESP3D-TFT.bin
```

## To the SD card

`[ESP216]SNAP` writes the same image as a raw `snapshot<millis>.raw` file to
the display's SD card.

Before this change, `LV_USE_SNAPSHOT` was tested in `esp3d_commands.h` and
`esp216.cpp` before `lvgl.h` was included, so `[ESP216]` was never compiled on
any target. Both now include `lvgl.h` first on display builds.
