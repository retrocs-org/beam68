#!/usr/bin/env python3
"""Validate a Beam68 plugin drawer and pack it for distribution.

    package-plugin.py <PluginDrawer> [-o OUTDIR] [--format auto|lha|zip]

Produces Beam68-Plugin-<Drawer>-<PLUGIN_VERSION>.lha (when an `lha` tool
is installed and --format is auto or lha) or .zip.  The archive contains
exactly one top-level drawer, which is what Beam68Prefs installs.

The checks mirror Beam68's own rules (src/config.c, src/plugin_package.c),
so a package that passes here is accepted by Install in Beam68Prefs.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import zipfile

ID_RE = re.compile(r"^[A-Za-z0-9_][A-Za-z0-9_.-]{0,62}$")
DRAWER_RE = re.compile(r"^[A-Za-z0-9_][A-Za-z0-9_.-]{0,29}$")
VERSION_RE = re.compile(r"^[0-9]+(\.[0-9]+)*$")
CAP_RE = re.compile(r"^[A-Z_]{1,19}$")
SKIP = {".DS_Store", "Thumbs.db"}
BEAM68_VERSION = "0.1"


def version_tuple(text):
    parts = [int(p) for p in text.split(".")]
    return tuple(parts + [0] * (8 - len(parts)))


def plugindir_ok(value):
    if not value.upper().startswith("PLUGINDIR:"):
        return False
    rel = value[10:]
    return bool(rel) and ":" not in rel and not rel.startswith("/") and "//" not in rel \
        and not any(c in rel for c in '"*\r\n')


def parse_manifest(path):
    keys, settings, errors = {}, 0, []
    with open(path, "rb") as handle:
        raw = handle.read()
    try:
        text = raw.decode("ascii")
    except UnicodeDecodeError:
        errors.append("manifest must be plain ASCII")
        text = raw.decode("latin-1")
    for number, line in enumerate(text.splitlines(), 1):
        line = line.strip()
        if not line or line[0] in "#;":
            continue
        if "=" not in line:
            continue
        key, value = (part.strip() for part in line.split("=", 1))
        key = key.upper()
        if key == "SETTING":
            settings += 1
            continue
        if key in keys:
            errors.append("line %d: duplicate %s" % (number, key))
        keys[key] = value
    return keys, settings, errors


def validate(drawer):
    errors = []
    name = os.path.basename(os.path.normpath(drawer))
    if not DRAWER_RE.match(name):
        errors.append("drawer name %r: use letters, digits, . _ - (max 30)" % name)
    manifests = [f for f in os.listdir(drawer) if f.lower().endswith(".plugin")]
    if len(manifests) != 1:
        return None, ["drawer must contain exactly one .plugin manifest"]
    keys, settings, errs = parse_manifest(os.path.join(drawer, manifests[0]))
    errors += errs

    def need(key, check, message):
        if key not in keys or not check(keys[key]):
            errors.append(message)

    need("VERSION", lambda v: v == "1", "VERSION=1 is required (public protocol)")
    need("ID", ID_RE.match, "ID= must be letters, digits, . _ - (e.g. org.you.plugin)")
    need("PLUGIN_VERSION", lambda v: len(v)<=15 and VERSION_RE.match(v), "PLUGIN_VERSION= must be dotted digits, e.g. 1.0")
    need("NAME", lambda v: 0 < len(v) < 64, "NAME= is required (max 63 characters)")
    need("TRIGGER", lambda v: len(v) == 1 and "!" <= v <= "~", "TRIGGER= must be one printable character")
    need("COMMAND", plugindir_ok, "COMMAND= must be PLUGINDIR:<file> inside the drawer")
    if "MIN_BEAM68" in keys:
        if len(keys["MIN_BEAM68"])>15 or not VERSION_RE.match(keys["MIN_BEAM68"]):
            errors.append("MIN_BEAM68= must be dotted digits")
        elif version_tuple(keys["MIN_BEAM68"]) > version_tuple(BEAM68_VERSION):
            errors.append("MIN_BEAM68=%s is newer than this SDK (%s)" % (keys["MIN_BEAM68"], BEAM68_VERSION))
    if keys.get("ICON") and keys["ICON"].upper() != "PROGDIR:BEAM68" and not plugindir_ok(keys["ICON"]):
        errors.append("ICON= must be PLUGINDIR:<file>, PROGDIR:Beam68 or omitted")
    if keys.get("GUIDE") and not plugindir_ok(keys["GUIDE"]):
        errors.append("GUIDE= must be PLUGINDIR:<file>")
    elif keys.get("GUIDE") and not os.path.isfile(os.path.join(drawer, *keys["GUIDE"][10:].split("/"))):
        errors.append("GUIDE file %s is missing" % keys["GUIDE"][10:])
    if keys.get("SETTINGS"):
        rest = keys["SETTINGS"][len("ENVARC:Beam68/Plugins/"):]
        if not keys["SETTINGS"].upper().startswith("ENVARC:BEAM68/PLUGINS/") or not rest \
                or "/" in rest or ":" in rest or rest.startswith("."):
            errors.append("SETTINGS= must be ENVARC:Beam68/Plugins/<file>")
    elif settings:
        errors.append("SETTING= entries need a SETTINGS= file")
    if keys.get("PREFIX") and (" " in keys["PREFIX"] or len(keys["PREFIX"]) >= 24):
        errors.append("PREFIX= must be one word, max 23 characters")
    for cap in filter(None, keys.get("CAPABILITIES", "").split(",")):
        if not CAP_RE.match(cap):
            errors.append("CAPABILITIES entry %r is invalid" % cap)
    if "COMMAND" in keys and plugindir_ok(keys["COMMAND"]):
        exe = os.path.join(drawer, *keys["COMMAND"][10:].split("/"))
        if not os.path.isfile(exe):
            errors.append("executable %s is missing (build it first)" % keys["COMMAND"][10:])
        else:
            with open(exe, "rb") as handle:
                head = handle.read(4)
            if exe.lower().endswith(".rexx"):
                if not head.startswith(b"/*"):
                    errors.append("%s must start with an ARexx /* comment */" % keys["COMMAND"][10:])
            elif head != b"\x00\x00\x03\xf3":
                errors.append("%s is not an AmigaOS (HUNK) executable" % keys["COMMAND"][10:])
    return keys, errors


def files_of(drawer):
    for root, dirs, files in os.walk(drawer):
        for name in dirs + files:
            path = os.path.join(root, name)
            if os.path.islink(path):
                raise ValueError("Plugin packages cannot contain links: " + path)
        dirs[:] = sorted(d for d in dirs if not d.startswith("."))
        for name in sorted(files):
            if name in SKIP or name.startswith(".") or name.endswith(".o"):
                continue
            path=os.path.join(root, name)
            rel=os.path.relpath(path, os.path.dirname(os.path.abspath(drawer))).replace(os.sep, "/")
            try:
                encoded=rel.encode("latin-1")
            except UnicodeEncodeError:
                raise ValueError("Plugin filenames must be representable on classic AmigaOS: " + rel)
            if len(encoded)>=240 or any(ord(c)<32 or ord(c)==127 or c in '\"*`:\\' for c in rel):
                raise ValueError("Unsafe or oversized package filename: " + rel)
            yield path


def write_zip(drawer, target):
    base = os.path.dirname(os.path.abspath(drawer))
    top = os.path.basename(os.path.normpath(drawer))
    with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.writestr(top + "/", b"")
        for path in files_of(drawer):
            archive.write(path, os.path.relpath(os.path.abspath(path), base).replace(os.sep, "/"))


def write_lha(drawer, target, tool):
    base = os.path.dirname(os.path.abspath(drawer))
    files = [os.path.relpath(os.path.abspath(p), base) for p in files_of(drawer)]
    if os.path.exists(target):
        os.remove(target)
    subprocess.run([tool, "a", os.path.abspath(target)] + files, cwd=base, check=True,
                   stdout=subprocess.DEVNULL)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("drawer")
    parser.add_argument("-o", "--outdir", default=".")
    parser.add_argument("--format", choices=("auto", "lha", "zip"), default="auto")
    parser.add_argument("--check", action="store_true", help="validate only")
    args = parser.parse_args(argv)
    if not os.path.isdir(args.drawer):
        print("error: %s is not a drawer" % args.drawer, file=sys.stderr)
        return 2
    keys, errors = validate(args.drawer)
    if errors:
        for message in errors:
            print("error: " + message, file=sys.stderr)
        return 1
    if args.check:
        print("OK: %s %s" % (keys["NAME"], keys["PLUGIN_VERSION"]))
        return 0
    tool = shutil.which("lha")
    fmt = args.format
    if fmt == "auto":
        fmt = "lha" if tool else "zip"
    if fmt == "lha" and not tool:
        print("error: no `lha` tool found; use --format zip", file=sys.stderr)
        return 2
    top = os.path.basename(os.path.normpath(args.drawer))
    target = os.path.join(args.outdir, "Beam68-Plugin-%s-%s.%s" % (top, keys["PLUGIN_VERSION"], fmt))
    os.makedirs(args.outdir, exist_ok=True)
    if fmt == "zip":
        write_zip(args.drawer, target)
    else:
        write_lha(args.drawer, target, tool)
    print(target)
    return 0


if __name__ == "__main__":
    sys.exit(main())
