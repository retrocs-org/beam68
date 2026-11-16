#!/usr/bin/env python3
"""Static checks for the AmigaOS Installer script and its resources.

The Installer utility cannot run on the build host, so this verifies what
can be verified: balanced syntax, defined procedures, every source file the
script copies, the plugin list, the derived base configuration and icons.
"""
import importlib.util
import re
import os
SOURCE_ONLY=os.environ.get("BEAM68_SOURCE_ONLY")=="1"
import struct
import sys
from pathlib import Path

sys.dont_write_bytecode = True
root = Path(__file__).resolve().parents[1]
script = (root / "Install").read_text(encoding="latin-1")


def tokens(text):
    """Yield '(' , ')' and string literals, skipping ';' comments."""
    i = 0
    while i < len(text):
        c = text[i]
        if c == ";":
            i = text.find("\n", i)
            if i < 0:
                return
        elif c in "\"'":
            j = i + 1
            while text[j] != c:
                j += 2 if text[j] == "\\" else 1
                assert j < len(text), "unterminated string"
            yield text[i:j + 1]
            i = j
        elif c in "()":
            yield c
        i += 1


depth = 0
for tok in tokens(script):
    if tok == "(":
        depth += 1
    elif tok == ")":
        depth -= 1
        assert depth >= 0, "unbalanced ')'"
assert depth == 0, "unbalanced '(' (depth %d)" % depth

code = re.sub(r'"(\\.|[^"\\])*"', '""', script)
code = re.sub(r";[^\n]*", "", code)
defined = re.findall(r"\(procedure\s+(P_[A-Z_]+)", code)
called = set(re.findall(r"\((P_[A-Z_]+)\)", code))
assert len(defined) == len(set(defined)), "duplicate procedure"
assert called <= set(defined), called - set(defined)
# Installer only knows a procedure once its definition has been read.
for match in re.finditer(r"\((P_[A-Z_]+)\)", code):
    assert code.index("(procedure " + match.group(1)) < match.start(), match.group(1)

# Texts: every #t_ variable used is defined in English, German and Swedish.
used = set(re.findall(r"#t_[a-z_]+", re.sub(r"\(set #t_[a-z_]+ ", "", code)))
english = re.findall(r'^\(set (#t_[a-z_]+) ', script, re.M)
assert used and used <= set(english), used - set(english)
for language in ("deutsch", "svenska"):
    block = script.split('(= @language "%s")' % language)[1].split("\n)\n")[0]
    assert re.findall(r"\(set (#t_[a-z_]+) ", block) == english, language

# Files the script copies from the distribution drawer.
for rel in ("Beam68", "Beam68.info", "Beam68Ctl", "Aliases.txt", "Beam68.cfg",
            "Beam68Prefs", "Beam68Prefs.info", "scripts/Beam68Status.rexx", "scripts/Beam68Responsiveness.rexx",
            "installer/Beam68-base.cfg", "installer/Beam68Start.info",
            "catalogs/deutsch/Beam68.catalog", "catalogs/svenska/Beam68.catalog",
            "catalogs/deutsch/Beam68Prefs.catalog", "catalogs/svenska/Beam68Prefs.catalog",
            "catalogs/deutsch/Beam68Plugins.catalog", "catalogs/svenska/Beam68Plugins.catalog",
            "Beam68.guide", "Beam68.guide.info", "Snippets.txt"):
    if not (SOURCE_ONLY and rel in {"Beam68","Beam68Prefs","Beam68Ctl"}):
        assert (root / rel).is_file(), rel
    assert '"%s"' % rel in script or rel.endswith((".info", ".catalog")), rel
languages = re.search(r'\(select #lang ((?:"[^"]+"\s*)+)\)', script).group(1)
assert sorted(re.findall(r'"([^"]+)"', languages)) == sorted(
    p.parent.name for p in root.glob("catalogs/*/Beam68.catalog"))

# Plugin list: complete bundled inventory, all installed automatically.
names = re.search(r'\(select #i ((?:"[^"]+"\s*)+)\)', script).group(1)
names = re.findall(r'"([^"]+)"', names)
on_disk = sorted(p.name for p in (root / "Plugins").iterdir() if p.is_dir())
assert sorted(names) == on_disk, (names, on_disk)
assert "(set #plugin-count %d)" % len(names) in script
assert "(set #plugin-mask %d)" % ((1 << len(names)) - 1) in script
assert '(askoptions' not in script
assert '(set #plugins #plugin-mask)' in script
for name in names:
    if not SOURCE_ONLY:
        assert (root / "Plugins" / name / name).is_file(), name
    assert (root / "Plugins" / name / (name + ".plugin")).is_file(), name

