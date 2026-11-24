# Z offset (babystepping) during a print

On resolutions that enable `ESP3D_BABYSTEP_SCREEN` (currently the 480x480
Marlin UI), the main screen shows a **Z offset** button while a print streamed
by ESP3D-TFT is running (it shares the bottom row with Speed, Pause and Stop).

The Z offset screen:

- **Step**: 0.01, 0.02, 0.05 or 0.1 mm.
- **Raise** / **Lower**: `M290 Z+step` / `M290 Z-step` (nozzle away from /
  closer to the bed).
- The total applied since the print started (reset at the start of each print).
- **Save**: `M500`.

Needs Marlin built with `BABYSTEPPING`. With `BABYSTEP_ZPROBE_OFFSET` the
babysteps also change the probe Z offset (`M851`), so **Save** keeps them for
the next prints. The stock Elegoo Neptune 3 Pro firmware has both.
