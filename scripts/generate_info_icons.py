#!/usr/bin/env python3
"""Generate Beam68's palette-safe classic Workbench icons.

WBTOOL icons for Beam68 and Beam68Prefs, plus WBPROJECT icons for the GUI
installer (Install.info) and the WBStartup entry (installer/Beam68Start.info).
"""

from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parent.parent
WIDTH = 54
HEIGHT = 22


class Canvas:
    def __init__(self):
        self.pixels = [[0 for _ in range(WIDTH)] for _ in range(HEIGHT)]

    def pixel(self, x, y, colour):
        if 0 <= x < WIDTH and 0 <= y < HEIGHT:
            self.pixels[y][x] = colour

    def rect(self, x1, y1, x2, y2, colour):
        for y in range(y1, y2 + 1):
            for x in range(x1, x2 + 1):
                self.pixel(x, y, colour)

    def line(self, x1, y1, x2, y2, colour):
        dx = abs(x2 - x1)
        sx = 1 if x1 < x2 else -1
        dy = -abs(y2 - y1)
        sy = 1 if y1 < y2 else -1
        error = dx + dy
        while True:
            self.pixel(x1, y1, colour)
            if x1 == x2 and y1 == y2:
                return
            twice = 2 * error
            if twice >= dy:
                error += dy
                x1 += sx
            if twice <= dx:
                error += dx
                y1 += sy

    def remap(self, colours):
        selected = Canvas()
        selected.pixels = [
            [colours.get(pixel, pixel) for pixel in row]
            for row in self.pixels
        ]
        return selected


def launcher_art():
    c = Canvas()
    # A compact launcher window: title beam, prompt, result and magnifier.
    c.rect(2, 2, 51, 19, 1)
    c.rect(4, 4, 49, 17, 3)
    c.rect(4, 4, 49, 6, 2)
    c.line(7, 8, 11, 10, 2)
    c.line(11, 10, 7, 12, 2)
    c.rect(14, 9, 31, 10, 1)
    c.rect(14, 13, 27, 14, 2)
    c.rect(14, 16, 23, 16, 1)
    for x in range(34, 43):
        for y in range(7, 16):
            if 9 <= (x - 38) * (x - 38) + (y - 11) * (y - 11) <= 20:
                c.pixel(x, y, 2)
    c.line(42, 15, 48, 20, 2)
    c.line(43, 15, 49, 20, 1)
    c.line(22, 1, 31, 1, 2)
    return c


def prefs_art():
    c = Canvas()
    # Two-pane Preferences window with sidebar sections and slider controls.
    c.rect(2, 2, 51, 20, 1)
    c.rect(4, 4, 49, 18, 3)
    c.rect(4, 4, 49, 6, 2)
    c.rect(4, 7, 16, 18, 2)
    c.rect(6, 9, 14, 10, 1)
    c.rect(6, 12, 13, 13, 3)
    c.rect(6, 15, 12, 16, 3)
    for y in (9, 13, 17):
        c.line(20, y, 46, y, 1)
    c.rect(26, 7, 30, 11, 2)
    c.rect(38, 11, 42, 15, 2)
    c.rect(31, 15, 35, 19, 2)
    c.rect(46, 4, 48, 5, 1)
    return c


def append_image(data, canvas):
    data.extend(struct.pack(">hhhhhIBBI", 0, 0, WIDTH, HEIGHT, 2, 1, 3, 0, 0))
    row_words = (WIDTH + 15) // 16
    for plane in range(2):
        for y in range(HEIGHT):
            for word_index in range(row_words):
                word = 0
                for bit in range(16):
                    x = word_index * 16 + bit
                    if x < WIDTH and canvas.pixels[y][x] & (1 << plane):
                        word |= 1 << (15 - bit)
                data.extend(struct.pack(">H", word))