# The installer supplies plugin registration and activation; the base has neither.
spec = importlib.util.spec_from_file_location("cfg", root / "scripts/make-installer-cfg.py")
cfg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cfg)
base = (root / "installer/Beam68-base.cfg").read_text(encoding="ascii")
assert base == cfg.base_text(), "run scripts/make-installer-cfg.py"
assert "PLUGIN_DISABLED=" not in base
assert "PLUGIN=" not in "\n".join(l for l in base.splitlines() if not l.startswith("#"))


def project_icon(path):
    data = (root / path).read_bytes()
    assert struct.unpack_from(">HH", data, 0) == (0xE310, 1)
    assert data[48] == 4  # WBPROJECT
    offset = 822
    size = struct.unpack_from(">I", data, offset)[0]
    tool = data[offset + 4:offset + 3 + size].decode("ascii")
    offset += 4 + size
    types = []
    if struct.unpack_from(">I", data, 54)[0]:
        count = struct.unpack_from(">I", data, offset)[0] // 4 - 1
        offset += 4
        for _ in range(count):
            size = struct.unpack_from(">I", data, offset)[0]
            types.append(data[offset + 4:offset + 3 + size].decode("ascii"))
            offset += 4 + size
    assert offset == len(data), path
    return tool, types


tool, types = project_icon("Install.info")
assert tool == "Installer" and "SCRIPT=Install" in types and "APPNAME=Beam68" in types
assert "MINUSER=NOVICE" in types and "DEFUSER=NOVICE" in types
tool, types = project_icon("installer/Beam68Start.info")
assert tool == "SYS:Utilities/Beam68/Beam68" and "DONOTWAIT" in types
print("GUI installer: syntax, procedures, sources, plugin list, base config, icons: PASS")

# Execute the actual copy-if-missing procedure against an in-memory target.
# This checks both upgrade preservation and first-install copying.
def parse_forms(stream):
    forms = []
    for token in stream:
        if token == ")":
            return forms
        if token == "(":
            forms.append(parse_forms(stream))
        else:
            forms.append(token)
    return forms

full_tokens = (m.group(0) for m in re.finditer(
    r'"(?:\\.|[^"\\])*"|;[^\n]*|[()]|[^\s();]+', script)
    if not m.group(0).startswith(";"))
forms = parse_forms(iter(full_tokens))
procedures = {node[1]: node[2:] for node in forms
              if isinstance(node, list) and node and node[0] == "procedure"}
for source in ["Beam68.cfg", "Aliases.txt", "themes/Amber.theme",
               "themes/GreenScreen.theme", "themes/MagicGrey.theme", "themes/Paper.theme"]:
    for existing in (False, True):
        directory = "Work:Beam68/Themes" if source.startswith("themes/") else "Work:Beam68"
        target = directory + "/" + source.split("/")[-1]
        files = {target: b"user customized bytes"} if existing else {}
        values = {"#to": directory, "#src": source}
        def evaluate(node):
            if not isinstance(node, list):
                return values.get(node, node)
            operation, *args = node
            if operation == "if":
                return evaluate(args[1]) if evaluate(args[0]) else None
            if operation == "=":
                return evaluate(args[0]) == int(args[1])
            if operation == "exists":
                return int(evaluate(args[0]) in files)
            if operation == "tackon":
                return evaluate(args[0]) + "/" + evaluate(args[1])
            if operation == "fileonly":
                return evaluate(args[0]).split("/")[-1]
            if operation == "P_COPY":
                files[target] = b"new release default"
                return None
            raise AssertionError(operation)
        for expression in procedures["P_COPY_MISSING"]:
            evaluate(expression)
        assert files[target] == (b"user customized bytes" if existing else b"new release default")
assert '(set #src "Aliases.txt") (P_COPY_MISSING)' in script
assert '(set #src "Beam68.cfg") (P_COPY_MISSING)' in script
assert '(if #portable-settings' in script
assert '(set #src (tackon #dest "Beam68.cfg")) (P_COPY)' in script

