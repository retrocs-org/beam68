#!/usr/bin/env python3
"""Built catalogs are well-formed IFF CTLG files with every MSG_* string."""
import re
import struct
from pathlib import Path

root = Path(__file__).resolve().parents[1]
total = 0
for cd in sorted(root.glob("catalogs/*.cd")):
  ids = {int(m.group(2)): m.group(1) for m in
         re.finditer(r"^([A-Z][A-Z0-9_]*) \((\d+)//\)$", cd.read_text(), re.M)}
  assert ids and sorted(ids) == list(range(1, len(ids) + 1)), cd
  total += len(ids)
  catalogs = sorted(root.glob("catalogs/*/%s.catalog" % cd.stem))
  assert len(catalogs) == 2, cd
  for path in catalogs:
      data = path.read_bytes()
      assert data[:4] == b"FORM" and data[8:12] == b"CTLG", path
      assert struct.unpack(">I", data[4:8])[0] == len(data) - 8, path
      pos, chunks = 12, {}
      while pos < len(data):
          tag, size = data[pos:pos + 4], struct.unpack(">I", data[pos + 4:pos + 8])[0]
          chunks[tag] = data[pos + 8:pos + 8 + size]
          pos += 8 + size + (size & 1)
      assert pos == len(data) and {b"FVER", b"LANG", b"STRS"} <= set(chunks), path
      assert chunks[b"LANG"].rstrip(b"\0").decode() == path.parent.name
      assert chunks[b"FVER"].startswith(b"$VER: %s.catalog " % cd.stem.encode())
      strs, pos, seen = chunks[b"STRS"], 0, {}
      while pos < len(strs):
          ident, size = struct.unpack(">II", strs[pos:pos + 8])
          text = strs[pos + 8:pos + 8 + size]
          assert text.endswith(b"\0") and b"\0" not in text[:-1], (path, ident)
          seen[ident] = text[:-1].decode("latin-1")
          pos += 8 + size + (-size % 4)
      assert set(seen) == set(ids), (path, set(ids) - set(seen))
print("Catalogs: IFF CTLG structure, language, all %d strings: PASS" % total)
