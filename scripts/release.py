#!/usr/bin/env python3
"""Build once, review a manifest, then publish the same Beam68 release bytes."""
import argparse
import ftplib
import hashlib
import importlib.util
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path
from release_support import (ReleaseError, atomic_json, binaries, canonical, fingerprint,
                             lha_write, runtime_files, sha, source_files, verify_lha,
                             verify_zip, zip_write)

ROOT = Path(__file__).resolve().parents[1]
ALLOWED_ENV = {'BEAM68_CC', 'BEAM68_VBCC', 'BEAM68_PHP', 'HOST_CC', 'CFLAGS', 'BEAM68_RELEASE_PYTHON', 'LHA'}
SECRET_ENV = ('GITHUB_TOKEN', 'GITLAB_TOKEN')

def need(condition, message):
    if not condition:
        raise ReleaseError(message)

def load(path):
    need(path.is_file() and not path.is_symlink(), 'Missing or linked metadata: ' + path.name)
    return json.loads(path.read_text())

def module(root, name):
    spec = importlib.util.spec_from_file_location(name.replace('-', '_'), root / 'scripts' / (name + '.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result

def repo_url(url, github=False):
    p = urllib.parse.urlsplit(url)
    need(p.scheme == 'https' and p.hostname and p.netloc == p.hostname and
         not p.username and not p.password and not p.query and not p.fragment,
         'Repository must be a credential-free HTTPS URL without a port/query')
    need(re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9.-]*', p.hostname) and
         (not github or p.hostname == 'github.com'), 'Unexpected repository host')
    parts = p.path.strip('/').split('/')
    need(len(parts) >= 2 and (not github or len(parts) == 2) and
         all(re.fullmatch(r'[A-Za-z0-9_][A-Za-z0-9_.-]*', x) and x not in ('.', '..') for x in parts),
         'Invalid repository path')
    need(not parts[-1].endswith('.git') and p.path == '/' + '/'.join(parts), 'Use the canonical repository web URL')
    return p.hostname, '/'.join(parts)

def git_url(url, web):
    host, path = repo_url(web, web.startswith('https://github.com/'))
    need(url in ('git@' + host + ':' + path + '.git', web + '.git'),
         'Git push destination does not match repository URL')
    return url

def config_read(root, path):
    defaults = {'github': 'https://github.com/retrocs-org/beam68',
                'github_git': 'git@github.com:retrocs-org/beam68.git',
                'gitlab': '', 'gitlab_git': '', 'author': 'Beam68 contributors',
                'uploader': '', 'aminet': False, 'env': {}, 'credentials': {}, 'source_branch': ''}
    if path.exists():
        raw = load(path)
        need(isinstance(raw, dict) and not set(raw) - set(defaults), 'Unknown release configuration key')
        defaults.update(raw)
    c = defaults
    need(isinstance(c['env'], dict) and not set(c['env']) - ALLOWED_ENV and
         all(isinstance(v, str) and '\0' not in v and '\n' not in v for v in c['env'].values()),
         'Only documented non-secret build environment settings are allowed')
    for key in ('github', 'github_git', 'gitlab', 'gitlab_git', 'author', 'uploader', 'source_branch'):
        need(isinstance(c[key], str) and not any(ch in c[key] for ch in ('\r', '\n', '\0')), 'Invalid configuration value: ' + key)
    for key in ('github', 'gitlab'):
        if c[key]:
            repo_url(c[key], key == 'github')
            git_url(c[key + '_git'], c[key])
        else:
            need(not c[key + '_git'], 'A Git destination requires its repository web URL')
    need(type(c['aminet']) is bool, 'aminet must be true or false')
    if c['aminet']:
        need(re.fullmatch(r'[^\s@<>]+@[^\s@<>]+\.[^\s@<>]+', c['uploader']), 'Aminet requires an uploader email contact')
    need(isinstance(c['credentials'], dict) and set(c['credentials']) <= {'github','gitlab'}, 'Invalid credential service')
    for service, spec in c['credentials'].items():
        need(isinstance(spec, dict) and set(spec) <= {'provider','variable','reference','vault','item','field','account'}, 'Invalid credential configuration')
        need(all(isinstance(v,str) and not any(ch in v for ch in ('\r','\n','\0')) for v in spec.values()), 'Invalid credential selector')
        need(spec.get('provider','env') in ('env','1password'), 'Unknown credential provider')
        if spec.get('provider','env') == 'env':
            need(re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*',spec.get('variable',service.upper()+'_TOKEN')), 'Invalid token variable')
        else:
            need((spec.get('reference','').startswith('op://') and not any(spec.get(k) for k in ('vault','item','field'))) or
                 (not spec.get('reference') and all(spec.get(k) for k in ('vault','item','field'))), 'Use one exact 1Password reference or vault/item/field')
    return c

ACTIVE_SECRETS = set()

def credential(service, config):
    spec = config.get('credentials',{}).get(service,{})
    if spec.get('provider','env') == 'env':
        token = os.environ.get(spec.get('variable',service.upper()+'_TOKEN'),'')
    else:
        reference = spec.get('reference')
        command = ['op','read','--no-newline',reference] if reference else [
            'op','item','get',spec['item'],'--vault',spec['vault'],'--fields','label='+spec['field'],'--format','json','--reveal']
        if spec.get('account'):
            command += ['--account',spec['account']]
        result = subprocess.run(command,capture_output=True,text=True)
        need(result.returncode == 0, '1Password lookup failed or was cancelled for '+service+'; no secret output displayed')
        if reference:
            token = result.stdout
        else:
            try:
                data=json.loads(result.stdout)
                fields=data if isinstance(data,list) else data.get('fields',[data])
                matches=[f['value'] for f in fields if f.get('label')==spec['field'] and f.get('value')]
                need(len(matches)==1,'1Password field missing or ambiguous; use an exact secret reference')
                token=matches[0]
            except (ValueError,TypeError,KeyError,AttributeError):
                raise ReleaseError('1Password field could not be resolved; no secret output displayed') from None
    need(isinstance(token,str) and token.strip() and not token.strip().startswith('op://') and not any(c in token for c in ('\r','\n','\0')), 'Missing/unresolved token for '+service)
    token=token.strip()
    ACTIVE_SECRETS.add(token)
    return token

def release_tag(value, version):
    tag='v'+version if value is None else value
    need(isinstance(tag,str) and re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_.-]*',tag) and '..' not in tag and not tag.endswith(('.','.lock')), 'Invalid release tag (use a simple name such as v0.1 or 0.1)')
    return tag

def run(root, args, env=None):
    clean = dict(os.environ)
    for key in list(clean):
        if key in SECRET_ENV or key.upper().endswith(('_TOKEN','_PASSWORD','_SECRET')):
            clean.pop(key, None)
    clean.update(env or {})
    result = subprocess.run(args, cwd=root, env=clean, capture_output=True)
    # Tool stderr may contain credentials from machine Git helpers; never echo it.
    if result.returncode and args[0] != 'git' and (root / 'outputs/release').is_dir():
        text = (result.stdout + result.stderr).decode(errors='replace')
        for token in ACTIVE_SECRETS | {os.environ[k] for k in SECRET_ENV if os.environ.get(k)}:
            text = text.replace(token, '[redacted]')
        log = root / 'outputs/release/failed-build.log'
        need(not log.is_symlink(), 'Linked build failure log')
        log.write_text(text)
    need(result.returncode == 0, 'Command failed: ' + Path(args[0]).name + ' (build diagnostics: outputs/release/failed-build.log)')
    return result.stdout.decode().strip()

def git(root, *args):
    return run(root, ['git', *args], {'GIT_TERMINAL_PROMPT': '0'})

def commit(root, required=False):
    try:
        top = git(root, 'rev-parse', '--show-toplevel')
        need(Path(top).samefile(root), 'Release root must be the Git checkout root')
        value = git(root, 'rev-parse', 'HEAD')
        need(re.fullmatch('[0-9a-f]{40,64}', value), 'Invalid source commit')
        return value
    except ReleaseError:
        if required:
            raise ReleaseError('Publication requires a Git checkout at the project root')
        return None

def tracked_source(root, files):
    tracked = set(git(root, 'ls-files', '-z').split('\0'))
    need(set(files) <= tracked, 'Release inputs must all be tracked; commit source changes first')

def clean_checkout(root, files):
    value = commit(root, True)
    need(not git(root, 'status', '--porcelain', '--untracked-files=all'), 'Publication requires a clean committed checkout')
    tracked_source(root, files)
    return value

def source_destination(config, service):
    need(service in ('github','gitlab') and config.get(service), 'Missing source repository for '+service)
    return git_url(config[service+'_git'],config[service])

def initialize_source(config, services, root, message=None):
    # Explicit --init may create local metadata, never an extra commit in an
    # established checkout and never a replacement for existing remote history.
    detected=subprocess.run(['git','rev-parse','--show-toplevel'],cwd=root,capture_output=True,text=True)
    if detected.returncode==0:
        need(Path(detected.stdout.strip()).samefile(root), 'Refusing initialization inside another repository')
        if commit(root):
            return
    else:
        need(not (root/'.git').exists() and not (root/'.git').is_symlink(), 'Inspect existing Git metadata before initialization')
    need((root/'.gitignore').is_file(), 'Review .gitignore before initialization')
    destinations={service:source_destination(config,service) for service in services}
    for destination in destinations.values():
        need(not git(root,'ls-remote','--refs',destination), 'Remote already has history; clone it and import source instead')
    for identity in ('GIT_AUTHOR_IDENT','GIT_COMMITTER_IDENT'):
        git(root,'var',identity)
    branch=config.get('source_branch') or 'main'
    git(root,'check-ref-format','--branch',branch)
    if detected.returncode:
        git(root,'init','--initial-branch',branch)
    for service,destination in destinations.items():
        name='origin' if service=='gitlab' else 'github'
        try:
            existing=git(root,'remote','get-url',name)
        except ReleaseError:
            existing=None
        if existing:
            need(existing==destination, 'Existing source remote differs')
        else:
            git(root,'remote','add',name,destination)
    # Inspect public inputs before staging anything, including local/private files.
    module(root,'check-sensitive').scan(root)
    git(root,'add','--all','--','.')
    git(root,'commit','-m',message or 'Initial Beam68 source')
    print('Created the first local commit. No source uploaded.')

def push_source(config, services, confirmed=False, dry_run=False, root=ROOT, initialize=False, message=None):
    need(services and len(services)==len(set(services)) and set(services)<={'github','gitlab'}, 'Source supports distinct github/gitlab destinations only')
    need(dry_run or confirmed, 'Source upload requires --confirm-push; use --dry-run first')
    destinations={service:source_destination(config,service) for service in services}
    if initialize:
        initialize_source(config,services,root,message)
    module(root,'check-sensitive').scan(root)
    head=clean_checkout(root,source_files(root))
    branch=git(root,'symbolic-ref','--quiet','--short','HEAD')
    target=config.get('source_branch') or branch
    git(root,'check-ref-format','--branch',target)
    ref='refs/heads/'+branch+':refs/heads/'+target
    for destination in destinations.values():
        git(root,'push','--dry-run','--no-follow-tags',destination,ref)
    if dry_run:
        print('Source preflight passed. No upload performed.')
        return
    for service,destination in destinations.items():
        need(clean_checkout(root,source_files(root))==head,'Source changed during push')
        git(root,'push','--no-follow-tags',destination,ref)
        remote=git(root,'ls-remote','--heads',destination,'refs/heads/'+target)
        need(remote and remote.split()[0]==head, 'Remote source branch verification failed')
        print(service+': committed source verified')

class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None

class API:
    """HTTPS only; API calls never forward authorization through redirects."""
    def __init__(self, origin, token, github=False):
        self.origin = origin
        self.headers = {'User-Agent': 'Beam68-release', 'Accept': 'application/json'}
        if github:
            self.headers.update({'Authorization': 'Bearer ' + token, 'Accept': 'application/vnd.github+json',
                                 'X-GitHub-Api-Version': '2022-11-28'})
        else:
            self.headers['PRIVATE-TOKEN'] = token
        self.opener = urllib.request.build_opener(NoRedirect())

    def open(self, url, method='GET', data=None, headers=None, missing=False):
        p = urllib.parse.urlsplit(url)
        need(p.scheme == 'https' and not p.username and not p.password, 'Invalid API destination')
        h = dict(self.headers)
        allowed = {self.origin}
        if self.origin == 'https://api.github.com':
            allowed.add('https://uploads.github.com')
        need(p.scheme + '://' + p.netloc in allowed, 'API origin mismatch')
        h.update(headers or {})
        try:
            return self.opener.open(urllib.request.Request(url, data=data, headers=h, method=method), timeout=60)
        except urllib.error.HTTPError as exc:
            if missing and exc.code == 404:
                return None
            raise ReleaseError('Release service returned HTTP ' + str(exc.code)) from None
        except (urllib.error.URLError, OSError):
            raise ReleaseError('Release service connection failed') from None

    def json(self, url, method='GET', data=None, file=None, missing=False):
        headers = {}
        if file:
            with file.open('rb') as stream:
                response = self.open(url, method, stream, {'Content-Type': 'application/octet-stream',
                                                         'Content-Length': str(file.stat().st_size)}, missing)
        else:
            body = canonical(data) if data is not None else None
            if body is not None:
                headers['Content-Type'] = 'application/json'
            response = self.open(url, method, body, headers, missing)
        if response is None:
            return None
        with response:
            raw = response.read(8 * 1024 * 1024 + 1)
        need(len(raw) <= 8 * 1024 * 1024, 'Oversized API response')
        return json.loads(raw)

    def download_hash(self, url, size):
        # Generic packages may redirect to object storage. Never forward tokens there.
        headers = dict(self.headers)
        for attempt in range(6):
            p = urllib.parse.urlsplit(url)
            need(p.hostname and p.netloc == p.hostname and p.path, 'Invalid package URL')
            need(p.scheme == 'https' and not p.username and not p.password, 'Unsafe package redirect')
            if p.scheme + '://' + p.netloc != self.origin:
                headers = {'User-Agent': 'Beam68-release'}
            try:
                response = self.opener.open(urllib.request.Request(url, headers=headers), timeout=60)
            except urllib.error.HTTPError as exc:
                if exc.code == 404 and attempt == 0:
                    return None
                if exc.code in (301, 302, 303, 307, 308):
                    location = exc.headers.get('Location')
                    need(location, 'Package redirect is missing its destination')
                    url = urllib.parse.urljoin(url, location)
                    continue
                raise ReleaseError('Package download returned HTTP ' + str(exc.code)) from None
            except (urllib.error.URLError, OSError):
                raise ReleaseError('Package verification connection failed') from None
            h = hashlib.sha256()
            total = 0
            with response:
                for chunk in iter(lambda: response.read(1024 * 1024), b''):
                    total += len(chunk)
                    need(total <= size, 'Remote package size conflicts with release')
                    h.update(chunk)
            need(total == size, 'Remote package is truncated')
            return h.hexdigest()
        raise ReleaseError('Too many package redirects')

    def pages(self, url):
        result = []
        for page in range(1, 1001):
            items = self.json(url + ('&' if '?' in url else '?') + 'per_page=100&page=' + str(page))
            need(isinstance(items, list), 'Invalid paginated response')
            result.extend(items)
            if len(items) < 100:
                return result
        raise ReleaseError('Release service pagination limit exceeded')

class GitHub:
    def __init__(self, manifest, directory, api=None, token=None):
        self.m, self.directory = manifest, directory
        host, path = repo_url(manifest['github'], True)
        self.base = 'https://api.github.com/repos/' + path
        self.upload = 'https://uploads.github.com/repos/' + path
        self.api = api or API('https://api.github.com', token or os.environ['GITHUB_TOKEN'], True)

    def release(self):
        releases = self.api.pages(self.base + '/releases')
        found = [x for x in releases if x['tag_name'] == self.m['tag']]
        need(len(found) <= 1, 'Duplicate GitHub releases')
        return found[0] if found else None

    def assets(self, release):
        ident = str(release['id'])
        need(ident.isdigit(), 'Invalid GitHub release ID')
        return self.api.pages(self.base + '/releases/' + ident + '/assets')

    def verify(self, release, complete=False):
        need((release.get('body') or '') == self.m['notes'], 'GitHub release notes conflict')
        assets = self.assets(release)
        names = [a['name'] for a in assets]
        need(len(names) == len(set(names)) and set(names) <= set(self.m['files']), 'Unexpected GitHub release assets')
        for asset in assets:
            name = asset['name']
            need(asset.get('state') == 'uploaded' and asset.get('digest') == 'sha256:' + self.m['files'][name] and
                 asset.get('size') == (self.directory / name).stat().st_size, 'Unknown or conflicting GitHub asset: ' + name)
        if complete or not release['draft']:
            need(set(names) == set(self.m['files']), 'Published GitHub release has an incomplete asset inventory')
        return set(names)

    def preflight(self):
        repo = self.api.json(self.base)
        need(repo.get('html_url') == self.m['github'] and repo.get('permissions', {}).get('push'), 'GitHub repository or write permission mismatch')
        release = self.release()
        if release:
            self.verify(release)

    def publish(self):
        release = self.release()
        if release is None:
            release = self.api.json(self.base + '/releases', 'POST',
                                    {'tag_name': self.m['tag'], 'target_commitish': self.m['commit'],
                                     'name': self.m['tag'], 'body': self.m['notes'],
                                     'draft': True, 'prerelease': '-' in self.m['version']})
        existing = self.verify(release)
        ident = str(release['id'])
        for name, digest in self.m['files'].items():
            if name not in existing:
                asset = self.api.json(self.upload + '/releases/' + ident + '/assets?name=' + urllib.parse.quote(name),
                                      'POST', file=self.directory / name)
                need(asset.get('digest') == 'sha256:' + digest and asset.get('state') == 'uploaded' and
                     asset.get('size') == (self.directory / name).stat().st_size,
                     'GitHub did not verify uploaded asset: ' + name)
        self.verify(release, True)
        if release['draft']:
            self.api.json(self.base + '/releases/' + ident, 'PATCH', {'draft': False})
        final = self.release()
        need(final and not final['draft'], 'GitHub release is still a draft')
        self.verify(final, True)

class GitLab:
    def __init__(self, manifest, directory, api=None, token=None):
        self.m, self.directory = manifest, directory
        host, path = repo_url(manifest['gitlab'])
        self.origin = 'https://' + host
        self.base = self.origin + '/api/v4/projects/' + urllib.parse.quote(path, safe='')
        self.api = api or API(self.origin, token or os.environ['GITLAB_TOKEN'])

    def package_url(self, name):
        return self.base + '/packages/generic/beam68/' + urllib.parse.quote(self.m['version'], safe='') + '/' + urllib.parse.quote(name, safe='')

    def links(self):
        return [{'name': n, 'url': self.package_url(n), 'link_type': 'package'} for n in self.m['files']]

    def release(self):
        return self.api.json(self.base + '/releases/' + urllib.parse.quote(self.m['tag'], safe=''), missing=True)

    def verify(self, release):
        need(release.get('description') == self.m['notes'], 'GitLab release notes conflict')
        links = release.get('assets', {}).get('links', [])
        actual = [(x['name'], x['url']) for x in links]
        need(len(actual) == len(set(actual)) and sorted(actual) == sorted((x['name'], x['url']) for x in self.links()), 'GitLab release links conflict')

    def packages(self, require_all=False):
        existing = set()
        for name, digest in self.m['files'].items():
            got = self.api.download_hash(self.package_url(name), (self.directory / name).stat().st_size)
            if got is not None:
                need(got == digest, 'GitLab package bytes conflict: ' + name)
                existing.add(name)
        if require_all:
            need(existing == set(self.m['files']), 'Missing GitLab packages')
        return existing

    def preflight(self):
        project = self.api.json(self.base)
        permissions = project.get('permissions', {})
        level = max((v or {}).get('access_level', 0) for v in
                    (permissions.get('project_access'), permissions.get('group_access')))
        need(project.get('web_url') == self.m['gitlab'] and level >= 40,
             'GitLab repository mismatch or Maintainer access missing')
        self.packages()
        release = self.release()
        if release:
            self.verify(release)

    def publish(self):
        existing = self.packages()
        for name in self.m['files']:
            if name not in existing:
                self.api.json(self.package_url(name), 'PUT', file=self.directory / name)
                need(self.api.download_hash(self.package_url(name), (self.directory / name).stat().st_size) == self.m['files'][name], 'GitLab upload verification failed: ' + name)
        self.packages(True)
        release = self.release()
        if release:
            self.verify(release)
        else:
            self.api.json(self.base + '/releases', 'POST', {'tag_name': self.m['tag'],
                          'name': self.m['tag'], 'description': self.m['notes'],
                          'assets': {'links': self.links()}})
        self.verify(self.release())

class HashingReader:
    def __init__(self, stream):
        self.stream, self.digest = stream, hashlib.sha256()
    def read(self, size=-1):
        data = self.stream.read(size)
        self.digest.update(data)
        return data

class Aminet:
    def __init__(self, manifest, directory, state, manifest_hash, ftp_factory=ftplib.FTP):
        self.m, self.directory, self.state, self.key = manifest, directory, state, manifest_hash
        self.factory = ftp_factory
        self.names = ['Beam68-' + manifest['version'] + '.lha', 'Beam68-' + manifest['version'] + '.readme']

    def prior(self):
        if not self.state.exists():
            return False
        receipt = load(self.state)
        need(receipt.get('manifest_sha256') == self.key, 'Aminet receipt belongs to another manifest')
        need(receipt.get('state') == 'submitted_awaiting_moderation', 'Aminet submission may be partial; inspect server and receipt before recovery')
        return True

    def connect(self):
        ftp = self.factory('main.aminet.net', timeout=60)
        try:
            ftp.login('anonymous', self.m['uploader'])
            ftp.cwd('/new')
        except Exception:
            ftp.close()
            raise
        return ftp

    def preflight(self):
        need(self.m['aminet'] and all(n in self.m['files'] for n in self.names), 'Aminet submission was not prepared')
        if self.prior():
            return
        with self.connect() as ftp:
            existing = {Path(x).name for x in ftp.nlst()}
            need(not existing.intersection(self.names), 'Aminet incoming files already exist; inspect before submitting')

    def publish(self):
        if self.prior():
            return
        self.preflight()
        receipt = {'manifest_sha256': self.key, 'state': 'in_progress', 'acknowledged': [],
                   'host': 'main.aminet.net', 'directory': '/new'}
        with self.connect() as ftp:
            atomic_json(self.state, receipt)
            for name in self.names:
                with (self.directory / name).open('rb') as stream:
                    reader = HashingReader(stream)
                    ftp.storbinary('STOR ' + name, reader)
                    need(reader.digest.hexdigest() == self.m['files'][name], 'Aminet input changed during upload; inspect partial submission')
                receipt['acknowledged'].append(name)
                atomic_json(self.state, receipt)
        receipt['state'] = 'submitted_awaiting_moderation'
        atomic_json(self.state, receipt)

class Release:
    def __init__(self, root, config, tag=None, tag_message=None):
        self.root, self.config = root, config
        self.out = root / 'outputs'
        self.state = self.out / 'release'
        version = (root / 'VERSION').read_text().strip()
        need(re.fullmatch(r'[0-9]+(?:\.[0-9]+)+(?:-[A-Za-z0-9][A-Za-z0-9.-]*)?', version), 'Invalid VERSION')
        self.version, self.tag = version, release_tag(tag,version)
        self.tag_message = tag_message or 'Beam68 '+self.tag
        need(not any(c in self.tag_message for c in ('\0','\r','\n')), 'Invalid tag annotation')
        self.record = self.state / 'build.json'
        self.manifest = self.state / 'manifest.json'
        self.names = ['Beam68-' + version + '.lha', 'Beam68-' + version + '-source.zip', 'Beam68-website.zip']
        if config['aminet']:
            self.names.append('Beam68-' + version + '.readme')
        self.payload_names = sorted(self.names)
        self.names = sorted(self.payload_names + [n+'.sha256' for n in self.payload_names])

    def plan(self):
        py=self.config['env'].get('BEAM68_RELEASE_PYTHON',sys.executable)
        return [[py, 'scripts/make-catalog.py', '--check'],
                ['make', 'host-test'], ['./scripts/build-amiga.sh', 'all'],
                [py, 'scripts/build-provenance.py', 'verify']]

    def inputs(self):
        return source_files(self.root)

    def binding(self):
        return {key: self.config[key] for key in ('github', 'github_git', 'gitlab', 'gitlab_git', 'author', 'uploader', 'aminet')}

    def native(self):
        mod = module(self.root, 'build-provenance')
        return mod.verify()

    def source_entries(self, record):
        entries = [('Beam68/' + name, (self.root / name).read_bytes(), meta['executable'])
                   for name, meta in self.inputs().items()]
        entries += [('Beam68/BUILD-PROVENANCE.json', (self.root / 'BUILD-PROVENANCE.json').read_bytes(), False),
                    ('Beam68/RELEASE-BUILD.json', canonical(record), False)]
        return entries

    def runtime_entries(self):
        return {'Beam68/' + name: (self.root / name).read_bytes() for name in runtime_files(self.root)}

    def web_entries(self):
        return [(name[len('public-web/'):], (self.root / name).read_bytes(), meta['executable'])
                for name, meta in self.inputs().items() if name.startswith('public-web/')]

    def build(self):
        self.out.mkdir(exist_ok=True)
        self.state.mkdir(exist_ok=True)
        need(not self.out.is_symlink() and not self.state.is_symlink(), 'Linked release directory')
        if (self.state / 'receipt.json').exists() or (self.state / 'aminet-receipt.json').exists():
            raise ReleaseError('Keep existing publication receipts; use another checkout for a new build')
        module(self.root,'check-sensitive').scan(self.root)
        before = self.inputs()
        if commit(self.root):
            tracked_source(self.root, before)
        env = dict(self.config['env'])
        if env.get('BEAM68_RELEASE_PYTHON'):
            env['PATH']=str(Path(env['BEAM68_RELEASE_PYTHON']).parent)+os.pathsep+os.environ.get('PATH','')
        # Tests run before native binaries exist in a source checkout.
        for args in self.plan():
            step_env = dict(env)
            if args[:2] == ['make', 'host-test']:
                step_env['BEAM68_SOURCE_ONLY'] = '1'
            run(self.root, args, step_env)
        need(self.inputs() == before, 'Release inputs changed during build; rebuild')
        native = self.native()
        record = {'schema': 1, 'version': self.version, 'source_sha256': fingerprint(before),
                  'inputs': before, 'native_record_sha256': sha(self.root / 'BUILD-PROVENANCE.json'),
                  'target': {'name': 'amiga-m68000', 'status': 'Available',
                             'compiler': native['compiler'], 'compiler_version': native['compiler_version'],
                             'runtime_validation': 'Manual hardware testing remains separate from build success'},
                  'configuration_sha256': hashlib.sha256(canonical(self.binding())).hexdigest()}
        # Preserve an older build record if preparation fails; check() will reject it.
        with tempfile.TemporaryDirectory(dir=self.out, prefix='.prepare-') as stage_name:
            stage = Path(stage_name)
            lha_write(stage / ('Beam68-' + self.version + '.lha'), self.runtime_entries(), self.config['env'].get('LHA'))
            zip_write(stage / ('Beam68-' + self.version + '-source.zip'), self.source_entries(record))
            zip_write(stage / 'Beam68-website.zip', self.web_entries())
            if self.config['aminet']:
                mod = module(self.root, 'make-aminet-release')
                (stage / ('Beam68-' + self.version + '.readme')).write_bytes(
                    mod.readme(self.config['author'], self.config['uploader'], 'Beam68-' + self.version + '.lha'))
            for name in self.payload_names:
                (stage/(name+'.sha256')).write_text(sha(stage/name)+'  '+name+'\n')
            sums = ''.join(sha(stage / name) + '  ' + name + '\n' for name in self.names)
            (stage / 'SHA256SUMS').write_text(sums)
            need(self.inputs() == before, 'Release inputs changed during packaging')
            # Never remove unexpected output files silently.
            existing = {p.name for p in self.out.iterdir() if p.name != 'release' and p != stage}
            need(existing <= set(self.names + ['SHA256SUMS']), 'Unexpected files in outputs/')
            for name in self.names + ['SHA256SUMS']:
                target = self.out / name
                need(not target.is_symlink(), 'Linked release asset')
                os.replace(stage / name, target)
        atomic_json(self.record, record)
        self.check()

    def check(self):
        module(self.root,'check-sensitive').scan(self.root)
        record = load(self.record)
        inputs = self.inputs()
        need(record.get('inputs') == inputs and record.get('source_sha256') == fingerprint(inputs), 'Stale release source fingerprint; rebuild')
        need(record.get('version') == self.version and record.get('configuration_sha256') == hashlib.sha256(canonical(self.binding())).hexdigest(), 'Release build configuration changed; rebuild')
        self.native()
        need(record.get('native_record_sha256') == sha(self.root / 'BUILD-PROVENANCE.json'), 'Native build record changed; rebuild')
        expected = set(self.names + ['SHA256SUMS'])
        need(not self.out.is_symlink() and not self.state.is_symlink(), 'Linked release directory')
        actual = {p.name for p in self.out.iterdir() if p.name != 'release'}
        need(actual == expected, 'Missing or unexpected release inventory')
        for name in expected:
            need((self.out / name).is_file() and not (self.out / name).is_symlink(), 'Missing or linked release asset: ' + name)
        lines = (self.out / 'SHA256SUMS').read_text().splitlines()
        sums = {}
        for line in lines:
            match = re.fullmatch(r'([0-9a-f]{64})  ([A-Za-z0-9_.-]+)', line)
            need(match and match[2] not in sums, 'Invalid or duplicate checksum entry')
            sums[match[2]] = match[1]
        need(set(sums) == set(self.names), 'Checksum inventory mismatch')
        need(all(sha(self.out / n) == h for n, h in sums.items()), 'Release artifact checksum changed')
        for name in self.payload_names:
            need((self.out/(name+'.sha256')).read_text()==sha(self.out/name)+'  '+name+'\n', 'Changed per-file checksum')
        verify_lha(self.out / ('Beam68-' + self.version + '.lha'), self.runtime_entries(), self.config['env'].get('LHA'))
        verify_zip(self.out / ('Beam68-' + self.version + '-source.zip'), {n: d for n, d, _ in self.source_entries(record)})
        verify_zip(self.out / 'Beam68-website.zip', {n: d for n, d, _ in self.web_entries()})
        if self.config['aminet']:
            mod = module(self.root, 'make-aminet-release')
            need((self.out / ('Beam68-' + self.version + '.readme')).read_bytes() ==
                 mod.readme(self.config['author'], self.config['uploader'], 'Beam68-' + self.version + '.lha'), 'Aminet readme changed')
        return {name: sha(self.out / name) for name in sorted(expected)}

    def prepare(self, notes):
        files = self.check()
        text = notes.read_text().strip()
        need(text and len(text.encode()) <= 100000, 'Missing or oversized release notes')
        need(not any(token and token in text for token in (os.environ.get(x) for x in SECRET_ENV)), 'Release notes contain a credential')
        manifest = {'schema': 1, 'version': self.version, 'tag': self.tag, 'source_sha256': fingerprint(self.inputs()),
                    'tag_message': self.tag_message, 'commit': commit(self.root), 'build_sha256': sha(self.record), 'notes': text, 'files': files,
                    **self.binding()}
        if self.manifest.exists() and load(self.manifest) != manifest:
            need(not (self.state / 'receipt.json').exists() and not (self.state / 'aminet-receipt.json').exists(),
                 'Prepared manifest has publication receipts; do not replace it')
        stage=self.state/'publish'/'outputs'
        need(not stage.is_symlink() and not stage.parent.is_symlink(), 'Linked publication stage')
        if stage.exists():
            need(not stage.is_symlink() and not stage.parent.is_symlink(), 'Linked publication stage')
            need({p.name for p in stage.iterdir()} <= set(files), 'Unexpected prepared assets; inspect stage')
        stage.mkdir(parents=True,exist_ok=True)
        for name in files:
            target=stage/name
            need(not target.is_symlink(), 'Linked prepared asset')
            # Never replace reviewed bytes once a receipt exists.
            if (self.state/'receipt.json').exists() or (self.state/'aminet-receipt.json').exists():
                need(target.is_file() and sha(target)==files[name], 'Receipted prepared asset changed')
            else:
                shutil.copyfile(self.out/name,target)
        self.prepared_check(manifest)
        atomic_json(self.manifest, manifest)
        return manifest

    def prepared_check(self, manifest):
        stage=self.state/'publish'/'outputs'
        need(not stage.is_symlink() and not stage.parent.is_symlink() and stage.is_dir(), 'Missing or linked publication stage')
        need({p.name for p in stage.iterdir()}==set(manifest['files']), 'Prepared publication inventory differs')
        for name,digest in manifest['files'].items():
            path=stage/name
            need(path.is_file() and not path.is_symlink() and sha(path)==digest, 'Prepared asset changed: '+name)
        return stage

    def manifest_check(self):
        m = load(self.manifest)
        need(m.get('files') == self.check() and m.get('build_sha256') == sha(self.record) and
             m.get('source_sha256') == fingerprint(self.inputs()) and m.get('version') == self.version and
             m.get('tag') == self.tag and m.get('tag_message') == self.tag_message, 'Prepared manifest is stale; prepare again')
        need(all(m.get(k) == v for k, v in self.binding().items()), 'Prepared destinations changed')
        need(m.get('commit') == clean_checkout(self.root, self.inputs()), 'Prepared commit differs; prepare again')
        need(isinstance(m.get('notes'), str) and m['notes'], 'Missing prepared notes')
        self.prepared_check(m)
        return m

    def tag_preflight(self, m, remote):
        try:
            local = git(self.root, 'rev-parse', '--verify', 'refs/tags/' + m['tag'])
        except ReleaseError:
            local = None
        if local:
            need(git(self.root, 'cat-file', '-t', 'refs/tags/' + m['tag']) == 'tag', 'Existing local release tag must be annotated')
            need(git(self.root, 'rev-parse', 'refs/tags/' + m['tag'] + '^{commit}') == m['commit'], 'Local tag points to a different commit')
        lines = git(self.root, 'ls-remote', '--tags', remote, 'refs/tags/' + m['tag'], 'refs/tags/' + m['tag'] + '^{}').splitlines()
        refs = dict(line.split('\t')[::-1] for line in lines if line)
        peeled = refs.get('refs/tags/' + m['tag'] + '^{}')
        direct = refs.get('refs/tags/' + m['tag'])
        if direct:
            need(peeled == m['commit'], 'Remote tag differs or is not annotated')
            if local:
                need(direct == local, 'Remote annotated tag object differs from local tag')
        return direct

    def push_tag(self, m, remote):
        existing = self.tag_preflight(m, remote)
        if existing:
            # Fetch only this tag if no local tag exists. Never move an existing tag.
            try:
                git(self.root, 'rev-parse', '--verify', 'refs/tags/' + m['tag'])
            except ReleaseError:
                git(self.root, 'fetch', '--no-tags', remote, 'refs/tags/' + m['tag'] + ':refs/tags/' + m['tag'])
        else:
            try:
                git(self.root, 'rev-parse', '--verify', 'refs/tags/' + m['tag'])
            except ReleaseError:
                git(self.root, '-c', 'tag.gpgSign=false', 'tag', '-a', m['tag'], m['commit'], '-m', m.get('tag_message','Beam68 '+m['tag']))
            git(self.root, 'push', '--no-follow-tags', remote, 'refs/tags/' + m['tag'] + ':refs/tags/' + m['tag'])
        need(self.tag_preflight(m, remote), 'Remote tag verification failed')

    def publish(self, destinations, confirmed):
        need(confirmed, 'Publication requires --confirm-publish')
        need(destinations and len(destinations) == len(set(destinations)) and set(destinations) <= {'github', 'gitlab', 'aminet'}, 'Invalid or duplicate destinations')
        m = self.manifest_check()
        key = sha(self.manifest)
        receipt_path = self.state / 'receipt.json'
        receipt = load(receipt_path) if receipt_path.exists() else {'manifest_sha256': key, 'current': None, 'completed': []}
        need(receipt.get('manifest_sha256') == key, 'Receipt belongs to another manifest')
        adapters = {}
        tokens = {}
        stage = self.prepared_check(m)
        # Validate every selected credential before even the first remote request.
        for target in destinations:
            if target != 'aminet':
                need(m[target], 'Missing repository configuration or token for '+target)
                tokens[target]=credential(target,self.config)
        remote_tags = []
        for target in destinations:
            if target == 'aminet':
                adapter = Aminet(m, stage, self.state / 'aminet-receipt.json', key)
            else:
                direct = self.tag_preflight(m, m[target + '_git'])
                if direct:
                    remote_tags.append(direct)
                adapter = (GitHub if target == 'github' else GitLab)(m, stage, token=tokens[target])
            adapters[target] = adapter
            adapter.preflight()
        need(len(set(remote_tags)) <= 1, 'Destinations contain different annotated tag objects')
        # Revalidate after potentially slow preflight and before any mutation.
        self.manifest_check()
        for target in destinations:
            if target in receipt['completed']:
                continue
            receipt['current'] = target
            atomic_json(receipt_path, receipt)
            if target != 'aminet':
                # Reuse a remote annotated tag before creating a new one, ensuring
                # every destination receives the identical annotated tag object.
                for other in destinations:
                    if other != 'aminet' and self.tag_preflight(m, m[other + '_git']):
                        self.push_tag(m, m[other + '_git'])
                        break
                self.push_tag(m, m[target + '_git'])
            need(self.check() == m['files'], 'Artifacts changed before upload')
            self.prepared_check(m)
            adapters[target].publish()
            receipt['completed'].append(target)
            receipt['current'] = None
            atomic_json(receipt_path, receipt)
        return receipt

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=('source','plan', 'build', 'check', 'prepare', 'publish','release'))
    parser.add_argument('--config', type=Path, default=Path(os.environ.get('BEAM68_RELEASE_CONFIG',str(Path.home()/'Development/tools/git-release/projects/beam68.json'))))
    parser.add_argument('--notes', type=Path, default=ROOT / 'docs/RELEASE-NOTES.md')
    parser.add_argument('--to', default='gitlab,github')
    parser.add_argument('--confirm-publish', action='store_true')
    parser.add_argument('--confirm-push',action='store_true')
    parser.add_argument('--dry-run',action='store_true')
    parser.add_argument('--init',dest='initialize',action='store_true')
    parser.add_argument('--commit-message')
    parser.add_argument('--tag')
    parser.add_argument('--tag-message')
    args = parser.parse_args(argv)
    need(args.config.is_file(), 'Create external release settings or pass --config; see docs/RELEASING.md')
    tool = Release(ROOT, config_read(ROOT, args.config),args.tag,args.tag_message)
    if args.command=='source':
        push_source(tool.config,args.to.split(','),args.confirm_push,args.dry_run,ROOT,args.initialize,args.commit_message)
        return
    need(not args.initialize and not args.confirm_push and not args.dry_run and args.commit_message is None, 'Source options apply only to source')
    need(args.command in ('prepare','publish','release') or (args.tag is None and args.tag_message is None), 'Tag options apply only to prepare/publish/release')
    if args.command == 'plan':
        for command in tool.plan():
            print(shlex.join(command))
        print('Package exact source, website and Amiga LHA (LH5 with configured LHA; stored otherwise) into outputs/.')
        print('Target: Amiga m68000 — Available; runtime certification is separate.')
        return
    if args.command == 'build':
        tool.build()
    elif args.command == 'check':
        tool.check()
    elif args.command == 'prepare':
        tool.prepare(args.notes)
    else:
        need(args.confirm_publish, 'Publication requires --confirm-publish')
        if args.command=='release':
            tool.prepare(args.notes)
        tool.publish(args.to.split(','), args.confirm_publish)
    print(args.command.capitalize() + ' completed. ' + ('No publication performed.' if args.command not in ('publish','release') else 'See outputs/release/receipt.json.'))

if __name__ == '__main__':
    try:
        main()
    except (ReleaseError, OSError, ValueError, KeyError, ftplib.Error) as exc:
        # Never log service response bodies, configuration values or credentials.
        message = str(exc) if isinstance(exc, ReleaseError) else 'Release operation failed; inspect local configuration and receipts.'
        for token in ACTIVE_SECRETS | {os.environ[k] for k in SECRET_ENV if os.environ.get(k)}:
            message = message.replace(token, '[redacted]')
        print('Release refused: ' + message, file=sys.stderr)
        sys.exit(1)