# Evaluate the actual novice choices; any interactive question is a failure.
for existing, startup in (("", False), ("SYS:Beam68", False),
                           ("Work:Tools/MyLauncher", True)):
    values = {"#novice": True, "#existing": existing,
              "#default-parent": "SYS:Utilities",
              "#default-dest": "SYS:Utilities/Beam68",
              "#startup": "SYS:WBStartup/Beam68Start", "#plugin-mask": 262143}
    def novice_value(node):
        if not isinstance(node, list):
            if node.startswith('"'): return node[1:-1]
            if node.lstrip("-").isdigit(): return int(node)
            return values[node]
        op, *args = node
        if op == "if":
            return novice_value(args[1] if novice_value(args[0]) else args[2])
        if op == "=": return novice_value(args[0]) == novice_value(args[1])
        if op == "pathonly": return novice_value(args[0]).rsplit("/", 1)[0]
        if op == "tackon": return novice_value(args[0]) + "/" + novice_value(args[1])
        if op == "exists":
            path = novice_value(args[0])
            return int(startup if path == values["#startup"] else bool(existing))
        raise AssertionError("Unexpected novice prompt or operation: " + op)
    for form in procedures["P_INSTALL"]:
        if form[:1] == ["set"] and form[1] in (
                "#parent", "#dest", "#update", "#portable-settings", "#plugins", "#autostart"):
            values[form[1]] = novice_value(form[2])
    assert values["#dest"] == (existing or "SYS:Utilities/Beam68")
    assert values["#plugins"] == 262143
    assert values["#autostart"] == (int(startup) if existing else 1)
print("Novice defaults: fresh install and custom-location updates without prompts: PASS")

# Shell installer must guard every copy to a user-editable destination.
import shlex
shell = (root / "scripts/Install-Beam68").read_text()
protected = {"$Beam68InstallDir/Beam68.cfg", "$Beam68InstallDir/Aliases.txt",
             "ENV:Beam68/Beam68.cfg", "ENVARC:Beam68/Beam68.cfg",
             "ENV:Beam68/Aliases.txt", "ENVARC:Beam68/Aliases.txt"}
protected.update("$Beam68InstallDir/Themes/" + name for name in
                 ("Amber.theme", "GreenScreen.theme", "MagicGrey.theme", "Paper.theme"))
conditions = []
checked = set()
for line in shell.splitlines():
    if line.lstrip().startswith(";"):
        continue
    words = shlex.split(line.strip())
    if not words or words[0].startswith(";"):
        continue
    if words[:3] == ["IF", "NOT", "EXISTS"]:
        conditions.append((False, words[3]))
    elif words[:2] == ["IF", "EXISTS"]:
        conditions.append((True, words[2]))
    elif words[0] == "IF":
        conditions.append((False, "other condition"))
    elif words[0] == "ELSE":
        present, name = conditions.pop()
        conditions.append((not present, name))
    elif words[0] == "ENDIF":
        conditions.pop()
    elif words[0].lower() == "copy" and len(words) >= 3 and words[2] in protected:
        assert (False, words[2]) in conditions, line
        checked.add(words[2])
assert checked == protected
print("Upgrade installers: existing settings/themes preserved, missing defaults installed: PASS")

# Novice uninstall chooses the remembered directory, removes programs and keeps settings.
novice_uninstall = []
def find_novice_choices(node):
    if not isinstance(node, list): return
    if len(node) == 4 and node[:2] == ["if", "#novice"]:
        novice_uninstall.append(node)
    for child in node: find_novice_choices(child)
find_novice_choices(procedures["P_UNINSTALL"])
assert [node[2] for node in novice_uninstall] == ["#dest"]
assert "(set #keep-settings (askbool" in script
assert script.index("(set #keep-settings (askbool") < script.index("(P_STOP_LAUNCHER)",script.index("(procedure P_UNINSTALL\n"))
assert '(if #keep-settings' in script
assert script.index('(P_STOP_LAUNCHER)',script.index('(procedure P_INSTALL')) < script.index('(set #src "Beam68")')
assert '(P_START_LAUNCHER)' in script
assert 'C:Run >NIL: <NIL:' in script
assert '>NIL: WAITREADY' in script
assert 'C:Wait' not in script
assert '>NIL: SHOW' not in script
print("Uninstall: explicit consent and settings choice before stopping/removing; startup verified: PASS")

