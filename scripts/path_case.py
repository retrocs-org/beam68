"""Exact-case package paths, independent of the host filesystem's case rules."""
from pathlib import Path, PurePosixPath
import re
import os

class PathCaseError(ValueError):
    pass

def exact_file(root, name):
    parts=PurePosixPath(name).parts
    if not parts or name!=PurePosixPath(name).as_posix() or name.startswith('/') or any(p in {'.','..'} for p in parts) or "\\" in name:
        raise PathCaseError('Unsafe package path: '+name)
    current=Path(root)
    for part in parts:
        if not current.is_dir():
            raise PathCaseError('Missing path: '+name)
        names={p.name for p in current.iterdir()}
        if part not in names:
            similar=sorted(p for p in names if p.casefold()==part.casefold())
            raise PathCaseError(('Case mismatch: ' if similar else 'Missing path: ')+name+
                                (' (actual: '+', '.join(similar)+')' if similar else ''))
        current=current/part
        if current.is_symlink():
            raise PathCaseError('Linked package path: '+name)
    if not current.is_file():
        raise PathCaseError('Not a package file: '+name)
    return current

def unique_paths(names):
    seen={}
    for name in names:
        parts=PurePosixPath(name).parts
        for i in range(1,len(parts)+1):
            path='/'.join(parts[:i]);key=path.casefold()
            if key in seen and seen[key]!=path:
                raise PathCaseError('Case-colliding package paths: '+seen[key]+' / '+path)
            seen[key]=path

def check_runtime(root,names):
    root=Path(root);names=set(names);unique_paths(names)
    for name in sorted(names):exact_file(root,name)
    def reference(value,parent=''):
        if value.upper().startswith('PLUGINDIR:'):name=(PurePosixPath(parent)/value[10:]).as_posix()
        elif value.upper().startswith('PROGDIR:'):name=value[8:]
        else:return
        exact_file(root,name)
        if name not in names:raise PathCaseError('Referenced file absent from package: '+name)
    for name in sorted(names):
        path=root/name
        if name.endswith('.plugin'):
            for line in path.read_text(encoding="latin-1").splitlines():
                key,sep,value=line.partition('=')
                if sep and key in {'COMMAND','GUIDE','ICON'}:reference(value,str(PurePosixPath(name).parent))
        if name in {'Beam68.cfg','installer/Beam68-base.cfg'}:
            for line in path.read_text(encoding="latin-1").splitlines():
                key,sep,value=line.partition('=')
                if sep and key in {'PLUGIN','PLUGIN_DISABLED'}:reference(value)
    if 'Install' in names:
        text=(root/'Install').read_text(encoding='latin-1')
        for name in re.findall(r'\(set #src "([^"\n]+)"\)',text):
            if ':' not in name:
                exact_file(root,name)
                if name not in names:raise PathCaseError('Installer source absent from package: '+name)


def check_headers(root,names):
    root=Path(root)
    for name in names:
        if not name.endswith(('.c','.h')):continue
        path=exact_file(root,name)
        for header in re.findall(r'^\s*#include\s*"([^"\n]+)"',path.read_text(encoding="latin-1"),re.M):
            # Relative ../ includes are source references, not archive members.
            choices=[path.parent/header,root/'include'/header,root/'sdk/include'/header]
            found=False
            for choice in choices:
                try:relative=Path(os.path.abspath(choice)).relative_to(Path(os.path.abspath(root))).as_posix()
                except ValueError:continue
                try:exact_file(root,relative);found=True;break
                except PathCaseError:pass
            if not found:raise PathCaseError('Missing or incorrectly capitalized header: '+name+' -> '+header)
