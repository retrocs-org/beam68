#!/usr/bin/env python3
"""AmigaGuide manuals: structure, links and the nodes Beam68Prefs opens."""
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]


def check(path, required=()):
    text = path.read_text(encoding="latin-1")
    assert text.startswith("@database "), path
    assert re.search(r"^@\$VER: \S+ \d+\.\d+ \(\d\d\.\d\d\.\d{4}\)$", text, re.M), path
    opened = re.findall(r"^@node\s+(\S+)", text, re.M | re.I)
    closed = re.findall(r"^@endnode", text, re.M | re.I)
    assert opened and len(opened) == len(closed), (path, len(opened), len(closed))
    assert len(set(n.upper() for n in opened)) == len(opened), "duplicate node"
    assert opened[0].upper() == "MAIN", path
    nodes = {n.upper() for n in opened}
    for target in re.findall(r'@\{"[^"]*"\s+link\s+(\S+?)\}', text, re.I):
        assert target.upper() in nodes, (path, target)
    for name in required:
        assert name in nodes, (path, name)
    # Every @{ must be a known attribute or a link; stray @ needs \@.
    for cmd in re.findall(r"(?<!\\)@\{([^}]*)\}", text):
        assert cmd.strip('"').startswith(('"', 'b', 'ub', 'i', 'ui', 'u', 'uu')) \
            or ' link ' in cmd or cmd.startswith('"'), (path, cmd)
    return nodes


prefs = (root / "src/prefs.c").read_text()
wanted = re.search(r"static const char \*nodes\[\]=\{([^}]*)\}", prefs).group(1)
wanted = re.findall(r'"([A-Z_]+)"', wanted)
assert len(wanted) == 9
check(root / "Beam68.guide", wanted + ["WHDLOAD", "LANGUAGES", "PACKAGES"])
check(root / "sdk/Beam68PluginSDK.guide", ["PROTOCOL", "MANIFEST"])
print("AmigaGuide manuals: nodes, links, Preferences help nodes: PASS")
