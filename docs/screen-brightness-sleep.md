# Screen brightness and sleep

On boards whose BSP provides `bsp_set_backlight()` (currently
`ESP32S3_4848S040C`), the Marlin UI's **Settings** screen has two extra rows:

| Setting | Values | Default |
| --- | --- | --- |
| Brightness | 20 %, 40 %, 60 %, 80 %, 100 % | 100 % |
| Screen sleep | Never, 1, 2, 5, 10, 15, 30, 60 min | Never |

After the chosen time without a touch the backlight turns off. The first touch
only wakes the screen: a transparent overlay catches it, so it does not press
the button underneath. The screen does not go to sleep while a print streamed
by ESP3D-TFT is running or paused.

Both are regular ESP3D settings, so they can also be changed remotely and take
effect immediately:

```
[ESP401]P=55 T=B V=60     brightness 60 %  (10-100)
[ESP401]P=56 T=B V=10     sleep after 10 minutes (0 = never)
```

## Adding a board

Implement in the board's `bsp.c` (and declare in `bsp.h`):

```c
esp_err_t bsp_set_backlight(uint8_t percent);  // 0 = off, 100 = full
```

The UI calls it through a weak reference, so boards without it link unchanged
and simply don't show the two rows.
