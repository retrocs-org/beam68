#!/usr/bin/env python3
"""Place a verified test installer in a mounted builds directory without replacing drawers."""
import argparse
import os
from pathlib import Path
import shutil
import tempfile
from release_support import runtime_files, sha, verify_lha

ROOT=Path(__file__).resolve().parents[1]


def copy_files(source, target, names):
    """Preserve drawer inodes/guest directory locks and all unrelated files."""
    if target.is_symlink():
        raise ValueError('Linked staging drawer')
    target.mkdir(parents=True,exist_ok=True)
    paths=[]
    for name in names:
        relative=Path(name)
        if relative.is_absolute() or '..' in relative.parts:
            raise ValueError('Invalid staged path')
        src=source/relative;dst=target/relative
        if not src.is_file() or src.is_symlink():
            raise ValueError('Missing or linked source: '+name)
        for parent in (dst,*dst.parents):
            if parent==target.parent:break
            if parent.is_symlink():raise ValueError('Linked staging path: '+name)
        if dst.exists() and not dst.is_file():raise ValueError('Staging file conflicts with drawer: '+name)
        paths.append((src,dst))
    for src,dst in paths:
        dst.parent.mkdir(parents=True,exist_ok=True)
        fd,temp=tempfile.mkstemp(prefix='.beam68-copy-',dir=dst.parent)
        os.close(fd)
        try:
            shutil.copy2(src,temp)
            os.replace(temp,dst)
        finally:
            if os.path.exists(temp):os.unlink(temp)
        if dst.read_bytes()!=src.read_bytes():raise ValueError('Staging verification failed: '+dst.name)


def ensure_drawer_icon(source, destination):
    """Restore the outer drawer icon without overwriting user customizations."""
    icon=destination/'Beam68.info'
    if icon.is_symlink():raise ValueError('Linked drawer icon')
    if icon.exists():
        if not icon.is_file():raise ValueError('Drawer icon is not a file')
        return
    data=source.read_bytes()
    if len(data)<78 or data[:2]!=b'\xe3\x10' or data[48]!=2:
        raise ValueError('Invalid drawer icon')
    with tempfile.TemporaryDirectory() as tmp:
        staging=Path(tmp);(staging/'Beam68.info').write_bytes(data)
        copy_files(staging,destination,['Beam68.info'])



def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--destination',type=Path,required=True,help='Existing shared builds root')
    parser.add_argument('--lha',default=os.environ.get('LHA') or shutil.which('lha'))
    args=parser.parse_args()
    version=(ROOT/'VERSION').read_text().strip()
    archive='Beam68-'+version+'.lha'
    names=runtime_files(ROOT)
    if not args.destination.is_dir() or args.destination.is_symlink():
        parser.error('Destination must be an existing real builds directory')
    verify_lha(args.output/archive,{'Beam68/'+n:(ROOT/n).read_bytes() for n in names},args.lha)
    checksum=args.output/(archive+'.sha256')
    if checksum.read_text()!=sha(args.output/archive)+'  '+archive+'\n':
        parser.error('Archive checksum differs')
    # Back up before replacing any test files; leave unrelated guest metadata alone.
    backups=args.output/'release/staging-backups';backups.mkdir(parents=True,exist_ok=True)
    backup=Path(tempfile.mkdtemp(prefix='previous-',dir=backups))
    target=args.destination/'Beam68'
    if target.is_symlink():parser.error('Linked Beam68 drawer')
    for name in names:
        src=target/name
        if src.is_file() and not src.is_symlink():
            dst=backup/'Beam68'/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
    ensure_drawer_icon(ROOT/'installer/Beam68Drawer.info',args.destination)
    copy_files(ROOT,target,names)
    copy_files(args.output,args.destination,[archive,archive+'.sha256'])
    print('Verified installer: '+str(target))
    print('Existing Workbench drawer locks and unrelated files retained.')


if __name__=='__main__':main()
