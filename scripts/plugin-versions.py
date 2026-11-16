#!/usr/bin/env python3
"""Check independent plugin versions; --record accepts reviewed version bumps."""
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / 'Plugins/versions.json'


def version(value):
    if not re.fullmatch(r'[0-9]+(?:\.[0-9]+)*', value):
        raise ValueError('Invalid plugin version: ' + value)
    return tuple(int(x) for x in value.split('.'))


def version_greater(new, old):
    a, b = version(new), version(old)
    width = max(len(a), len(b))
    return a + (0,) * (width-len(a)) > b + (0,) * (width-len(b))


def inventory(root):
    make = (root/'Makefile').read_text()
    variables = dict(re.findall(r'^([A-Z_]+)\s*=\s*(.*)$', make, re.M))
    result = {}
    for manifest in sorted((root/'Plugins').glob('*/*.plugin')):
        name = manifest.parent.name
        fields = dict(re.findall(r'^([A-Z_]+)=(.*)$', manifest.read_text(), re.M))
        number = fields.get('PLUGIN_VERSION', '')
        version(number)
        command = re.search(r'^\t.*-o \$@ Plugins/'+re.escape(name)+r'/'+re.escape(name)+r'\.c.*$', make, re.M)
        if not command:
            raise ValueError('Missing plugin build command: ' + name)
        command = command.group()
        for key, value in variables.items():
            command = command.replace('$('+key+')', value)
        pending = [root/x for x in re.findall(r'(?:Plugins|src|sdk)/[^\s]+\.c', command)]
        pending += [p for p in manifest.parent.rglob('*') if p.is_file() and
                    p.name not in (name, 'version.c', '.DS_Store') and
                    p.suffix not in ('.uaem', '.o', '.pyc', '.lha', '.zip')]
        files = set()
        while pending:
            path = pending.pop()
            if path in files or path.name == 'version.c':
                continue
            if not path.is_file() or path.is_symlink():
                raise ValueError('Missing or linked plugin input: ' + str(path.relative_to(root)))
            files.add(path)
            if path.suffix in ('.c', '.h'):
                for header in re.findall(r'^\s*#include\s*"([^"]+)"', path.read_text(), re.M):
                    choices = [path.parent/header, root/'include'/header, root/'sdk/include'/header]
                    found = next((p for p in choices if p.is_file()), None)
                    if found is None:
                        raise ValueError('Missing local header: ' + header)
                    pending.append(found)
        digest = hashlib.sha256()
        for path in sorted(files):
            data = path.read_bytes()
            # App-only version changes do not increment independent plugins.
            if path == root/'include/beam.h':
                data = re.sub(rb'^#define BEAM68_VERSION(?:_DATE)? .*\n', b'', data, flags=re.M)
            digest.update(path.relative_to(root).as_posix().encode()+b'\0'+data+b'\0')
        result[name] = {'version': number, 'sha256': digest.hexdigest()}
    return result


def generated(name, number):
    return ('/* Generated from PLUGIN_VERSION by scripts/plugin-versions.py. */\n'
            'const char beam_plugin_version_tag[] = "$VER: '+name+' '+number+'";\n')


def check(root, record, accept=False):
    current = inventory(root)
    old = json.loads(record.read_text()) if record.exists() else None
    if old is None and not accept:
        raise ValueError('Missing plugin version record')
    if old:
        for name, item in current.items():
            previous = old.get(name)
            if previous and item != previous:
                if not version_greater(item['version'], previous['version']):
                    raise ValueError(name+': plugin inputs changed; increase PLUGIN_VERSION above '+previous['version'])
                if not accept:
                    raise ValueError(name+': review changes, then run plugin-versions.py --record')
        if set(old) != set(current) and not accept:
            raise ValueError('Plugin inventory changed; review and record it')
    for name, item in current.items():
        target = root/'Plugins'/name/'version.c'
        expected = generated(name, item['version'])
        if target.is_symlink():
            raise ValueError('Linked plugin version source: '+name)
        if accept:
            target.write_text(expected)
        elif not target.is_file() or target.read_text() != expected:
            raise ValueError(name+': generated native version differs; run --record')
    if accept:
        record.write_text(json.dumps(current, indent=2, sort_keys=True)+'\n')
    return current


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    if args.record and args.check:
        parser.error('choose --record or --check')
    try:
        items = check(ROOT, RECORD, args.record)
    except (ValueError, OSError) as exc:
        parser.exit(1, 'Plugin versions refused: '+str(exc)+'\n')
    print('Plugin versions verified: '+str(len(items))+' independent plugins')


if __name__ == '__main__':
    main()
