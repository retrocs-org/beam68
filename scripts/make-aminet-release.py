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
    intro = (ROOT / "README.md").read_text(encoding="utf-8").split("\n## ")[0]
    intro = re.sub(r"^# .*\n", "", intro).strip()
    intro = re.sub(r"[*`]", "", intro)
    intro = "\n\n".join(textwrap.fill(" ".join(p.split()), 76) for p in intro.split("\n\n"))
    changes = "\n".join(l for l in changelog.splitlines()[2:60] if not l.startswith("## Unreleased —"))
    header = [
        "Short:        Keyboard application launcher for OS 3.x",
        "Author:       %s" % author,
        "Uploader:     %s" % uploader,
        "Type:         util/wb",
        "Version:      %s" % VERSION,
        "Replaces:     util/wb/Beam68.lha",
        "Architecture: m68k-amigaos >= 3.0.0",
        "Distribution: Aminet",
        "Kurz:         Programmstarter per Tastatur fuer OS 3.x",
        "",
    ]
    body = [intro, "", "Installation: unpack %s anywhere, open the Beam68 drawer" % archive_name,
            "and double-click Install (AmigaOS Installer).  The manual is",
            "Beam68.guide.", "", "Changes:", "", re.sub(r"[*`]", "", changes).strip(), ""]
    text = "\n".join(header + body)
    return text.encode("latin-1", "replace")


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
