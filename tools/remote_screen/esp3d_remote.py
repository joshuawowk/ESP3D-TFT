#!/usr/bin/env python3
"""Drive an ESP3D-TFT display over HTTP (standard library only).

  esp3d_remote.py <host> snapshot [out.bmp]      save the screen (GET /snapshot)
  esp3d_remote.py <host> tap X Y [MS]            tap (GET /touch)
  esp3d_remote.py <host> swipe X Y X2 Y2 [MS]    swipe, e.g. to scroll a list
  esp3d_remote.py <host> cmd "[ESP420]"          send a command (GET /command)
  esp3d_remote.py <host> ota firmware.bin        flash firmware (POST /updatefw)

/snapshot and /touch need a firmware built with LV_USE_SNAPSHOT (see
docs/remote-screen.md). Authentication is not handled.
"""
import os
import sys
import urllib.parse
import urllib.request
import uuid


def get(host, path, params=None, timeout=30):
    url = f'http://{host}{path}'
    if params:
        url += '?' + urllib.parse.urlencode(params)
    with urllib.request.urlopen(url, timeout=timeout) as resp:
        return resp.read()


def ota(host, firmware):
    """Same multipart layout as the ESP3D web UI: path, <file>S size, file."""
    name = os.path.basename(firmware)
    data = open(firmware, 'rb').read()
    boundary = uuid.uuid4().hex
    parts = []
    for field, value in (('path', '/'), (f'/{name}S', str(len(data)))):
        parts.append(f'--{boundary}\r\nContent-Disposition: form-data; '
                     f'name="{field}"\r\n\r\n{value}\r\n'.encode())
    parts.append(f'--{boundary}\r\nContent-Disposition: form-data; '
                 f'name="myfile"; filename="{name}"\r\n'
                 'Content-Type: application/octet-stream\r\n\r\n'.encode())
    body = b''.join(parts) + data + f'\r\n--{boundary}--\r\n'.encode()
    req = urllib.request.Request(
        f'http://{host}/updatefw', data=body, method='POST',
        headers={'Content-Type': f'multipart/form-data; boundary={boundary}'})
    with urllib.request.urlopen(req, timeout=300) as resp:
        return resp.read()


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__)
    host, action, rest = argv[1], argv[2], argv[3:]
    if action == 'snapshot':
        out = rest[0] if rest else 'esp3d-tft-screen.bmp'
        open(out, 'wb').write(get(host, '/snapshot'))
        print(out)
    elif action == 'tap' and len(rest) >= 2:
        params = {'x': rest[0], 'y': rest[1]}
        if len(rest) > 2:
            params['ms'] = rest[2]
        print(get(host, '/touch', params).decode())
    elif action == 'swipe' and len(rest) >= 4:
        params = dict(zip(('x', 'y', 'x2', 'y2'), rest[:4]))
        if len(rest) > 4:
            params['ms'] = rest[4]
        print(get(host, '/touch', params).decode())
    elif action == 'cmd' and rest:
        print(get(host, '/command', {'cmd': rest[0]}).decode())
    elif action == 'ota' and rest:
        print(ota(host, rest[0]).decode())
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main(sys.argv)
