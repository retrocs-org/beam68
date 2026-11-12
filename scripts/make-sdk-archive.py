#!/usr/bin/env python3
"""Pack the plugin SDK with the same contents and filename as unified releases."""
from pathlib import Path
from release_support import sdk_entries,zip_write,verify_zip,sha
ROOT=Path(__file__).resolve().parents[1]
VERSION=(ROOT/'VERSION').read_text().strip()
OUT=ROOT/'dist'/('Beam68-'+VERSION+'-plugin-sdk.zip')
OUT.parent.mkdir(exist_ok=True)
entries=sdk_entries(ROOT)
zip_write(OUT,entries)
verify_zip(OUT,{name:data for name,data,_ in entries})
OUT.with_name(OUT.name+'.sha256').write_text(sha(OUT)+'  '+OUT.name+'\n')
print(OUT)
