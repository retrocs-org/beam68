#!/usr/bin/env python3
"""Validate the shipped Beam68 classic Workbench DiskObjects."""

from pathlib import Path
import struct

root = Path(__file__).resolve().parents[1]
payloads = []
for name in ("Beam68.info", "Beam68Prefs.info"):
    data = (root / name).read_bytes()
    assert len(data) == 822, (name, len(data))
    assert struct.unpack_from(">HH", data, 0) == (0xE310, 1)
    assert struct.unpack_from(">hhhh", data, 8) == (0, 0, 54, 22)
    assert struct.unpack_from(">H", data, 16)[0] == 0x0006
    assert struct.unpack_from(">I", data, 22)[0] != 0
    assert struct.unpack_from(">I", data, 26)[0] != 0
    assert data[48] == 3  # WBTOOL
    assert struct.unpack_from(">I", data, 74)[0] == 16384
    assert struct.unpack_from(">hhhhh", data, 78) == (0, 0, 54, 22, 2)
    assert struct.unpack_from(">hhhhh", data, 450) == (0, 0, 54, 22, 2)
    assert data[98:450] != data[470:822]  # selected artwork is explicit
    assert b"FORM" not in data
    payloads.append(data)
assert payloads[0] != payloads[1]
print("Beam68 and Beam68Prefs Workbench tool icons passed.")
