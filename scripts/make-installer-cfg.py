#!/usr/bin/env python3
"""Derive installer/Beam68-base.cfg from Beam68.cfg.

The GUI installer writes a fresh configuration as this base file plus one
PLUGIN= line per plugin the user chose to install, so deselected plugins
never appear as broken entries.  Run after editing Beam68.cfg; the host
tests fail when the two files are out of sync.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def base_text():
    lines = (ROOT / "Beam68.cfg").read_text(encoding="ascii").splitlines()
    kept = [line for line in lines if not line.startswith("PLUGIN=")]
    kept = [line.replace("# Plugin executables are resolved relative to the launcher, NOT Prefs.",
                         "# Plugins chosen in the installer follow (PLUGIN=, one per package).")
            for line in kept]
    return "\n".join(kept).rstrip("\n") + "\n"


if __name__ == "__main__":
    target = ROOT / "installer" / "Beam68-base.cfg"
    target.write_text(base_text(), encoding="ascii")
    print("Wrote", target)
