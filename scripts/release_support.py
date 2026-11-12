"""Local, deterministic source selection and stored LHA packaging for releases."""
import hashlib
import json
import os
import re
import struct
import subprocess
import tempfile
import zipfile
from pathlib import Path

class ReleaseError(RuntimeError):
    pass

SKIP_DIRS = {'.git', '.svn', 'build', 'dist', 'outputs', 'work', '__pycache__',
             '.pytest_cache'}
SKIP_SUFFIXES = {'.o', '.pyc', '.swp', '.zip', '.lha', '.uaem', '.log'}
GENERATED = {'BUILD-PROVENANCE.json', 'RELEASE-BUILD.json', '.DS_Store'}

def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()

def canonical(data):
    return (json.dumps(data, sort_keys=True, indent=2) + '\n').encode()

def atomic_json(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.is_symlink() or path.parent.is_symlink():
        raise ReleaseError('Refusing linked release metadata')
    temp = path.with_name(path.name + '.new')
    with temp.open('xb') as stream:
        stream.write(canonical(data))
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temp, path)

def binaries(root):
    return {'Beam68', 'Beam68Prefs', 'Beam68Ctl', 'CreateIcons', 'sdk/Example/HelloWorld/HelloWorld'} | {
        p.parent.relative_to(root).as_posix() + '/' + p.parent.name
        for p in (root / 'Plugins').glob('*/*.plugin')}

def source_files(root):
    excluded = binaries(root)
    files = {}
    for base, dirs, names in os.walk(root, followlinks=False):
        relbase = Path(base).relative_to(root)
        for name in list(dirs):
            path = Path(base) / name
            if name in SKIP_DIRS or (name.startswith('.') and name not in {'.github', '.gitlab'}):
                dirs.remove(name)
            elif path.is_symlink():
                raise ReleaseError('Linked source directory: ' + str(path.relative_to(root)))
        for name in sorted(names):
            path = Path(base) / name
            rel = (relbase / name).as_posix()
            if name in SKIP_DIRS or rel in excluded or name in GENERATED or path.suffix.lower() in SKIP_SUFFIXES:
                continue
            if name.startswith('.env') or path.suffix.lower() in {'.pem', '.key', '.p12', '.pfx'} or name in {'.netrc', '.npmrc', '.pypirc', '.git-credentials', 'id_rsa', 'id_ed25519', 'credentials', 'release-config.json'}:
                # Only the committed placeholder example belongs in the source archive.
                if rel != 'examples/release-config.json':
                    raise ReleaseError('Private/configuration file outside work/: ' + rel)
            if path.is_symlink() or not path.is_file():
                raise ReleaseError('Not an ordinary source file: ' + rel)
            files[rel] = {'sha256': sha(path), 'executable': bool(path.stat().st_mode & 0o111)}
    for required in ('VERSION', 'LICENSE', 'README.md', 'Makefile', 'src/main.c',
                     'scripts/release.py', 'public-web/index.php', 'sdk/README.md'):
        if required not in files:
            raise ReleaseError('Missing source input: ' + required)
    return dict(sorted(files.items()))

def fingerprint(files):
    return hashlib.sha256(canonical(files)).hexdigest()

def zip_write(path, entries):
    with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data, executable in sorted(entries):
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.create_system = 3
            info.external_attr = (0o100755 if executable else 0o100644) << 16
            archive.writestr(info, data)

def verify_zip(path, expected):
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        if len(names) != len(set(names)) or set(names) != set(expected) or archive.testzip():
            raise ReleaseError('Unexpected ZIP inventory: ' + path.name)
        for name, data in expected.items():
            if archive.read(name) != data:
                raise ReleaseError('Changed ZIP member: ' + name)

def sdk_entries(root):
    files=source_files(root)
    required={'README.md','include/beam68plugin.h','src/beam68plugin.c',
              'Beam68PluginSDK.guide','Example/HelloWorld/HelloWorld.c',
              'Example/Makefile','tools/package-plugin.py','Example/HelloRexx/HelloRexx.rexx'}
    selected={name[4:]:meta for name,meta in files.items() if name.startswith('sdk/')}
    if not required <= set(selected):
        raise ReleaseError('Plugin SDK is incomplete')
    top='Beam68-PluginSDK/'
    result=[(top+name,(root/'sdk'/name).read_bytes(),meta['executable']) for name,meta in selected.items()]
    result.append((top+'LICENSE',(root/'LICENSE').read_bytes(),False))
    return result

