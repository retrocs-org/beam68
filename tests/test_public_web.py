#!/usr/bin/env python3
"""Render PHP pages and verify navigation, local assets and screenshot metadata."""
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urlsplit
import json,struct,subprocess,os,shutil
ROOT=Path(__file__).resolve().parents[1];WEB=ROOT/'public-web'
PHP=os.environ.get('BEAM68_PHP') or shutil.which('php')
assert PHP,'Install PHP CLI or set BEAM68_PHP to run the website checks'
for path in WEB.rglob('*.php'):
    subprocess.run([PHP,'-n','-l',str(path)],check=True,capture_output=True)
class SiteParser(HTMLParser):
    def __init__(self):
        super().__init__();self.links=[];self.assets=[];self.anchors=set();self.tags=set();self.images=[];self.canonical=[];self.current=[]
    def handle_starttag(self,tag,attrs):
        self.tags.add(tag);a=dict(attrs)
        assert not any(k.lower().startswith('on') for k in a),'Unexpected inline event handler'
        if 'id' in a:self.anchors.add(a['id'])
        if tag=='a' and 'href' in a:self.links.append(a['href'])
        if a.get('aria-current')=='page':self.current.append(a.get('href'))
        if tag=='img':self.images.append(a)
        if tag=='link':
            if a.get('rel')=='canonical':self.canonical.append(a.get('href'))
            else:
                assert a.get('rel')=='stylesheet' and a.get('href')=='site.css'
                self.assets.append(a['href'])
pages={};rendered={}
for name in ('index.php','screenshots.php','download.php','developers.php','source.php','marketplace.php'):
    result=subprocess.run([PHP,'-n','-d','display_errors=stderr',str(WEB/name)],check=True,capture_output=True)
    assert not result.stderr,result.stderr.decode()
    raw=result.stdout;assert raw.lower().startswith(b'<!doctype html>') and len(raw)<32000,name
    assert b'<?php' not in raw and b'javascript:' not in raw.lower(),name
    assert b'width=device-width' in raw,name
    parser=SiteParser();parser.feed(raw.decode('utf-8'));pages[name]=parser;rendered[name]=raw
    assert not ({'script','style','iframe','canvas','video','object','embed','table','font'}&parser.tags),name
    assert {'header','main','footer','nav'}<=parser.tags,name
    expected='https://beam68.retrocs.org/'+('' if name=='index.php' else name)
    assert parser.canonical==[expected] and parser.current==[name],name
    assert b'https://github.com/retrocs-org/beam68' in raw,name
    assert b'BitSkim' not in raw and b'bitskim.' not in raw,name
for name,parser in pages.items():
    for href in parser.links+parser.assets:
        url=urlsplit(href)
        if url.scheme or url.netloc:
            assert url.scheme=='https' and url.netloc in ('beam68.retrocs.org','github.com'),href
            if url.netloc=='github.com':assert url.path == '/retrocs-org/beam68' or url.path.startswith('/retrocs-org/beam68/')
            for prefix in ('/retrocs-org/beam68/blob/main/','/retrocs-org/beam68/tree/main/'):
                if url.netloc=='github.com' and url.path.startswith(prefix):
                    relative=url.path[len(prefix):]
                    exact={p.relative_to(ROOT).as_posix() for p in ROOT.rglob('*')}
                    assert relative in exact, 'Missing or incorrectly capitalized repository link: '+href
            continue
        target=url.path or name
        assert (WEB/target).is_file(),href
        if url.fragment:assert url.fragment in pages[target].anchors,href
    for img in parser.images:
        src=img.get('src','');assert src and not urlsplit(src).scheme,src
        data=(WEB/src).read_bytes()
        assert img.get('alt') and img.get('width') and img.get('height'),src
        assert struct.unpack('>II',data[16:24])==(int(img['width']),int(img['height'])),src
        assert len(data)<64000,src
for shot in json.loads((WEB/'images/screenshots.json').read_text()):
    data=(WEB/'images'/shot['file']).read_bytes()
    assert data[:8]==b'\x89PNG\r\n\x1a\n'
    assert struct.unpack('>II',data[16:24])==(shot['width'],shot['height'])
    assert shot['provenance']
    if shot.get('build_sha256'):
        record=json.loads((WEB/'images/capture-build.json').read_text())
        assert record['binaries'][shot['app']]==shot['build_sha256']
# Captions and other inventory text are escaped at the shared output boundary.
include=json.dumps(str(WEB/'includes/site.php'))
probe="define('BEAM68_PAGE',true); require "+include+"; echo escape('<tag>\"&');"
assert subprocess.check_output([PHP,'-n','-r',probe])==b'&lt;tag&gt;&quot;&amp;'
assert subprocess.check_output([PHP,'-n',str(WEB/'includes/site.php')])==b''
css=(WEB/'site.css').read_text()
assert '@media' in css and 'prefers-reduced-motion' in css and ':focus-visible' in css
assert '@import' not in css and 'url(' not in css
assert len(css)<24000
assert not list(WEB.rglob('*.zip')) and not list(WEB.glob('*.html'))
assert not (WEB/'downloads').exists()
assert b'Download' in rendered['index.php']
assert b'Awaiting publication' in rendered['download.php']
assert b'/releases/download/v0.1/Beam68-0.1.lha' in rendered['download.php']
assert b'/releases/download/v0.1/Beam68-0.1-source.zip' in rendered['download.php']
assert b'/blob/main/sdk/README.md' in rendered['developers.php']
assert b'Release status' not in rendered['index.php']
catalog=json.loads((WEB/'includes/marketplace.json').read_text())
assert len(catalog['plugins'])==13 and len(catalog['themes'])==4
for item in catalog['plugins']+catalog['themes']:
    assert (ROOT/item['path']).exists()
assert (ROOT/'VERSION').read_text('ascii').strip()=='0.1'
print('PASS: PHP syntax/rendering, escaping, navigation, local CSS and screenshot metadata')
