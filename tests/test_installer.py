#!/usr/bin/env python3
"""Static checks for the AmigaOS Installer script and its resources.

The Installer utility cannot run on the build host, so this verifies what
can be verified: balanced syntax, defined procedures, every source file the
script copies, the plugin list, the derived base configuration and icons.
"""
import importlib.util
import re
import os
SOURCE_ONLY=os.environ.get("BEAM68_SOURCE_ONLY")=="1"
import struct
import sys
from pathlib import Path

sys.dont_write_bytecode = True
root = Path(__file__).resolve().parents[1]
script = (root / "Install").read_text(encoding="latin-1")


def tokens(text):
    """Yield '(' , ')' and string literals, skipping ';' comments."""
    i = 0
    while i < len(text):
        c = text[i]
        if c == ";":
            i = text.find("\n", i)
            if i < 0:
                return
        elif c in "\"'":
            j = i + 1
            while text[j] != c:
                j += 2 if text[j] == "\\" else 1
                assert j < len(text), "unterminated string"
            yield text[i:j + 1]
            i = j
        elif c in "()":
            yield c
        i += 1


depth = 0
for tok in tokens(script):
    if tok == "(":
        depth += 1
    elif tok == ")":
        depth -= 1
        assert depth >= 0, "unbalanced ')'"
assert depth == 0, "unbalanced '(' (depth %d)" % depth

code = re.sub(r'"(\\.|[^"\\])*"', '""', script)
code = re.sub(r";[^\n]*", "", code)
defined = re.findall(r"\(procedure\s+(P_[A-Z_]+)", code)
called = set(re.findall(r"\((P_[A-Z_]+)\)", code))
assert len(defined) == len(set(defined)), "duplicate procedure"
assert called <= set(defined), called - set(defined)
# Installer only knows a procedure once its definition has been read.
for match in re.finditer(r"\((P_[A-Z_]+)\)", code):
    assert code.index("(procedure " + match.group(1)) < match.start(), match.group(1)

# Texts: every #t_ variable used is defined in English, German and Swedish.
used = set(re.findall(r"#t_[a-z_]+", re.sub(r"\(set #t_[a-z_]+ ", "", code)))
english = re.findall(r'^\(set (#t_[a-z_]+) ', script, re.M)
assert used and used <= set(english), used - set(english)
for language in ("deutsch", "svenska"):
    block = script.split('(= @language "%s")' % language)[1].split("\n)\n")[0]
    assert re.findall(r"\(set (#t_[a-z_]+) ", block) == english, language

# Files the script copies from the distribution drawer.
for rel in ("Beam68", "Beam68.info", "Beam68Ctl", "Aliases.txt", "Beam68.cfg",
            "Beam68Prefs", "Beam68Prefs.info", "scripts/Beam68Status.rexx",
            "installer/Beam68-base.cfg", "installer/Beam68Start.info",
            "catalogs/deutsch/Beam68.catalog", "catalogs/svenska/Beam68.catalog",
            "catalogs/deutsch/Beam68Prefs.catalog", "catalogs/svenska/Beam68Prefs.catalog",
            "catalogs/deutsch/Beam68Plugins.catalog", "catalogs/svenska/Beam68Plugins.catalog",
            "Beam68.guide", "Beam68.guide.info", "Snippets.txt"):
    if not (SOURCE_ONLY and rel in {"Beam68","Beam68Prefs","Beam68Ctl"}):
        assert (root / rel).is_file(), rel
    assert '"%s"' % rel in script or rel.endswith((".info", ".catalog")), rel
languages = re.search(r'\(select #lang ((?:"[^"]+"\s*)+)\)', script).group(1)
assert sorted(re.findall(r'"([^"]+)"', languages)) == sorted(
    p.parent.name for p in root.glob("catalogs/*/Beam68.catalog"))

# Plugin list: same names, same order as the choices, all present on disk.
names = re.search(r'\(select #i ((?:"[^"]+"\s*)+)\)', script).group(1)
names = re.findall(r'"([^"]+)"', names)
on_disk = sorted(p.name for p in (root / "Plugins").iterdir() if p.is_dir())
assert sorted(names) == on_disk, (names, on_disk)
assert "(set #plugin-count %d)" % len(names) in script
assert "(set #plugin-mask %d)" % ((1 << len(names)) - 1) in script
choices = [m.group(1) for m in re.finditer(r'\(choices\s+((?:"[^"]*"\s*)+)\)', script)
           if "Calculator" in m.group(1)][0]
assert len(re.findall(r'"[^"]+"', choices)) == len(names)
labels = re.findall(r'"([^"]+)"', choices)
for name, label in zip(names, labels):
    manifest = (root / "Plugins" / name / (name + ".plugin")).read_text(encoding="ascii")
    title = re.search(r"^NAME=(.+)$", manifest, re.M).group(1).strip()
    assert label.startswith(title), (name, label, title)
for name in names:
    if not SOURCE_ONLY:
        assert (root / "Plugins" / name / name).is_file(), name
    assert (root / "Plugins" / name / (name + ".plugin")).is_file(), name

# The base configuration is Beam68.cfg without PLUGIN= lines, kept in sync.
spec = importlib.util.spec_from_file_location("cfg", root / "scripts/make-installer-cfg.py")
cfg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cfg)
base = (root / "installer/Beam68-base.cfg").read_text(encoding="ascii")
assert base == cfg.base_text(), "run scripts/make-installer-cfg.py"
assert "PLUGIN=" not in "\n".join(l for l in base.splitlines() if not l.startswith("#"))


def project_icon(path):
    data = (root / path).read_bytes()
    assert struct.unpack_from(">HH", data, 0) == (0xE310, 1)
    assert data[48] == 4  # WBPROJECT
    offset = 822
    size = struct.unpack_from(">I", data, offset)[0]
    tool = data[offset + 4:offset + 3 + size].decode("ascii")
    offset += 4 + size
    types = []
    if struct.unpack_from(">I", data, 54)[0]:
        count = struct.unpack_from(">I", data, offset)[0] // 4 - 1
        offset += 4
        for _ in range(count):
            size = struct.unpack_from(">I", data, offset)[0]
            types.append(data[offset + 4:offset + 3 + size].decode("ascii"))
            offset += 4 + size
    assert offset == len(data), path
    return tool, types


tool, types = project_icon("Install.info")
assert tool == "Installer" and "SCRIPT=Install" in types and "APPNAME=Beam68" in types
tool, types = project_icon("installer/Beam68Start.info")
assert tool == "SYS:Utilities/Beam68/Beam68" and "DONOTWAIT" in types
print("GUI installer: syntax, procedures, sources, plugin list, base config, icons: PASS")
