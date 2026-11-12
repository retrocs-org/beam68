#!/usr/bin/env python3
"""Pack sdk/ as dist/Beam68-PluginSDK-<version>.zip with one top-level drawer."""
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parent.parent
VERSION = (ROOT / "VERSION").read_text(encoding="ascii").strip()
SDK = ROOT / "sdk"
OUT = ROOT / "dist" / ("Beam68-PluginSDK-%s.zip" % VERSION)
TOP = "Beam68-PluginSDK"

OUT.parent.mkdir(exist_ok=True)
with zipfile.ZipFile(OUT, "w", zipfile.ZIP_DEFLATED) as archive:
    for path in sorted(SDK.rglob("*")):
        rel = path.relative_to(SDK)
        if any(p.startswith(".") or p == "__pycache__" for p in rel.parts):
            continue
        if path.is_file() and path.suffix not in (".o", ".pyc", ".lha", ".zip"):
            archive.write(path, (Path(TOP) / rel).as_posix())
print(OUT)
