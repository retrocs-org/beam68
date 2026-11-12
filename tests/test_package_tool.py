#!/usr/bin/env python3
"""SDK packaging tool: validation rules and archive layout."""
import importlib.util
import sys
import os
import shutil
import tempfile
import zipfile
from pathlib import Path

sys.dont_write_bytecode = True  # keep sdk/tools free of __pycache__
root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("pkg", root / "sdk/tools/package-plugin.py")
pkg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pkg)

work = Path(tempfile.mkdtemp())
try:
    drawer = work / "HelloWorld"
    shutil.copytree(root / "sdk/Example/HelloWorld", drawer)
    (drawer / "HelloWorld").write_bytes(b"\x00\x00\x03\xf3" + b"\x00" * 60)
    keys, errors = pkg.validate(str(drawer))
    assert not errors, errors
    assert pkg.main([str(drawer), "-o", str(work), "--format", "zip"]) == 0
    archive = work / "Beam68-Plugin-HelloWorld-1.0.zip"
    names = zipfile.ZipFile(archive).namelist()
    assert all(n.startswith("HelloWorld/") for n in names), names
    assert "HelloWorld/HelloWorld.plugin" in names and "HelloWorld/HelloWorld" in names
    assert not any(".." in n or n.startswith("/") or ":" in n for n in names)

    manifest = drawer / "HelloWorld.plugin"
    good = manifest.read_text()

    def broken(old, new):
        manifest.write_text(good.replace(old, new))
        _, errs = pkg.validate(str(drawer))
        manifest.write_text(good)
        return errs

    assert broken("COMMAND=PLUGINDIR:HelloWorld", "COMMAND=PROGDIR:Plugins/HelloWorld")
    assert broken("COMMAND=PLUGINDIR:HelloWorld", "COMMAND=PLUGINDIR:/HelloWorld")
    assert broken("ID=org.example.helloworld", "ID=bad id")
    assert broken("PLUGIN_VERSION=1.0", "PLUGIN_VERSION=one")
    assert broken("MIN_BEAM68=0.1", "MIN_BEAM68=9.0")
    assert broken("TRIGGER=+", "TRIGGER=ab")
    assert not broken("MIN_BEAM68=0.1", "MIN_BEAM68=0.1.0")
    (drawer / "HelloWorld").write_bytes(b"#!/bin/sh\n")
    assert any("HUNK" in e for e in pkg.validate(str(drawer))[1])
    # ARexx plugins: a .rexx script starting with a comment, protocol 1 only.
    rexx = work / "HelloRexx"
    shutil.copytree(root / "sdk/Example/HelloRexx", rexx)
    assert not pkg.validate(str(rexx))[1], pkg.validate(str(rexx))[1]
    m = rexx / "HelloRexx.plugin"
    m.write_text(m.read_text().replace("VERSION=1", "VERSION=2", 1))
    assert any("VERSION=1" in e for e in pkg.validate(str(rexx))[1])
    m.write_text(m.read_text().replace("VERSION=2", "VERSION=1", 1))
    (rexx / "HelloRexx.rexx").write_text("say hi\n")
    assert any("ARexx" in e for e in pkg.validate(str(rexx))[1])

    # Every bundled plugin is itself a valid package.
    for bundled in sorted((root / "Plugins").iterdir()):
        if bundled.is_dir():
            candidate = bundled
            if os.environ.get("BEAM68_SOURCE_ONLY")=="1" and not (bundled / bundled.name).exists():
                # Source-only CI validates manifests/package layout with an
                # explicit test fixture, not a claimed native executable.
                candidate = work / ('bundled-' + bundled.name)
                shutil.copytree(bundled, candidate)
                (candidate / bundled.name).write_bytes(b"\x00\x00\x03\xf3" + b"\x00" * 60)
            _, errs = pkg.validate(str(candidate))
            assert not errs, (bundled.name, errs)
    secret = work / "outside.txt"
    secret.write_text("outside the plugin")
    os.symlink(secret, drawer / "linked.txt")
    try:
        list(pkg.files_of(str(drawer)))
        raise AssertionError("Linked files must not be packaged")
    except ValueError:
        pass
    print("Plugin SDK packaging tool: validation and archive layout: PASS")
finally:
    shutil.rmtree(work)
