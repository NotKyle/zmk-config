#!/usr/bin/env python3
"""
lily58-hid-sidecar.py

Reads layer-change packets from the Lily58's raw HID interface and writes
the active layer number to /tmp/.lily58-layer so the Hammerspoon HUD can
read it via a pathwatcher.

Packet format (layer_reporter.c):
  byte 0 = 0x01  (MSG_TYPE_LAYER)
  byte 1 = layer index (0-3)
  bytes 2-31 = 0x00

Usage:
  pip3 install hid
  python3 lily58-hid-sidecar.py
"""

import hid
import time
import sys

USAGE_PAGE = 0xFF60   # zmk-raw-hid default (CONFIG_RAW_HID_USAGE_PAGE)
USAGE      = 0x61     # zmk-raw-hid default (CONFIG_RAW_HID_USAGE)
MSG_LAYER  = 0x01     # matches layer_reporter.c
LAYER_FILE = "/tmp/.lily58-layer"
POLL_MS    = 500      # read timeout — keeps the loop responsive without busy-waiting


def find_device_path():
    for dev in hid.enumerate():
        if dev["usage_page"] == USAGE_PAGE and dev["usage"] == USAGE:
            return dev["path"]
    return None


def write_layer(layer: int) -> None:
    try:
        with open(LAYER_FILE, "w") as f:
            f.write(str(layer))
    except OSError:
        pass


def run() -> None:
    write_layer(0)
    print("[lily58-sidecar] searching for keyboard…", flush=True)

    while True:
        path = find_device_path()
        if not path:
            time.sleep(2)
            continue

        try:
            dev = hid.device()
            dev.open_path(path)
            dev.set_nonblocking(False)
            print("[lily58-sidecar] connected", flush=True)

            while True:
                data = dev.read(32, timeout_ms=POLL_MS)
                if not data:
                    continue
                if data[0] == MSG_LAYER:
                    layer = data[1]
                    write_layer(layer)
                    print(f"[lily58-sidecar] layer → {layer}", flush=True)

        except Exception as exc:
            print(f"[lily58-sidecar] disconnected: {exc}", flush=True)
            write_layer(0)
            time.sleep(2)


if __name__ == "__main__":
    run()
