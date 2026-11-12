#!/usr/bin/env python3
"""Record/verify that packaged native binaries match the current code."""
import argparse,hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
RECORD=ROOT/'BUILD-PROVENANCE.json'
BINARIES=['Beam68','Beam68Prefs','Beam68Ctl','CreateIcons']+[
    str(p.parent/ p.parent.name).replace(str(ROOT)+'/', '') for p in sorted((ROOT/'Plugins').glob('*/*.plugin'))]

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def code_hash():
    paths=[ROOT/'Makefile',ROOT/'Makefile.cross',ROOT/'VERSION',ROOT/'scripts/build-amiga.sh']
    for directory in ('src','include','Plugins','sdk/src','sdk/include'):
        paths += [p for p in (ROOT/directory).rglob('*') if p.is_file() and p.suffix in ('.c','.h','.plugin','.cfg','.rexx')]
    result=hashlib.sha256()
    for p in sorted(set(paths)):
        result.update(str(p.relative_to(ROOT)).encode()+b'\0'+digest(p).encode()+b'\n')
    return result.hexdigest()
def verify():
    if not RECORD.is_file():raise ValueError('No build provenance. Rebuild with scripts/build-amiga.sh before packaging.')
    record=json.loads(RECORD.read_text())
    if record.get('version')!=(ROOT/'VERSION').read_text().strip() or record.get('code_sha256')!=code_hash():raise ValueError('Code changed after the native build; rebuild before packaging.')
    if set(record.get('binaries',{}))!=set(BINARIES):raise ValueError('Incomplete native binary inventory; rebuild.')
    for name,expected in record['binaries'].items():
        if not (ROOT/name).is_file() or digest(ROOT/name)!=expected:raise ValueError('Native binary changed: '+name)
    return record
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action',choices=['record','verify']);parser.add_argument('--compiler');parser.add_argument('--flags',default='')
    args=parser.parse_args()
    if args.action=='verify':verify();print('Native source and binary hashes verified');return
    if not args.compiler:parser.error('record needs --compiler')
    binaries={}
    latest=max(p.stat().st_mtime for directory in ('src','include','Plugins','sdk/src','sdk/include') for p in (ROOT/directory).rglob('*') if p.is_file() and p.suffix in ('.c','.h'))
    for name in BINARIES:
        path=ROOT/name
        if not path.is_file() or path.read_bytes()[:4]!=b'\0\0\3\xf3':raise ValueError('Missing native HUNK binary: '+name)
        if path.stat().st_mtime<latest:raise ValueError("Native binary predates current code: "+name+"; perform a clean all-target build.")
        binaries[name]=digest(path)
    result=subprocess.run([args.compiler,'--version'],capture_output=True,text=True)
    version=(result.stdout+result.stderr).strip().splitlines()
    record={'version':(ROOT/'VERSION').read_text().strip(),'target':'m68000','compiler':Path(args.compiler).name,'compiler_version':version[0] if version else 'unreported','flags':args.flags,'code_sha256':code_hash(),'binaries':binaries}
    temp=RECORD.with_suffix('.json.new');temp.write_text(json.dumps(record,indent=2)+'\n');temp.replace(RECORD)
    verify();print('Recorded '+str(len(binaries))+' native binaries')
if __name__=='__main__':
    try:main()
    except (OSError,ValueError) as error:raise SystemExit(str(error))