# Maintenance is offered at every user level, including Novice. Execute the
# actual main dispatch with fake prompts; verify level restoration before work.
maintenance = forms[-1]
for level in (0, 1, 2):
    for existing in ("", "Work:Beam68"):
        for action in (0, 1):
            state={"#existing":existing,"#novice":level==0,"@user-level":level}
            events=[]
            def dispatch(node):
                if not isinstance(node,list):
                    if node.startswith('"'): return node[1:-1]
                    if node.isdigit(): return int(node)
                    return state.get(node,node)
                if not node:return None
                if isinstance(node[0],list):
                    for child in node:dispatch(child)
                    return None
                op,*args=node
                if op=='if':return dispatch(args[1] if dispatch(args[0]) else args[2]) if dispatch(args[0]) or len(args)>2 else None
                if op=='<>':return dispatch(args[0])!=dispatch(args[1])
                if op=='=':return dispatch(args[0])==dispatch(args[1])
                if op=='set':state[args[0]]=dispatch(args[1]);return None
                if op=='user':state['@user-level']=dispatch(args[0]);return None
                if op=='askchoice':
                    assert state['@user-level']>0
                    assert next(x for x in args if x[0]=='choices')==['choices','#t_choice_install','#t_choice_uninstall']
                    events.append('menu');return action
                if op in ('P_INSTALL','P_UNINSTALL'):
                    assert state['@user-level']==level
                    events.append(op);return None
                raise AssertionError(op)
            dispatch(maintenance)
            assert events==(['menu','P_INSTALL' if action==0 else 'P_UNINSTALL'] if existing else ['P_INSTALL'])
print("Maintenance menu: fresh install, update/uninstall and level restoration for all levels: PASS")

main_source=(root/'src/main.c').read_text()
cleanup=main_source.split('cleanup:',1)[1]
assert cleanup.index('beam_index_free(&beam_index)') < cleanup.index('RemPort(control_port)')
assert cleanup.index('DeleteFile((STRPTR)BEAM_RECOVERY_MARKER)') < cleanup.index('RemPort(control_port)')
assert '(set @default-dest "T:")' in script
assert 'ENVARC:Beam68-Uninstall.log' in script
print("Shutdown completion: control port retained through cleanup; uninstall trace survives settings removal: PASS")

# Start resident silently, then display the actual configured shortcut.
start_branch=next(form for form in procedures['P_INSTALL']
                  if form[:1]==['if'] and isinstance(form[1],list)
                  and form[1][:2]==['if','#novice'])
assert start_branch[2]==['P_START_LAUNCHER']
assert 'INSTALLINFO' in script
assert '(getenv "Beam68-InstallerShortcut")' in script
assert '(set #t_ready (cat #t_ready #t_open_shortcut #shortcut #t_open_prefs))' in script
print("Install completion: background resident, no SHOW, actual configured shortcut: PASS")

# Live ENV: may resolve to ENVARC:. Never recurse over it after deleting the
# backing store; follow the safe files-first sequence for both aliases.
remove_settings=script.split('(if #keep-settings',1)[1].split('(complete 100)',1)[0]
assert 'Delete ENV:Beam68 ALL' not in script
assert remove_settings.index('(foreach "ENV:Beam68"') < remove_settings.index('Delete ENVARC:Beam68 ALL')
assert '(set #file "ENV:Beam68.cfg") (P_DELETE_FILE)' in remove_settings
assert '(set #file "ENVARC:Beam68.cfg") (P_DELETE_FILE)' in remove_settings
assert 'THEME=MOCHA' in base
print("Remove settings: live files before saved tree, no recursive ENV removal, legacy files removed: PASS")

# Fresh installs and newly added plugins share the six-tool default policy.
assert "(set #default-enabled-mask 63)" in script
assert "(if (= (IN #default-enabled-mask #i) 0)" in script

# Fresh installs append activation defaults as well as the full inventory.
# Updates use P_UPDATE_CONFIG and retain existing explicit choices.
fresh_registry = script.split('(set #pluginlines "")', 1)[1].split('(complete 70)', 1)[0]
assert '(if (= (IN #default-enabled-mask #i) 0)' in fresh_registry
assert '(cat #pluginlines "PLUGIN_DISABLED=PROGDIR:Plugins/" #plugin "/" #plugin ".plugin\\n")' in fresh_registry

# Remove-settings uninstall and fresh installation must discard temporary Use
# configuration: read_active checks it before the newly written saved file.
clear_session = script.split('(procedure P_CLEAR_SESSION_CONFIG', 1)[1].split('; Updates keep', 1)[0]
for suffix in ('', '.bak', '.new'):
    assert '(set #file "T:Beam68/Beam68.cfg%s") (P_DELETE_FILE)' % suffix in clear_session
fresh_config = script.split('; -- configuration: written once', 1)[1].split('; ENV:', 1)[0]
assert fresh_config.index('(P_CLEAR_SESSION_CONFIG)') < fresh_config.index('(set #cfgextra "")')
remove_config = script.split('(set #stage "Removing live settings files")', 1)[1]
assert remove_config.index('(P_CLEAR_SESSION_CONFIG)') < remove_config.index('(run "C:Delete ENVARC:Beam68 ALL QUIET FORCE")')
print("Fresh install and remove-settings uninstall clear overriding temporary configuration: PASS")