def runtime_files(root):
    text = (root / 'scripts/build-install-amiga.sh').read_text()
    result = []
    for var in ('runtime_executables', 'runtime_data'):
        match = re.search(var + r'="\n(.*?)"', text, re.S)
        if not match:
            raise ReleaseError('Missing runtime inventory: ' + var)
        result += match.group(1).splitlines()
    result += ['README.md', 'LICENSE', 'CHANGELOG.md']
    result = [name.strip() for name in result if name.strip()]
    if len(result) != len(set(result)):
        raise ReleaseError('Duplicate runtime inventory')
    for name in result:
        path = root / name
        if '..' in Path(name).parts or path.is_symlink() or not path.is_file():
            raise ReleaseError('Missing or linked runtime file: ' + name)
    return sorted(result)

def crc16(data):
    crc = 0
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xa001 if crc & 1 else 0)
    return crc

def lha_environment():
    return {key:value for key,value in os.environ.items()
            if not key.upper().endswith(('_TOKEN','_PASSWORD','_SECRET'))}

def lha_write(path, entries, tool=None):
    """LHA level 0, -lh0- stored members. No external compressor required."""
    if tool:
        with tempfile.TemporaryDirectory(dir=path.parent) as directory:
            stage=Path(directory)
            for name,data in entries.items():
                target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
                os.utime(target,(315532800,315532800))
            result=subprocess.run([tool,'ao5g0q2',str(path.resolve()),*sorted(entries)],cwd=stage,capture_output=True,env=lha_environment())
            if result.returncode:
                raise ReleaseError('LHA compression failed')
        return
    with path.open('wb') as stream:
        for name, data in sorted(entries.items()):
            encoded = name.replace('/', '\\').encode('ascii')
            if len(encoded) > 233 or len(data) > 0xffffffff:
                raise ReleaseError('LHA member exceeds level-0 limits')
            # Fixed DOS timestamp: 1980-01-01. Attribute 0x20 is a regular file.
            body = b'-lh0-' + struct.pack('<III', len(data), len(data), 0x00210000)
            body += bytes((0x20, 0, len(encoded))) + encoded + struct.pack('<H', crc16(data))
            stream.write(bytes((len(body), sum(body) & 255)) + body + data)
        stream.write(b'\0')

def verify_lha(path, expected, tool=None):
    """Check headers, CRCs, complete names and exact bytes, including EOF."""
    found = set()
    compressed = False
    with path.open('rb') as stream:
        while True:
            size = stream.read(1)
            if size == b'\0':
                if stream.read(1):
                    raise ReleaseError('Trailing LHA data')
                break
            if not size:
                raise ReleaseError('Truncated LHA')
            checksum = stream.read(1)
            body = stream.read(size[0])
            if len(checksum) != 1 or len(body) != size[0] or sum(body) & 255 != checksum[0] or len(body) < 22:
                raise ReleaseError('Invalid LHA header')
            packed, unpacked = struct.unpack_from('<II', body, 5)
            n = body[19]
            if body[:5] not in (b'-lh0-',b'-lh5-') or body[18] != 0 or len(body) != 22 + n or packed>64*1024*1024:
                raise ReleaseError('Unexpected LHA format')
            name = body[20:20+n].replace(b'\xff',b'/').decode('ascii').replace('\\', '/')
            if name in found or name not in expected or unpacked != len(expected[name]):
                raise ReleaseError('Unexpected LHA member: ' + name)
            data = stream.read(packed)
            if len(data)!=packed or crc16(expected[name]) != struct.unpack_from('<H', body, 20+n)[0]:
                raise ReleaseError('Changed LHA member: ' + name)
            if body[:5]==b'-lh0-':
                if data!=expected[name]:
                    raise ReleaseError('Changed stored LHA member')
            else:
                compressed=True
            found.add(name)
    if found != set(expected):
        raise ReleaseError('Missing LHA members')

    if compressed:
        if not tool:
            raise ReleaseError('Compressed LHA verification requires the configured LHA reader')
        # Names, lengths, duplicates and checksums were checked before extraction.
        with tempfile.TemporaryDirectory() as directory:
            result=subprocess.run([tool,'xq2',str(path.resolve())],cwd=directory,capture_output=True,env=lha_environment())
            if result.returncode:
                raise ReleaseError('LHA extraction verification failed')
            actual={}
            for p in Path(directory).rglob('*'):
                if p.is_symlink():
                    raise ReleaseError('Unexpected LHA link')
                if p.is_file():
                    actual[p.relative_to(directory).as_posix()]=p.read_bytes()
            if actual!=expected:
                raise ReleaseError('Extracted LHA differs from runtime inputs')
