#!/usr/bin/env python3
"""Check package inventory/references and quoted source includes for exact case."""
import argparse
from pathlib import Path
from path_case import check_runtime,check_headers
from release_support import runtime_files,source_files
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
parser.add_argument('--runtime-only',action='store_true',help='Check an extracted runtime drawer')
args=parser.parse_args()
if args.runtime_only:
    names=[p.relative_to(args.root).as_posix() for p in args.root.rglob('*') if p.is_file() and p.suffix!='.uaem']
    check_runtime(args.root,names)
else:
    runtime_files(args.root)
    check_headers(args.root,source_files(args.root))
print('PASS: exact-case inventory, plugin/config/Installer references'+('' if args.runtime_only else ' and source includes'))
