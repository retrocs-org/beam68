#!/usr/bin/env python3
"""Isolation boundaries of the generated native installer review package."""
import importlib.util
from pathlib import Path
import re
import sys
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
spec = importlib.util.spec_from_file_location("qa", ROOT / "scripts/build-installer-qa.py")
qa = importlib.util.module_from_spec(spec)
spec.loader.exec_module(qa)
source = (ROOT / "Install").read_text(encoding="latin-1")
for uninstall in (False, True):
    text = qa.review_script(source, uninstall)
    # Strip comments and check the executable script, including translated paths.
    code = "\n".join(line for line in text.splitlines() if not line.lstrip().startswith(";"))
    for assign in ("SYS:", "ENV:", "ENVARC:", "LOCALE:"):
        assert assign not in code, assign
    assert qa.QA_ROOT + "/completed-install" in code
    assert qa.QA_ROOT + "/completed-uninstall" in code
    assert "(askdir" not in code
    assert "(getenv" not in code
    assert "QUITWAIT" not in code
    assert "Run >NIL:" not in code
    assert "(set #existing \"\")" in code
    # Every command left after adaptation is file search, append or deletion.
    for match in re.finditer(r"\(run\s+", code):
        call = code[match.start():qa.expression_end(code, match.start())]
        assert call.startswith(('(run (cat "C:Search ', '(run (cat "Echo >>',
                                '(run (cat "C:Delete ', '(run "C:Delete ')), call
    depth = 0
    for token in re.findall(r'"(?:\\.|[^"\\])*"|;[^\n]*|[()]', text):
        if token == "(": depth += 1
        elif token == ")":
            depth -= 1
            assert depth >= 0
    assert depth == 0
    if uninstall: assert "(set #action 1)" in code
    else: assert "(if #novice 0" in code
runner = (ROOT / "tests/native/installer_qa.c").read_text()
assert 'if(choice==0 || choice==5)break;' in runner
assert '"AVERAGE"' not in runner
assert '"NOVICE","NOVICE"' in runner
print("Installer QA: fixed test destinations, no live assignments or resident control: PASS")
