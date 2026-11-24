#!/usr/bin/env python3
"""Run the Marlin simulator on an explicit serial port.

Unlike fw_simulator.py (which picks the first port whose description contains
"SERIAL"), this takes the port on the command line, so it cannot grab the
wrong device. Optionally slows G0/G1 replies to simulate motion, which keeps
a streamed test print running long enough to look at the print screens.

Usage:
  fw_simulator_port.py <port> [--baud 115200] [--move-delay 0.25] [--seconds N]

Example with the USB host bridge in bench mode (tools/usb_host_bridge):
  fw_simulator_port.py /dev/ttyACM0 --move-delay 0.25
"""
import argparse
import os
import re
import sys
import time

import serial

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import esp3d_common as common  # noqa: E402
import marlin  # noqa: E402

GCODE_LINE = re.compile(r'^[GMTN][0-9]')
MOVE = re.compile(r'(^|\s)G[01](\s|$)')


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('port')
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--move-delay', type=float, default=0.0,
                        help='seconds to wait before acknowledging G0/G1')
    parser.add_argument('--seconds', type=float, default=0,
                        help='stop after this many seconds (0 = run forever)')
    args = parser.parse_args()

    ser = serial.Serial()
    ser.port = args.port
    ser.baudrate = args.baud
    ser.timeout = 0.2
    ser.dtr = False  # don't reset whatever is on the other end
    ser.rts = False
    ser.open()
    start = time.time()
    try:
        while not args.seconds or time.time() - start < args.seconds:
            if not ser.in_waiting:
                time.sleep(0.01)
                continue
            line = ser.readline().decode('utf-8', 'replace').strip()
            if not line:
                continue
            print(f'{time.time() - start:7.1f} < {line}', flush=True)
            if not GCODE_LINE.match(line):
                continue  # boot banner, logs, echoes
            if args.move_delay and MOVE.search(line):
                time.sleep(args.move_delay)
            response = marlin.processLine(line, ser)
            if response:
                common.send_echo(ser, response)
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()


if __name__ == '__main__':
    main()
