#!/usr/bin/env python3
"""Build an Aminet-style release: dist/Beam68.lha (or .zip) + dist/Beam68.readme.

    make-aminet-release.py [--author "Name <mail>"] [--uploader "..."]

The archive holds one "Beam68" drawer with the runtime files (the same list
scripts/build-install-amiga.sh stages) plus Install/Install.info, so users
unpack it anywhere and double-click Install.  Aminet requires LHA: the
archive is .lha when an `lha` command is available, otherwise .zip with a
warning.  The readme follows the Aminet header format; its text comes from
VERSION, CHANGELOG.md and README.md.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import textwrap
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VERSION = (ROOT / "VERSION").read_text(encoding="ascii").strip()
DIST = ROOT / "dist"


def runtime_files():
    script = (ROOT / "scripts/build-install-amiga.sh").read_text()
    files = []
    for var in ("runtime_executables", "runtime_data"):
        block = re.search(var + r'="\n(.*?)"', script, re.S).group(1)
        files += [line.strip() for line in block.splitlines() if line.strip()]
    for extra in ("README.md", "LICENSE", "CHANGELOG.md"):
        if extra not in files:
            files.append(extra)
    return files


def readme(author, uploader, archive_name):
    changelog = (ROOT / "CHANGELOG.md").read_text(encoding="utf-8")
    section = re.search(r"^## " + re.escape(VERSION) + r"(?:[^\n]*)\n(.*?)(?=^## |\Z)",
                        changelog, re.M | re.S)
    if not section:
        raise ValueError("Missing changelog section for " + VERSION)
    changes = [VERSION]
    paragraph = []
    def flush():
        if paragraph:
            changes.append(textwrap.fill(" ".join(paragraph), 76,
                           initial_indent="        - ", subsequent_indent="          "))
            paragraph.clear()
    for line in section.group(1).splitlines():
        if line.startswith("### "):
            flush()
            changes.extend(["", "    " + line[4:].strip()])
        elif line.startswith("- "):
            flush()
            paragraph.append(line[2:].strip().replace("`", ""))
        elif line.strip():
            paragraph.append(line.strip().replace("`", ""))
        else:
            flush()
    flush()
    header = [
        "Short:        Keyboard application launcher for OS 3.x",
        "Author:       %s" % author,
        "Uploader:     %s" % uploader,
        "Type:         util/wb",
        "Version:      %s" % VERSION,
        "Architecture: m68k-amigaos >= 3.0.0",
        "Requires:     AmigaOS 3.0+, 68000+; TCP/IP for Aminet Browser",
        "Distribution: Aminet",
        "URL:          https://beam68.retrocs.org/",
        "",
    ]
    body = """Beam68 is a keyboard-driven application launcher for classic AmigaOS,
with a global hotkey and a separate native Preferences application.

FEATURES

- Find applications, files, drawers and WHDLoad games from the keyboard.
- Use favorites, aliases, recent items and searchable command help.
- Use 18 bundled plugins, including opt-in Aminet and Wikipedia searches.
- Start with six local tools; network and optional plugins are off.
- Adjust indexed drawers, shortcuts, fonts, colors and portable themes.

INSTALLATION

Unpack %s, open the Beam68 drawer and double-click Install.
Choose Novice for automatic setup, or Intermediate/Expert to customize it.
Updates preserve saved settings. The launcher starts in the background; the
installer shows the configured shortcut (Ctrl + Alt + Space by default).
Uninstall can keep or remove saved and temporary preferences.
Read Beam68.guide for the full manual.

SOURCE AND LICENCE

Source:  https://github.com/retrocs-org/beam68
Release: https://github.com/retrocs-org/beam68/releases/tag/%s
Licence: MIT. See LICENSE in the archive for the full licence text.

VERSION CHANGELOG

%s
""" % (archive_name, VERSION, "\n".join(changes))
    text = "\n".join(header) + "\n" + body
    # Plain ASCII is safe in classic viewers; never emit replacement characters.
    for old, new in (("\u2014", "-"), ("\u2013", "-"), ("\u2019", "'"), ("\u201c", '"'), ("\u201d", '"')):
        text = text.replace(old, new)
    return text.encode("ascii", "strict")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--author", default=os.environ.get("BEAM68_AUTHOR", "Beam68 contributors"))
    parser.add_argument("--uploader", default=os.environ.get("BEAM68_UPLOADER", ""))
    args = parser.parse_args(argv)
    if not re.fullmatch(r'[^\s@<>]+@[^\s@<>]+\.[^\s@<>]+', args.uploader) or any(c in args.author for c in ('\r','\n')):
        print("error: supply a valid --uploader contact (or BEAM68_UPLOADER) and a single-line author", file=sys.stderr)
        return 1
    missing = [f for f in runtime_files() if not (ROOT / f).is_file()]
    if missing:
        print("error: missing (build first?): " + ", ".join(missing), file=sys.stderr)
        return 1
    DIST.mkdir(exist_ok=True)
    tool = shutil.which("lha")
    stage = Path(tempfile.mkdtemp())
    try:
        for rel in runtime_files():
            target = stage / "Beam68" / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / rel, target)
        if tool:
            archive = DIST / "Beam68.lha"
            if archive.exists():
                archive.unlink()
            subprocess.run([tool, "a", str(archive)] + [str(p.relative_to(stage)) for p in
                           sorted((stage / "Beam68").rglob("*")) if p.is_file()],
                           cwd=stage, check=True, stdout=subprocess.DEVNULL)
        else:
            archive = DIST / "Beam68.zip"
            print("warning: no `lha` tool; wrote ZIP (Aminet requires LHA)", file=sys.stderr)
            with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as out:
                for path in sorted((stage / "Beam68").rglob("*")):
                    out.write(path, path.relative_to(stage).as_posix())
    finally:
        shutil.rmtree(stage)
    (DIST / "Beam68.readme").write_bytes(readme(args.author, args.uploader, archive.name))
    print(archive)
    print(DIST / "Beam68.readme")
    return 0


if __name__ == "__main__":
    sys.exit(main())
