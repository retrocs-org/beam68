#!/usr/bin/env python3
"""Create one Beam68 0.1 archive containing code and public-web together."""

from pathlib import Path
import os
import tempfile
import zipfile
import importlib.util

ROOT = Path(__file__).resolve().parent.parent
VERSION = (ROOT / "VERSION").read_text(encoding="ascii").strip()

OUTPUT = ROOT.parent / ("Beam68-" + VERSION + ".zip")
SKIP_DIRS = {".git", ".svn", "build", "dist", "__pycache__", ".pytest_cache", "work", "outputs"}
SKIP_SUFFIXES = {".o", ".pyc", ".swp", ".zip", ".lha", ".uaem"}


def package_files():
    for path in sorted(ROOT.rglob("*")):
        rel = path.relative_to(ROOT)
        if any(part in SKIP_DIRS or (part.startswith('.') and part not in {'.github', '.gitlab'}) for part in rel.parts[:-1]):
            continue
        if path.name in SKIP_DIRS:
            continue
        if path.is_symlink():
            raise RuntimeError("Refusing to package a filesystem link: " + str(rel))
        if not path.is_file():
            continue
        if path.name.startswith(".env") or path.suffix.lower() in {".pem", ".key", ".p12", ".pfx"} or path.name in {".netrc", ".npmrc", ".pypirc", ".git-credentials", "id_rsa", "id_ed25519", "credentials"}:
            raise RuntimeError("Private file outside work/: " + str(rel))
        if path.name == "release-config.json" and rel.as_posix() != "examples/release-config.json":
            raise RuntimeError("Local release config belongs in work/")
        if path.suffix.lower() in SKIP_SUFFIXES or path.name in {
            ".DS_Store", "native-plugin-test.log", "runtime-test.log",
            "music-test.log"
        }:
            continue
        yield path, (Path("Beam68") / rel).as_posix()


def main():
    spec=importlib.util.spec_from_file_location('beam_build_provenance',ROOT/'scripts/build-provenance.py')
    provenance=importlib.util.module_from_spec(spec);spec.loader.exec_module(provenance)
    provenance.verify()
    security_spec=importlib.util.spec_from_file_location('release_sensitive_check',ROOT/'scripts/check-sensitive.py')
    security=importlib.util.module_from_spec(security_spec);security_spec.loader.exec_module(security)
    security.scan(ROOT)
    fd, temp_path = tempfile.mkstemp(prefix=".beam68-", suffix=".zip", dir=OUTPUT.parent)
    os.close(fd)
    try:
        with zipfile.ZipFile(temp_path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            for source, target in package_files():
                archive.write(source, target)
        with zipfile.ZipFile(temp_path) as archive:
            bad = archive.testzip()
            if bad is not None:
                raise RuntimeError("ZIP integrity error: " + bad)
            names = archive.namelist()
            for required in ("Beam68/README.md", "Beam68/BUILD-PROVENANCE.json", "Beam68/VERSION", "Beam68/src/main.c", "Beam68/public-web/index.php"):
                if required not in names:
                    raise RuntimeError("Archive missing " + required)
            if any(name.lower().endswith(".zip") for name in names):
                raise RuntimeError("Unexpected nested ZIP")
        os.replace(temp_path, OUTPUT)
    finally:
        if os.path.exists(temp_path):
            os.unlink(temp_path)
    print("Complete project:", OUTPUT)
    print("Contains source and public-web/ in one archive; no nested downloads.")


if __name__ == "__main__":
    main()
