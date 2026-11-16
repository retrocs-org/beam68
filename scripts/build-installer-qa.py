#!/usr/bin/env python3
"""Build a separate native installer review package; never install or publish."""
import argparse
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
from release_support import runtime_files

ROOT = Path(__file__).resolve().parents[1]
QA_ROOT = "RAM:Beam68InstallerQA"


def expression_end(text, start):
    depth, quoted, escaped, comment = 0, False, False, False
    for index in range(start, len(text)):
        char = text[index]
        if comment:
            if char == "\n": comment = False
        elif quoted:
            if escaped: escaped = False
            elif char == "\\": escaped = True
            elif char == '"': quoted = False
        elif char == ";": comment = True
        elif char == '"': quoted = True
        elif char == "(": depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0: return index + 1
    raise ValueError("Unbalanced installer expression")


def replace_expression(text, marker, replacement):
    start = text.index(marker)
    return text[:start] + replacement + text[expression_end(text, start):]


def review_script(source, uninstall=False):
    text = replace_expression(source, "(procedure P_STOP_LAUNCHER", "(procedure P_STOP_LAUNCHER (set #qa-no-stop 1))")
    text = replace_expression(text, "(procedure P_START_LAUNCHER", "(procedure P_START_LAUNCHER (set #qa-no-start 1))")
    # Test copies must neither stop the resident nor start test binaries with
    # the real active ENV: configuration. The production installer is unchanged.
    start = text.index("  (if (if #novice 1")
    text = text[:start] + '(message "QA files are ready. Return to Beam68 Installer QA to check them.")' + text[expression_end(text, start + 2):]
    start = text.index("  ; Read the same active configuration")
    end = text.index("\n  (exit #t_ready)",start)
    text = text[:start] + text[end:]
    text = text.replace('(set #existing (getenv "Beam68/InstallDir"))', '(set #existing "")')
    # Fixed destinations at every user level: no path chosen by a requester
    # can escape the dedicated RAM test tree.
    while "(askdir" in text:
        start = text.index("(askdir")
        finish = expression_end(text, start)
        form = text[start:finish]
        default = "#parent" if "(default #parent)" in form else "#dest"
        text = text[:start] + default + text[finish:]
    for old, new in (("T:Beam68/", QA_ROOT + "/Session/"),
                     ("ENVARC:", QA_ROOT + "/Saved/"),
                     ("ENV:", QA_ROOT + "/Environment/"),
                     ("LOCALE:", QA_ROOT + "/Locale/"),
                     ("SYS:", QA_ROOT + "/System/")):
        text = text.replace(old, new)
    text = text.replace('(exit #t_ready)',
        '(textfile (dest "' + QA_ROOT + '/completed-install") (append "done"))\n  (exit #t_ready)')
    text = text.replace('(exit #t_removed)',
        '(textfile (dest "' + QA_ROOT + '/completed-uninstall") (append "done"))\n  (exit #t_removed)')
    if uninstall:
        start = text.rindex('(if (<> #existing "")')
        text = text[:start] + "(P_UNINSTALL)\n"
    assert 'getenv "Beam68/InstallDir"' not in text
    assert "QUITWAIT" not in text[text.index("(procedure P_STOP_LAUNCHER"):text.index("(procedure P_MAKEDIR")]
    assert '(run (cat "Run >NIL:' not in text
    return text


def build(destination, cc):
    # Validate the inventory before creating a review package.
    names = runtime_files(ROOT)
    destination.mkdir(parents=True, exist_ok=True)
    package = destination / "Package"
    package.mkdir(exist_ok=True)
    for name in names:
        target = package / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / name, target)
    source = (ROOT / "Install").read_text(encoding="latin-1")
    (package / "Install").write_text(review_script(source), encoding="latin-1")
    (package / "Uninstall-QA").write_text(review_script(source, True), encoding="latin-1")
    executable = destination / "Beam68 Installer QA"
    subprocess.run([cc, "-O2", "-m68000", "-mcrt=nix20", "-Wall", "-Wextra", "-Werror", "-Wno-pointer-sign", "-I" + str(ROOT / "include"), "-o", str(executable), str(ROOT / "tests/native/installer_qa.c"), str(ROOT / "src/stack_guard.c")], check=True)
    spec = importlib.util.spec_from_file_location("icons", ROOT / "scripts/generate_info_icons.py")
    icons = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(icons)
    executable.with_name(executable.name + ".info").write_bytes(icons.disk_object(icons.installer_art()))
    drawer_icon = bytearray(icons.disk_object(icons.installer_art()))
    drawer_icon[48] = 2  # WBDRAWER, default drawer view
    destination.with_name(destination.name + ".info").write_bytes(drawer_icon)
    (destination / "ReadMe.txt.info").write_bytes(icons.project_object(icons.guide_art(), "MultiView", []))
    (destination / "ReadMe.txt").write_text("""Beam68 Installer QA 0.2

Double-click Beam68 Installer QA. Choose Install / update, then a user level.
Novice installs automatically. Intermediate and Expert expose the normal choices;
QA destination selection is fixed inside RAM:Beam68InstallerQA for every level.

Choose Check to inspect the installed copy. Choose Install / update again to
check that settings, aliases, snippets, themes and an extra plugin survive.
Choose Uninstall to test removal. Every level asks whether to keep or remove settings; choose Remove to test
temporary configuration cleanup as well.
The checker seeds temporary settings before update and uninstall, then checks
preservation or removal as appropriate. Choose Check afterward to verify program removal. The QA menu's Report button
shows the latest results. The full log is RAM:Beam68InstallerQA/report.txt.

All test installation files live inside RAM:Beam68InstallerQA. The real SYS:
installation, ENV:, ENVARC: and running launcher are not modified. Test copies
are not started: this package tests installer file operations, not application
startup or shutdown. Ordinary application testing uses the regular installer.

The test area persists until reboot, so repeated runs test updates. The QA
runner does not delete an existing test area. This is not a release archive.
""")
    print("Ready:", executable)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "outputs/testing/Beam68-Installer-QA")
    parser.add_argument("--cc", default=os.environ.get("BEAM68_CC"))
    args = parser.parse_args()
    if not args.cc: parser.error("set BEAM68_CC or pass --cc")
    build(args.output.resolve(), args.cc)
