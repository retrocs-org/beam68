"""Offline release acceptance tests. No real HTTP/FTP requests or publication."""
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
import release as rel
from release_support import ReleaseError, atomic_json, canonical, crc16, sha, source_files

class FixtureRelease(rel.Release):
    def native(self):
        record = rel.load(self.root / 'BUILD-PROVENANCE.json')
        for name, digest in record['binaries'].items():
            rel.need(sha(self.root / name) == digest, 'Changed native binary')
        return record

class GithubAPI:
    def __init__(self):
        self.r = None
        self.assets = []
        self.events = []
        self.bad_digest = False
        self.fail_upload = None
    def pages(self, url):
        return copy.deepcopy(self.assets if '/assets?' in url or url.endswith('/assets') else ([self.r] if self.r else []))
    def json(self, url, method='GET', data=None, file=None, missing=False):
        self.events.append((method, url))
        if method == 'GET':
            return {'html_url': 'https://github.com/retrocs-org/beam68', 'permissions': {'push': True}}
        if file:
            if self.fail_upload == file.name:
                raise ReleaseError('Injected interrupted upload')
            a = {'name': file.name, 'digest': None if self.bad_digest else 'sha256:' + sha(file),
                 'state': 'uploaded', 'size': file.stat().st_size}
            self.assets.append(a)
            return copy.deepcopy(a)
        if method == 'POST':
            self.r = dict(data, id=1)
            return copy.deepcopy(self.r)
        if method == 'PATCH':
            self.r.update(data)
            return copy.deepcopy(self.r)
        raise AssertionError((method, url))

class GitlabAPI:
    def __init__(self, url):
        self.url, self.r, self.files, self.events = url, None, {}, []
        self.corrupt = False
    def json(self, url, method='GET', data=None, file=None, missing=False):
        self.events.append((method, url))
        if method == 'PUT':
            self.files[url] = file.read_bytes() + (b'bad' if self.corrupt else b'')
            return {'message': '201 Created'}
        if method == 'POST':
            self.r = data
            return data
        if '/releases/' in url:
            return self.r
        return {'web_url': self.url, 'permissions': {'group_access': {'access_level': 40}}}
    def download_hash(self, url, size):
        self.events.append(('VERIFY', url))
        data = self.files.get(url)
        return hashlib.sha256(data).hexdigest() if data is not None else None

class FTP:
    uploads = []
    fail = False
    existing = []
    def __init__(self, host, timeout):
        assert host == 'main.aminet.net'
    def login(self, user, contact):
        assert user == 'anonymous' and '@' in contact
    def cwd(self, name):
        assert name == '/new'
    def nlst(self):
        return self.existing
    def storbinary(self, command, stream):
        if self.fail and self.uploads:
            raise OSError('interrupted')
        self.uploads.append((command, stream.read()))
    def close(self):
        pass
    def __enter__(self):
        return self
    def __exit__(self, *args):
        pass

class ReleaseTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name) / 'project'
        self.root.mkdir()
        contents = {'VERSION': '0.1\n', 'LICENSE': 'MIT\n', 'README.md': '# Beam68\nLauncher.\n',
                    'CHANGELOG.md': '# Changes\n\nInitial release.\n', 'Makefile': 'all:\n',
                    'src/main.c': 'int main(void){return 0;}\n', 'scripts/release.py': '# release\n',
                    'public-web/index.php': '<?php echo "Beam68";\n', 'sdk/README.md': 'Protocol 1\n',
                    'sdk/include/beam68plugin.h':'/* API */\n',
                    'sdk/src/beam68plugin.c':'/* SDK implementation */\n',
                    'sdk/Beam68PluginSDK.guide':'manual\n',
                    'sdk/Example/HelloWorld/HelloWorld.c':'/* example */\n',
                    'sdk/Example/HelloRexx/HelloRexx.rexx':'/* example */\n',
                    'sdk/Example/Makefile':'all:\n',
                    'sdk/tools/package-plugin.py':'# package validator\n',
                    'docs/RELEASE-NOTES.md': 'Reviewed notes\n',
                    'scripts/build-install-amiga.sh': 'runtime_executables="\nBeam68\n"\nruntime_data="\nLICENSE\n"\n'}
        for name, text in contents.items():
            p = self.root / name
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(text)
        # LICENSE is automatically added by runtime selection; use a distinct data file.
        (self.root / 'scripts/build-install-amiga.sh').write_text('runtime_executables="\nBeam68\n"\nruntime_data="\nVERSION\n"\n')
        shutil.copy(ROOT / 'scripts/make-aminet-release.py', self.root / 'scripts/make-aminet-release.py')
        shutil.copy(ROOT / 'scripts/check-sensitive.py', self.root / 'scripts/check-sensitive.py')
        self.config = rel.config_read(self.root, self.root / 'work/release-config.json')
        self.tool = FixtureRelease(self.root, self.config)
        self.commands = []
    def tearDown(self):
        self.tmp.cleanup()
    def command(self, root, args, env=None):
        if args[0] == 'git':
            return ORIGINAL_RUN(root, args, env)
        self.commands.append(args)
        if args[0] == './scripts/build-amiga.sh':
            (self.root / 'Beam68').write_bytes(b'\0\0\3\xf3native')
            atomic_json(self.root / 'BUILD-PROVENANCE.json', {
                'binaries': {'Beam68': sha(self.root / 'Beam68')}, 'compiler': 'test-gcc',
                'compiler_version': 'test-gcc 1', 'version': '0.1'})
        return ''
    def build(self, aminet=False):
        if aminet:
            self.config.update(aminet=True, uploader='maintainer@example.net')
            self.tool = FixtureRelease(self.root, self.config)
        with patch.object(rel, 'run', side_effect=self.command):
            self.tool.build()
        return self.tool.prepare(self.root / 'docs/RELEASE-NOTES.md')
    def initgit(self):
        def command(*args):
            return ORIGINAL_RUN(self.root, ['git', *args])
        command('init', '-q')
        command('config', 'user.email', 'test@example.net')
        command('config', 'user.name', 'Test')
        (self.root / '.gitignore').write_text('/outputs/\n/Beam68\n/BUILD-PROVENANCE.json\n/work/\n')
        command('add', '.')
        command('commit', '-qm', 'source')
        return command
    def test_plan_has_no_mutations_and_all_build_steps(self):
        with patch.object(rel, 'run', side_effect=AssertionError('plan ran a command')):
            plan = self.tool.plan()
        self.assertTrue(any(c[0] == 'make' for c in plan))
        self.assertIn(['./scripts/build-amiga.sh', 'all'], plan)
        self.assertFalse(self.tool.out.exists())
    def test_build_check_manifest_and_exact_contents(self):
        m = self.build()
        self.assertEqual(len(self.commands), 4)
        self.assertIsNone(m['commit'])
        self.assertEqual(m['tag'], 'v0.1')
        self.assertEqual(self.tool.check(), m['files'])
        with zipfile.ZipFile(self.tool.out / 'Beam68-0.1-plugin-sdk.zip') as z:
            self.assertIn('Beam68-PluginSDK/LICENSE', z.namelist())
            self.assertIn('Beam68-PluginSDK/include/beam68plugin.h', z.namelist())
            self.assertNotIn('Beam68/Beam68', z.namelist())
        self.assertEqual(set(m['files']), {'Beam68-0.1.lha','Beam68-0.1.lha.sha256','Beam68-0.1-plugin-sdk.zip','Beam68-0.1-plugin-sdk.zip.sha256'})
    def test_source_change_and_binary_change_refused(self):
        self.build()
        (self.root / 'Beam68').write_bytes(b'changed')
        with self.assertRaises(ReleaseError): self.tool.check()
        (self.root / 'src/main.c').write_text('changed')
        with self.assertRaisesRegex(ReleaseError, 'fingerprint'): self.tool.check()
    def test_input_change_during_build_refused(self):
        def changed(root, args, env=None):
            result = self.command(root, args, env)
            if args[0] == './scripts/build-amiga.sh':
                (root / 'README.md').write_text('changed during build')
            return result
        with patch.object(rel, 'run', side_effect=changed), self.assertRaisesRegex(ReleaseError, 'changed during build'):
            self.tool.build()
    def test_missing_duplicate_extra_and_changed_checksums(self):
        self.build()
        sums = self.tool.out / 'Beam68-0.1.lha.sha256'
        original = sums.read_text()
        for bad in (original+original,'',original.replace(original[:64],'0'*64)):
            sums.write_text(bad)
            with self.assertRaises(ReleaseError): self.tool.check()
        sums.write_text(original)
        (self.tool.out / 'extra').write_text('extra')
        with self.assertRaisesRegex(ReleaseError, 'inventory'): self.tool.check()
    def test_link_and_private_source_rejected(self):
        p = self.root / 'src/link.c'
        p.symlink_to(self.root / 'src/main.c')
        with self.assertRaises(ReleaseError): self.tool.inputs()
        p.unlink()
        (self.root / '.env').write_text('private')
        with self.assertRaisesRegex(ReleaseError, 'Private'): self.tool.inputs()
    def test_ignored_config_not_packaged_and_tokens_not_serialized(self):
        (self.root / 'work').mkdir()
        (self.root / 'work/release-config.json').write_text('{"private":"value"}')
        with patch.dict(os.environ, {'GITHUB_TOKEN': 'a-test-token-not-for-network'}):
            m = self.build()
        self.assertNotIn('a-test-token-not-for-network', json.dumps(m))
        with zipfile.ZipFile(self.tool.out / 'Beam68-0.1-plugin-sdk.zip') as z:
            self.assertFalse(any('/work/' in n for n in z.namelist()))
    def test_credentials_and_destination_validation(self):
        for url in ('https://u:secret@github.com/org/repo', 'https://github.com/org/../repo',
                    'https://github.com/org/repo?token=x', 'http://github.com/org/repo',
                    'https://evil.example/org/repo', 'https://github.com/org/%2e%2e'):
            with self.assertRaises(ReleaseError): rel.repo_url(url, True)
        rel.repo_url('https://gitlab.example.net/group/nested/beam68')
        with self.assertRaises(ReleaseError): rel.git_url('git@evil.example:org/repo.git', self.config['github'])
        config = self.root / 'bad.json'
        config.write_text(json.dumps({'env': {'GITHUB_TOKEN': 'not-allowed'}}))
        with self.assertRaises(ReleaseError): rel.config_read(self.root, config)
    def test_confirm_missing_token_non_git_and_dirty_guards(self):
        m = self.build()
        with self.assertRaisesRegex(ReleaseError, 'confirm'): self.tool.publish(['github'], False)
        with self.assertRaisesRegex(ReleaseError, 'Git checkout'): self.tool.manifest_check()
        with patch.object(self.tool, 'manifest_check', return_value=m), patch.dict(os.environ, {}, clear=True), patch.object(rel, 'GitHub') as api:
            with self.assertRaisesRegex(ReleaseError, 'token'): self.tool.publish(['github'], True)
            api.assert_not_called()
        git = self.initgit()
        # .gitignore changed the release source; rebuild and prepare.
        with patch.object(rel, 'run', side_effect=self.command): self.tool.build()
        self.tool.prepare(self.root / 'docs/RELEASE-NOTES.md')
        self.tool.manifest_check()
        (self.root / 'untracked.c').write_text('input')
        with self.assertRaises(ReleaseError): self.tool.manifest_check()
        (self.root / 'untracked.c').unlink()
        (self.root / 'README.md').write_text('dirty')
        with self.assertRaises(ReleaseError): rel.clean_checkout(self.root, self.tool.inputs())
    def test_ignored_untracked_inputs_refused(self):
        git = self.initgit()
        (self.root / '.gitignore').write_text((self.root / '.gitignore').read_text() + '/ignored.c\n')
        git('add', '.gitignore');git('commit', '-qm', 'ignore')
        (self.root / 'ignored.c').write_text('release input')
        with self.assertRaisesRegex(ReleaseError, 'tracked'): rel.clean_checkout(self.root, self.tool.inputs())
    def test_github_draft_published_only_after_hashes(self):
        m = self.build();api = GithubAPI();adapter = rel.GitHub(m, self.tool.out, api)
        adapter.preflight();adapter.publish()
        self.assertFalse(api.r['draft'])
        patch_index = next(i for i, e in enumerate(api.events) if e[0] == 'PATCH')
        uploads = [i for i, e in enumerate(api.events) if 'uploads.github.com' in e[1]]
        self.assertEqual(len(uploads), len(m['files']))
        self.assertTrue(all(i < patch_index for i in uploads))
        adapter.publish()
        self.assertEqual(len([e for e in api.events if 'uploads.github.com' in e[1]]), len(m['files']))
    def test_unknown_digest_and_conflict_keep_draft(self):
        m = self.build();api = GithubAPI();api.bad_digest = True
        with self.assertRaises(ReleaseError): rel.GitHub(m, self.tool.out, api).publish()
        self.assertTrue(api.r['draft'])
        with self.assertRaises(ReleaseError): rel.GitHub(m, self.tool.out, api).preflight()
        self.assertFalse(any(e[0] == 'PATCH' for e in api.events))
    def test_matching_github_assets_reused_after_interruption(self):
        m = self.build();api = GithubAPI();api.fail_upload = list(m['files'])[1]
        with self.assertRaises(ReleaseError): rel.GitHub(m, self.tool.out, api).publish()
        self.assertEqual(len(api.assets), 1)
        api.fail_upload = None
        rel.GitHub(m, self.tool.out, api).publish()
        self.assertEqual(len(api.assets), len(m['files']))
    def test_gitlab_downloads_before_advertising_and_refuses_conflict(self):
        m = self.build();m['gitlab'] = 'https://gitlab.example.net/team/nested/beam68'
        api = GitlabAPI(m['gitlab']);adapter = rel.GitLab(m, self.tool.out, api)
        adapter.preflight();adapter.publish()
        post = next(i for i, e in enumerate(api.events) if e[0] == 'POST')
        for name in m['files']:
            self.assertTrue(any(i < post and e == ('VERIFY', adapter.package_url(name)) for i,e in enumerate(api.events)))
        api.files[adapter.package_url(next(iter(m['files'])))] = b'conflict'
        with self.assertRaises(ReleaseError): adapter.preflight()
    def test_gitlab_corruption_never_creates_release(self):
        m = self.build();m['gitlab'] = 'https://gitlab.example.net/team/beam68'
        api = GitlabAPI(m['gitlab']);api.corrupt = True
        with self.assertRaises(ReleaseError): rel.GitLab(m, self.tool.out, api).publish()
        self.assertIsNone(api.r)
    def test_aminet_partial_not_resubmitted_and_success_is_submission(self):
        m = self.build(True);FTP.uploads=[];FTP.fail=True;FTP.existing=[]
        adapter = rel.Aminet(m, self.tool.state/'aminet', self.tool.state / 'aminet-receipt.json', sha(self.tool.manifest), FTP)
        with self.assertRaises(OSError): adapter.publish()
        self.assertEqual(len(FTP.uploads), 1)
        with self.assertRaisesRegex(ReleaseError, 'partial'): adapter.publish()
        with self.assertRaisesRegex(ReleaseError, 'receipts'): self.tool.build()
    def test_aminet_success_receipt_and_manifest_mismatch(self):
        m = self.build(True);FTP.uploads=[];FTP.fail=False;FTP.existing=[]
        state = self.tool.state / 'aminet-receipt.json'
        adapter = rel.Aminet(m, self.tool.state/'aminet', state, sha(self.tool.manifest), FTP)
        adapter.preflight();adapter.publish();adapter.publish()
        self.assertEqual(len(FTP.uploads), 2)
        self.assertEqual(rel.load(state)['state'], 'submitted_awaiting_moderation')
        with self.assertRaises(ReleaseError): rel.Aminet(m,self.tool.state/'aminet',state,'changed',FTP).preflight()
    def test_receipt_failure_preserves_completed_destination(self):
        m = self.build();m['commit'] = 'a'*40;m['gitlab'] = 'https://gitlab.example.net/team/beam68';m['gitlab_git']='git@gitlab.example.net:team/beam68.git'
        class Adapter:
            def __init__(self, fail=False): self.fail=fail;self.count=0
            def preflight(self): pass
            def publish(self):
                self.count+=1
                if self.fail: raise ReleaseError('later destination failed')
        first,second=Adapter(),Adapter(True)
        with patch.object(self.tool,'manifest_check',return_value=m), patch.object(self.tool,'tag_preflight',return_value=None), patch.object(self.tool,'push_tag'), patch.object(rel,'GitHub',return_value=first), patch.object(rel,'GitLab',return_value=second), patch.dict(os.environ,{'GITHUB_TOKEN':'x','GITLAB_TOKEN':'y'}):
            with self.assertRaises(ReleaseError): self.tool.publish(['github','gitlab'],True)
            self.assertEqual(rel.load(self.tool.state/'receipt.json')['completed'],['github'])
            second.fail=False;self.tool.publish(['github','gitlab'],True)
            self.assertEqual(first.count,1)
        receipt=rel.load(self.tool.state/'receipt.json');receipt['manifest_sha256']='other';atomic_json(self.tool.state/'receipt.json',receipt)
        with patch.object(self.tool,'manifest_check',return_value=m), self.assertRaisesRegex(ReleaseError,'another manifest'):
            self.tool.publish(['github'],True)
    def test_conflicting_tags_and_same_tag_push(self):
        git=self.initgit();head=git('rev-parse','HEAD')
        remote=Path(self.tmp.name)/'remote.git'
        subprocess.run(['git','init','--bare','-q',str(remote)],check=True)
        m={'tag':'v0.1','version':'0.1','commit':head}
        self.tool.push_tag(m,str(remote))
        self.assertEqual(git('cat-file','-t','refs/tags/v0.1'),'tag')
        self.tool.push_tag(m,str(remote))
        with self.assertRaises(ReleaseError): self.tool.tag_preflight(dict(m,commit='b'*40),str(remote))
    def test_zip_duplicate_or_changed_members_rejected_even_with_new_checksum(self):
        self.build()
        archive=self.tool.out/'Beam68-0.1-plugin-sdk.zip'
        with zipfile.ZipFile(archive,'a') as z:
            z.writestr('Beam68/unexpected.txt','unexpected')
        (self.tool.out/(archive.name+'.sha256')).write_text(sha(archive)+'  '+archive.name+'\n')
        record=rel.load(self.tool.record)
        record['artifacts'][archive.name]=sha(archive)
        record['artifacts'][archive.name+'.sha256']=sha(self.tool.out/(archive.name+'.sha256'))
        atomic_json(self.tool.record,record)
        with self.assertRaises(ReleaseError): self.tool.check()
    def test_linked_artifact_rejected(self):
        self.build()
        asset=self.tool.out/'Beam68-0.1-plugin-sdk.zip';copy_path=Path(self.tmp.name)/'asset.zip'
        shutil.copy(asset,copy_path);asset.unlink();asset.symlink_to(copy_path)
        with self.assertRaisesRegex(ReleaseError,'linked'): self.tool.check()
    def test_notes_and_unexpected_assets_conflict(self):
        m=self.build();api=GithubAPI();adapter=rel.GitHub(m,self.tool.out,api);adapter.publish()
        api.r['body']='different notes'
        with self.assertRaisesRegex(ReleaseError,'notes'): adapter.preflight()
        api.r['body']=m['notes'];api.assets.append({'name':'unexpected'})
        with self.assertRaisesRegex(ReleaseError,'Unexpected'): adapter.preflight()
    def test_all_destinations_preflight_before_any_tag_push(self):
        m=self.build();m['gitlab']='https://gitlab.example.net/team/beam68';m['gitlab_git']='git@gitlab.example.net:team/beam68.git'
        class Good:
            def preflight(self): pass
            def publish(self): raise AssertionError('published before preflight completed')
        class Bad:
            def preflight(self): raise ReleaseError('conflicting service state')
        with patch.object(self.tool,'manifest_check',return_value=m), patch.object(self.tool,'tag_preflight',return_value=None), patch.object(self.tool,'push_tag') as push, patch.object(rel,'GitHub',return_value=Good()), patch.object(rel,'GitLab',return_value=Bad()), patch.dict(os.environ,{'GITHUB_TOKEN':'x','GITLAB_TOKEN':'y'}):
            with self.assertRaises(ReleaseError): self.tool.publish(['github','gitlab'],True)
            push.assert_not_called()
            self.assertFalse((self.tool.state/'receipt.json').exists())
    def test_gitlab_link_destinations_conflict(self):
        m=self.build();m['gitlab']='https://gitlab.example.net/team/beam68'
        api=GitlabAPI(m['gitlab']);adapter=rel.GitLab(m,self.tool.out,api);adapter.publish()
        api.r['assets']['links'][0]['url']='https://other.example/file'
        with self.assertRaisesRegex(ReleaseError,'links'): adapter.preflight()
    def test_aminet_existing_incoming_name_refused(self):
        m=self.build(True);FTP.uploads=[];FTP.fail=False;FTP.existing=['Beam68-0.1.lha']
        try:
            with self.assertRaisesRegex(ReleaseError,'already exist'):
                rel.Aminet(m,self.tool.state/'aminet',self.tool.state/'aminet-receipt.json',sha(self.tool.manifest),FTP).preflight()
            self.assertFalse(FTP.uploads)
        finally: FTP.existing=[]
    def test_download_redirect_does_not_forward_token_to_storage(self):
        import urllib.error
        class Opener:
            def __init__(self): self.requests=[]
            def open(self,request,timeout):
                self.requests.append(request)
                if len(self.requests)==1:
                    raise urllib.error.HTTPError(request.full_url,302,'redirect',{'Location':'https://storage.example/signed'},None)
                return io.BytesIO(b'bytes')
        api=rel.API('https://gitlab.example.net','not-a-real-token');opener=Opener();api.opener=opener
        self.assertEqual(api.download_hash('https://gitlab.example.net/package',5),hashlib.sha256(b'bytes').hexdigest())
        self.assertTrue(any(k.lower()=='private-token' for k in opener.requests[0].headers))
        self.assertFalse(any(k.lower()=='private-token' for k in opener.requests[1].headers))
    def test_duplicate_destinations_refused_before_preflight(self):
        with self.assertRaisesRegex(ReleaseError,'duplicate'):
            self.tool.publish(['github','github'],True)
    def test_password_reference_resolved_only_in_memory(self):
        config=copy.deepcopy(self.config)
        config['credentials']={'github':{'provider':'1password','reference':'op://Test/Repository/token'}}
        result=subprocess.CompletedProcess([],0,stdout='private-test-value',stderr='')
        with patch.object(rel.subprocess,'run',return_value=result) as command:
            self.assertEqual(rel.credential('github',config),'private-test-value')
            self.assertEqual(command.call_args.args[0],['op','read','--no-newline','op://Test/Repository/token'])
        self.assertNotIn('private-test-value',json.dumps(config))
    def test_named_password_field_ambiguity_stops_lookup(self):
        config=copy.deepcopy(self.config)
        config['credentials']={'gitlab':{'provider':'1password','vault':'Test','item':'Repo','field':'group/project-rw'}}
        data=[{'label':'group/project-rw','value':'one'},{'label':'group/project-rw','value':'two'}]
        with patch.object(rel.subprocess,'run',return_value=subprocess.CompletedProcess([],0,stdout=json.dumps(data),stderr='')):
            with self.assertRaisesRegex(ReleaseError,'ambiguous'): rel.credential('gitlab',config)
    def test_custom_tag_and_prepared_copies(self):
        self.build()
        tool=FixtureRelease(self.root,self.config,tag='0.1',tag_message='First public release')
        m=tool.prepare(self.root/'docs/RELEASE-NOTES.md')
        self.assertEqual(m['tag'],'0.1');self.assertEqual(m['tag_message'],'First public release')
        stage=tool.prepared_check(m)
        self.assertEqual((stage/'Beam68-0.1.lha').read_bytes(),(tool.out/'Beam68-0.1.lha').read_bytes())
        (stage/'Beam68-0.1.lha').write_bytes(b'changed')
        with self.assertRaisesRegex(ReleaseError,'Prepared asset changed'): tool.prepared_check(m)
        for bad in ('-tag','../tag','0.1.lock','tag..other'):
            with self.assertRaises(ReleaseError): rel.release_tag(bad,'0.1')
    def test_source_dry_run_and_push_do_not_create_release_tags(self):
        git=self.initgit()
        remote=Path(self.tmp.name)/'source.git'
        subprocess.run(['git','init','--bare','-q',str(remote)],check=True)
        with patch.object(rel,'source_destination',return_value=str(remote)),patch.object(rel,'credential',side_effect=AssertionError('source requested API token')):
            rel.push_source(self.config,['github'],dry_run=True,root=self.root)
            self.assertEqual(ORIGINAL_RUN(self.root,['git','ls-remote',str(remote)]),'')
            rel.push_source(self.config,['github'],confirmed=True,root=self.root)
        self.assertEqual(ORIGINAL_RUN(self.root,['git','ls-remote','--tags',str(remote)]),'')
    def test_source_initialization_never_replaces_remote_history(self):
        (self.root/'.gitignore').write_text('/outputs/\n')
        with patch.object(rel,'source_destination',return_value='checked'),patch.object(rel,'git',return_value='existing remote ref'):
            with self.assertRaisesRegex(ReleaseError,'already has history'):
                rel.initialize_source(self.config,['github'],self.root)
        self.assertFalse((self.root/'.git').exists())
    def test_initialization_creates_one_local_commit_without_upload(self):
        (self.root/'.gitignore').write_text('/outputs/\n')
        remote=Path(self.tmp.name)/'empty.git'
        subprocess.run(['git','init','--bare','-q',str(remote)],check=True)
        identity={'GIT_AUTHOR_NAME':'Test','GIT_AUTHOR_EMAIL':'test@example.net','GIT_COMMITTER_NAME':'Test','GIT_COMMITTER_EMAIL':'test@example.net'}
        with patch.object(rel,'source_destination',return_value=str(remote)),patch.dict(os.environ,identity):
            rel.push_source(self.config,['github'],dry_run=True,root=self.root,initialize=True)
            head=rel.commit(self.root,True)
            rel.initialize_source(self.config,['github'],self.root)
            self.assertEqual(rel.commit(self.root,True),head)
        self.assertEqual(ORIGINAL_RUN(self.root,['git','ls-remote',str(remote)]),'')
    def test_cancelled_password_lookup_does_not_disclose_output(self):
        config=copy.deepcopy(self.config);config['credentials']={'github':{'provider':'1password','reference':'op://Test/Repo/token'}}
        with patch.object(rel.subprocess,'run',return_value=subprocess.CompletedProcess([],1,stdout='secret-output',stderr='secret-error')):
            with self.assertRaises(ReleaseError) as error: rel.credential('github',config)
            self.assertNotIn('secret-output',str(error.exception));self.assertNotIn('secret-error',str(error.exception))
    def test_source_push_requires_confirmation(self):
        with self.assertRaisesRegex(ReleaseError,'confirm-push'):
            rel.push_source(self.config,['github'],root=self.root)
    def test_configured_compressed_lha_roundtrip(self):
        tool=os.environ.get('BEAM68_TEST_LHA') or os.environ.get('LHA')
        if not tool: self.skipTest('No optional LHA writer configured')
        from release_support import lha_write,verify_lha
        path=Path(self.tmp.name)/'compressed.lha';entries={'Beam68/LongName':b'A'*8192}
        lha_write(path,entries,tool);verify_lha(path,entries,tool)
        self.assertLess(path.stat().st_size,8192)
    def test_sensitive_tokens_block_build_without_disclosing_values(self):
        secret='gh'+'p_'+'Z'*36
        (self.root/'src/leaked.c').write_text(secret)
        with self.assertRaises(ReleaseError) as error: self.tool.build()
        self.assertIn('GitHub token',str(error.exception));self.assertNotIn(secret,str(error.exception))
        self.assertFalse(self.commands)
    def test_private_key_and_extra_credential_files_refused(self):
        p=self.root/'src/leaked.c';p.write_text('-----BEGIN '+'PRIVATE KEY-----')
        with self.assertRaises(ReleaseError): self.tool.build()
        p.unlink();(self.root/'.npmrc').write_text('local authentication settings')
        with self.assertRaises(ReleaseError): self.tool.inputs()
    def test_source_push_scans_before_any_push(self):
        self.initgit();(self.root/'src/leaked.c').write_text('gl'+'pat-'+'Z'*24)
        with patch.object(rel,'source_destination',return_value='checked'),patch.object(rel,'git') as git:
            with self.assertRaises(ReleaseError): rel.push_source(self.config,['github'],dry_run=True,root=self.root)
            git.assert_not_called()
    def test_standalone_aminet_requires_explicit_contact(self):
        tool=rel.module(self.root,'make-aminet-release')
        with patch.dict(os.environ,{'BEAM68_UPLOADER':''}),contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(tool.main([]),1)
        self.assertFalse((self.root/'dist').exists())
    def test_build_logs_redact_success_failure_and_verbose_output(self):
        self.tool.state.mkdir(parents=True)
        secret='private-'+'logging-test-value'
        code="import os,sys; assert 'GITHUB_TOKEN' not in os.environ; print(os.environ['LOG_TEST_VALUE']); print('stderr detail',file=sys.stderr)"
        for verbose in ('0','1'):
            output=io.StringIO()
            env={'LOG_TEST_VALUE':secret,'BEAM68_RELEASE_VERBOSE':verbose}
            with patch.dict(os.environ,{'GITHUB_TOKEN':secret}),contextlib.redirect_stdout(output):
                ORIGINAL_RUN(self.root,[sys.executable,'-c',code],env)
                with self.assertRaisesRegex(ReleaseError,'build.log'):
                    ORIGINAL_RUN(self.root,[sys.executable,'-c',code+'; sys.exit(1)'],env)
            for name in ('build.log','failed-build.log'):
                text=(self.tool.state/name).read_text()
                self.assertNotIn(secret,text);self.assertIn('[redacted]',text)
                self.assertIn('stderr detail',text)
            self.assertNotIn(secret,output.getvalue())
            if verbose=='1': self.assertIn('[redacted]',output.getvalue())
    def test_build_log_symlinks_refused_before_execution(self):
        self.tool.state.mkdir(parents=True)
        target=Path(self.tmp.name)/'private.log';target.write_text('unchanged')
        (self.tool.state/'build.log').symlink_to(target)
        with patch.object(rel.subprocess,'run') as command:
            with self.assertRaisesRegex(ReleaseError,'Linked build log'):
                ORIGINAL_RUN(self.root,[sys.executable,'-c','print(1)'])
            command.assert_not_called()
        self.assertEqual(target.read_text(),'unchanged')
    def test_legacy_outputs_are_preserved_outside_upload_inventory(self):
        self.build()
        old=self.tool.out/'Beam68-0.1-source.zip';old.write_bytes(b'old archive')
        with patch.object(rel,'run',side_effect=self.command): self.tool.build()
        self.assertFalse(old.exists())
        self.assertTrue(any(p.read_bytes()==b'old archive' for p in (self.tool.state/'superseded').rglob('*.zip')))
        self.assertEqual(set(self.tool.check()),set(self.tool.names))
    def test_aminet_metadata_never_enters_tagged_release_inventory(self):
        m=self.build(True)
        self.assertEqual(len(m['files']),4)
        self.assertFalse(any(n.endswith('.readme') for n in m['files']))
        self.assertEqual(set(m['aminet_files']),{'Beam68-0.1.lha','Beam68-0.1.readme'})
    def test_lha_crc_and_independent_reader_when_available(self):
        self.build()
        self.assertEqual(crc16(b'123456789'),0xbb3d)
        archive=self.tool.out/'Beam68-0.1.lha'
        # libarchive provides an independent LHA implementation on macOS.
        if sys.platform=='darwin':
            listing=subprocess.run(['/usr/bin/tar','-tf',str(archive)],capture_output=True,check=True).stdout.decode()
            self.assertIn('Beam68/Beam68',listing)

ORIGINAL_RUN=rel.run
if __name__=='__main__': unittest.main()
