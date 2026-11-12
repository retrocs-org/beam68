#!/usr/bin/env python3
"""Build a Beam68 plugin/theme catalogue index.

    make-catalogue.py --base-url http://example.org/beam68/ [-o dist/Beam68.catalogue]

Lists every plugin package archive in dist/plugins (made by
`make plugin-packages`) and every theme in themes/, with download URLs
under --base-url.  Upload the index together with those files; users point
Beam68Prefs -> Extras -> Plugin catalogue at the index URL.
"""
import argparse
import io
import os
import re
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FIELD = re.compile(r"^[^|\r\n]*$")


def clean(text, limit):
    text = (text or "").replace("|", "/").strip()
    return text[:limit]


def manifest_from_zip(path):
    with zipfile.ZipFile(path) as archive:
        for name in archive.namelist():
            if name.lower().endswith(".plugin"):
                return archive.read(name).decode("latin-1")
    return None


def manifest_from_drawer(name):
    drawer = ROOT / "Plugins" / name
    manifests = list(drawer.glob("*.plugin")) if drawer.is_dir() else []
    return manifests[0].read_text(encoding="latin-1") if manifests else None


def keys_of(text):
    keys = {}
    for line in text.splitlines():
        line = line.strip().lstrip("#")
        if "=" in line:
            key, value = line.split("=", 1)
            keys.setdefault(key.strip().upper(), value.strip())
    return keys


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--base-url", required=True)
    parser.add_argument("-o", "--output", default=str(ROOT / "dist" / "Beam68.catalogue"))
    args = parser.parse_args(argv)
    base = args.base_url if args.base_url.endswith("/") else args.base_url + "/"
    lines = ["BEAM68CATALOGUE 1"]
    for archive in sorted((ROOT / "dist" / "plugins").glob("Beam68-Plugin-*")):
        text = manifest_from_zip(archive) if archive.suffix == ".zip" else \
            manifest_from_drawer(archive.name.split("-")[2])
        if not text:
            print("skipped (no manifest): %s" % archive.name, file=sys.stderr)
            continue
        k = keys_of(text)
        lines.append("PLUGIN|%s|%s|%s|%s|%s|%s%s" % (
            clean(k.get("ID"), 63), clean(k.get("NAME"), 47), clean(k.get("PLUGIN_VERSION"), 15),
            clean(k.get("AUTHOR"), 47), clean(k.get("DESCRIPTION"), 119), base, archive.name))
    for theme in sorted((ROOT / "themes").glob("*.theme")):
        k = keys_of(theme.read_text(encoding="latin-1"))
        lines.append("THEME|%s|%s|%s|%s|%s|%sthemes/%s" % (
            clean(k.get("ID"), 63), clean(k.get("NAME", theme.stem), 47),
            clean(k.get("VERSION", "1.0"), 15), clean(k.get("AUTHOR"), 47),
            clean(k.get("DESCRIPTION"), 119), base, theme.name))
    out = Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines) + "\n", encoding="latin-1")
    print("%s: %d entries" % (out, len(lines) - 1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
