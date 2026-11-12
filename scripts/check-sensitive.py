#!/usr/bin/env python3
"""Scan public release inputs without displaying matched credential values."""
import os
import re
import sys
from pathlib import Path
from release_support import ReleaseError, source_files

# These rules identify common credential formats, not every possible secret.
RULES = {
    'private-key material': rb'-----BEGIN (?:RSA |EC |OPENSSH |DSA |ENCRYPTED )?PRIVATE KEY-----',
    'GitHub token': rb'(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,})',
    'GitLab token': rb'glpat-[A-Za-z0-9_-]{16,}',
    'AWS access key': rb'(?:AKIA|ASIA)[A-Z0-9]{16}',
    'Slack token': rb'xox[baprs]-[A-Za-z0-9-]{20,}',
    'credential-bearing URL': rb'https?://[^\s/<>\"]+:[^\s/<>\"]+@[^\s\"\x27]+',
    'machine-specific path': rb'(?:/Us' + rb'ers/[^/\s]+/|/private/' + rb'(?:tmp|var)/)',
}
# An exact rejection fixture, not an exception for arbitrary tests or URLs.
FIXTURES = {('tests/test_release_tool.py', 'credential-bearing URL'):
            {b'https://' + b'u:secret@github.com/org/repo'}}

def findings(name, data, secrets=()):
    result = []
    for rule, pattern in RULES.items():
        if any(match.group() not in FIXTURES.get((name, rule), set())
               for match in re.finditer(pattern, data)):
            result.append(rule)
    if any(value and value in data for value in secrets):
        result.append('current environment credential')
    return result

def scan(root):
    secrets = [value.encode() for key,value in os.environ.items()
               if key.upper().endswith(('_TOKEN','_PASSWORD','_SECRET')) and len(value)>12]
    issues = []
    files = source_files(root)
    for name in files:
        for rule in findings(name, (root/name).read_bytes(), secrets):
            issues.append((name, rule))
    if issues:
        raise ReleaseError('Sensitive-input check failed: ' + '; '.join(name+' ['+rule+']' for name,rule in issues))
    return len(files)

if __name__ == '__main__':
    try:
        print('Sensitive-input scan passed for %d public files.' % scan(Path(__file__).resolve().parents[1]))
    except ReleaseError as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
