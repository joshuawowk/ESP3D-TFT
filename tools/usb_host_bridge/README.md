# ESP32-S3 USB host bridge for ESP3D-TFT

Some ESP3D-TFT displays cannot act as a USB host because their USB pins are
used by the screen (for example the Guition ESP32-S3-4848S040, where GPIO19/20
drive touch I2C and RGB data). Their printer link is a 3.3V UART.

Many printers only accept G-code on their USB port. This small firmware turns a
spare ESP32-S3 board with a free native USB port (for example an
ESP32-S3-DevKitC-1) into a transparent bridge:

```
printer USB port ──USB──> ESP32-S3 native USB (host) ──UART──> display serial port
```

The printer firmware stays stock. The bridge supports CH34x, CP210x and FTDI
USB-serial chips (the `usb_host_*_vcp` drivers).

## Printer reset safety

On most printer boards the USB-serial chip's DTR line is wired to the MCU reset
through a capacitor (Arduino-style auto reset). Opening the port and changing
DTR resets the printer, and a reset in the middle of a print aborts it.

The bridge never changes DTR/RTS (`CONFIG_BRIDGE_ASSERT_DTR` is off by
default), so the printer is not reset when the bridge starts, restarts, or
reconnects. The Elegoo Neptune 3 Pro is one of these boards: its CH340G DTR
goes to the STM32 reset through C33.

## Wiring (ESP32-S3-DevKitC-1 + Guition ESP32-S3-4848S040)

| DevKitC-1 | Display P1 connector | Notes |
| --- | --- | --- |
| GPIO17 (UART1 TX) | RX (GPIO44) | bridge → display |
| GPIO18 (UART1 RX) | TX (GPIO43) | display → bridge |
| GND | GND | required |
| — | +5V | leave unconnected (see power) |

Check the P1 silkscreen on your board for pin order (`GND RX TX +5V` on the
boards seen so far).

The printer's USB port (USB-B on the Neptune 3 Pro) plugs into the DevKitC-1
port labelled **USB** (the native ESP32-S3 port), using a USB OTG adapter if
needed. The port labelled **UART** (or COM) is the bridge console and power
input.

## Power

- Power the display from its own USB-C port. Its P1 `+5V` input has no bulk
  capacitance and is known to cause resets, and powering both inputs at once
  back-feeds them.
- Power the bridge from its **UART** USB port (any 5V USB supply).
- The printer's USB-serial chip is powered by the printer, so the bridge does
  not need to supply USB VBUS, and nothing is back-powered. The bridge
  connects when the printer is switched on and reconnects automatically after
  the printer is switched off and on again.

## Status LED

The RGB LED is blue while waiting for the printer and green while the printer
is connected (`CONFIG_BRIDGE_LED_GPIO`, GPIO38 on DevKitC-1 v1.1; v1.0 boards
use GPIO48).

## Build and flash

ESP-IDF v5.1 or newer:

```bash
cd tools/usb_host_bridge
idf.py set-target esp32s3
idf.py build
idf.py -p <port> flash monitor
```

Flash it through the **UART** port. Once running, the native **USB** port is
in host mode and no longer shows up as a serial port on a PC. To flash through
the native USB port again, hold BOOT while pressing RESET.

For bring-up, log every bridged byte on the console:

```bash
idf.py -B build-debug -DSDKCONFIG=build-debug/sdkconfig \
  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.debug" build
```

## Bench mode (no printer needed)

To test the display ↔ bridge wiring before the printer is connected, build
with `sdkconfig.bench`. While no USB device is plugged in, the bridge passes
the display UART straight through to its console port, so a PC on the
**UART** port can play the printer, for example with the Marlin simulator in
`tools/fw_simulator`:

```bash
idf.py -B build-bench -DSDKCONFIG=build-bench/sdkconfig \
  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.bench" build
```

The display should then show the simulator's temperatures. Logging is off in
this mode so it does not mix with the G-code. Reflash the normal build before
connecting the printer (plugging a printer in also takes over from the
passthrough).

## Configuration (`idf.py menuconfig` → USB host bridge)

| Option | Default | |
| --- | --- | --- |
| `BRIDGE_UART_NUM` | 1 | UART wired to the display |
| `BRIDGE_UART_TX_GPIO` / `RX_GPIO` | 17 / 18 | |
| `BRIDGE_UART_BAUDRATE` | 115200 | must match the ESP3D-TFT serial baud rate |
| `BRIDGE_USB_BAUDRATE` | 115200 | must match the printer firmware `BAUDRATE` |
| `BRIDGE_ASSERT_DTR` | off | see Printer reset safety |
| `BRIDGE_LED_GPIO` | 38 | -1 disables the LED |
| `BRIDGE_CONSOLE_PASSTHROUGH` | off | bench mode, see above |
| `BRIDGE_LOG_TRAFFIC` | off | bring-up only |
