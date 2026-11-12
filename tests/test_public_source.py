"""Public-source hygiene: no local-machine paths or excluded tooling references."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
patterns=[r'fs[ ._-]?'+'uae', 'win'+'uae', 'emu'+'lat', 'chat'+'gpt',
          'co'+'dex','open'+'a'+'i','artificial '+'intelligence',r'\b'+'A'+'I'+r'\b',
          '/Us'+'ers/', '/private/'+'tmp/']
for path in root.rglob('*'):
    rel=path.relative_to(root)
    if not path.is_file() or any(p in {'build','.git','dist','outputs','work','__pycache__'} for p in rel.parts):continue
    if path.name=='.DS_Store' or path.suffix.lower() in {'.png','.info','.catalog','.zip','.lha','.pyc'}:continue
    try:text=path.read_text()
    except UnicodeError:continue
    for pattern in patterns:
        assert not re.search(pattern,text,re.I),(str(rel),pattern)
    for pattern in patterns:
        assert not re.search(pattern,str(rel),re.I),(str(rel),pattern)
print('PASS: public source paths and documentation hygiene')