def installer_art():
    c = Canvas()
    # A disk with an arrow into the launcher window: "install".
    c.rect(2, 2, 25, 20, 1)
    c.rect(4, 4, 23, 11, 3)
    c.rect(8, 14, 19, 19, 2)
    c.rect(15, 15, 17, 18, 1)
    c.line(27, 11, 34, 11, 2)
    c.line(31, 8, 34, 11, 2)
    c.line(31, 14, 34, 11, 2)
    c.rect(36, 4, 51, 19, 1)
    c.rect(38, 6, 49, 17, 3)
    c.rect(38, 6, 49, 8, 2)
    c.rect(40, 11, 47, 12, 1)
    c.rect(40, 14, 45, 15, 2)
    return c


def guide_art():
    c = Canvas()
    # An open book.
    c.rect(4, 3, 26, 19, 1)
    c.rect(27, 3, 49, 19, 1)
    c.rect(6, 4, 25, 17, 3)
    c.rect(28, 4, 47, 17, 3)
    for y in (7, 10, 13):
        c.line(9, y, 22, y, 2)
        c.line(31, y, 44, y, 2)
    c.line(26, 2, 26, 20, 2)
    return c


def amiga_string(text):
    raw = text.encode("ascii") + b"\0"
    return struct.pack(">I", len(raw)) + raw


def project_object(canvas, default_tool, tooltypes, stack=16384):
    """Classic WBPROJECT DiskObject with a default tool and tool types."""
    data = bytearray(disk_object(canvas))
    data[48] = 4  # WBPROJECT
    struct.pack_into(">I", data, 50, 1)  # do_DefaultTool present
    struct.pack_into(">I", data, 54, 1 if tooltypes else 0)
    struct.pack_into(">I", data, 74, stack)
    data.extend(amiga_string(default_tool))
    if tooltypes:
        data.extend(struct.pack(">I", (len(tooltypes) + 1) * 4))
        for entry in tooltypes:
            data.extend(amiga_string(entry))
    return bytes(data)


def disk_object(canvas):
    # Classic planar DiskObject: WBTOOL, two bitplanes, normal and selected
    # imagery, and a 16K stack. Zero remains transparent on Workbench.
    selected = canvas.remap({0: 0, 1: 2, 2: 3, 3: 1})
    data = bytearray(78)
    struct.pack_into(">HH", data, 0, 0xE310, 1)
    struct.pack_into(">I", data, 4, 0)
    struct.pack_into(">hhhh", data, 8, 0, 0, WIDTH, HEIGHT)
    struct.pack_into(">HHH", data, 16, 0x0006, 0x0001, 0x0001)
    struct.pack_into(">I", data, 22, 1)
    struct.pack_into(">I", data, 26, 1)
    struct.pack_into(">I", data, 44, 1)
    data[48] = 3
    struct.pack_into(">II", data, 58, 0x80000000, 0x80000000)
    struct.pack_into(">I", data, 74, 16384)
    append_image(data, canvas)
    append_image(data, selected)
    return bytes(data)


def main():
    for path, art in ((ROOT / "Beam68.info", launcher_art()),
                      (ROOT / "Beam68Prefs.info", prefs_art())):
        path.write_bytes(disk_object(art))
        print("Created", path)
    # GUI installer: Workbench runs the Installer utility on the script.
    install = ROOT / "Install.info"
    install.write_bytes(project_object(installer_art(), "Installer", [
        "APPNAME=Beam68", "SCRIPT=Install", "MINUSER=AVERAGE",
        "DEFUSER=AVERAGE"]))
    print("Created", install)
    # WBStartup entry; the installer sets the real Default Tool path.
    (ROOT / "installer").mkdir(exist_ok=True)
    startup = ROOT / "installer" / "Beam68Start.info"
    startup.write_bytes(project_object(launcher_art(), "SYS:Utilities/Beam68/Beam68",
                                       ["DONOTWAIT"]))
    print("Created", startup)
    # AmigaGuide manuals open with MultiView (AmigaOS 3.0+).
    for guide in (ROOT / "Beam68.guide.info", ROOT / "sdk" / "Beam68PluginSDK.guide.info"):
        guide.write_bytes(project_object(guide_art(), "MultiView", []))
        print("Created", guide)


if __name__ == "__main__":
    main()
