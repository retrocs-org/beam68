"""Offline tests for PHP GitHub release discovery; no network requests."""
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
PHP=os.environ.get('BEAM68_PHP') or shutil.which('php')
class Releases(unittest.TestCase):
    def run_php(self,code):
        self.assertTrue(PHP,'PHP CLI required')
        with tempfile.TemporaryDirectory() as folder:
            include=str(ROOT/'public-web/includes/github-releases.php').replace("'","\\'")
            prefix="define('BEAM68_PAGE',true);require '"+include+"';"
            prefix+="$r=json_decode(file_get_contents($argv[1]),true);$d=$argv[2].'/cache';$t=2000000000;$calls=0;"
            prefix+="function expect($v){if(!$v)throw new RuntimeException('Release check failed');}"
            prefix+="$good=function($etag)use(&$calls,$r){$calls++;return ['status'=>200,'body'=>json_encode($r),'etag'=>'\"one\"'];};"
            result=subprocess.run([PHP,'-n','-d','display_errors=stderr','-r',prefix+code,
                str(ROOT/'tests/fixtures/github-release.json'),folder],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual(result.stderr,'')
    def test_fresh_cache_avoids_repeated_api_requests(self):
        self.run_php("$a=latest_release($good,$d,$t);$b=latest_release($good,$d,$t+1);expect($calls===1&&$a===$b&&$a['release']['tag']==='0.1');")
    def test_changed_release_is_discovered_after_ttl(self):
        self.run_php("latest_release($good,$d,$t);$new=function($etag)use($r){$raw=json_decode(str_replace('0.1','0.2',json_encode($r)),true);return ['status'=>200,'body'=>json_encode($raw),'etag'=>'new'];};$a=latest_release($new,$d,$t+601);expect($a['release']['tag']==='0.2');expect(str_contains($a['release']['runtime']['url'],'Beam68-0.2.lha'));")
    def test_conditional_request_reuses_valid_payload(self):
        self.run_php("latest_release($good,$d,$t);$notModified=function($etag){expect($etag==='\"one\"');return ['status'=>304,'body'=>'','etag'=>''];};$a=latest_release($notModified,$d,$t+601);expect($a['state']==='fresh'&&$a['release']['sdk']['name']==='Beam68-0.1-plugin-sdk.zip');")
    def test_outage_uses_bounded_stale_cache_and_backoff(self):
        self.run_php("latest_release($good,$d,$t);$bad=function($etag)use(&$calls){$calls++;return ['status'=>503,'body'=>'','etag'=>''];};$a=latest_release($bad,$d,$t+601);$b=latest_release($bad,$d,$t+602);expect($a['state']==='stale'&&$b['state']==='stale'&&$calls===2);$c=latest_release($bad,$d,$t+90000);expect($c['release']===null&&$c['state']==='unavailable');")
    def test_cold_failure_is_cached_briefly(self):
        self.run_php("$bad=function($etag)use(&$calls){$calls++;return ['status'=>403,'body'=>'rate limit','etag'=>''];};$a=latest_release($bad,$d,$t);$b=latest_release($bad,$d,$t+1);expect($a['state']==='unavailable'&&$b['state']==='unavailable'&&$calls===1);")
    def test_revalidation_clears_outage_status(self):
        self.run_php("latest_release($good,$d,$t);$bad=fn($etag)=>['status'=>500,'body'=>'','etag'=>''];latest_release($bad,$d,$t+601);$revalidated=fn($etag)=>['status'=>304,'body'=>'','etag'=>''];$a=latest_release($revalidated,$d,$t+662);$b=latest_release($bad,$d,$t+663);expect($a['state']==='fresh'&&$b['state']==='fresh');")
    def test_no_public_release_has_a_distinct_empty_state(self):
        self.run_php("$missing=function($etag)use(&$calls){$calls++;return ['status'=>404,'body'=>'','etag'=>''];};$a=latest_release($missing,$d,$t);$b=latest_release($missing,$d,$t+1);expect($a['state']==='missing'&&$a['release']===null&&$calls===1);")
    def test_drafts_prereleases_and_unsafe_tags_are_rejected(self):
        self.run_php("foreach(['draft','prerelease']as$key){$raw=$r;$raw[$key]=true;expect(release_normalize($raw)===null);}$raw=$r;$raw['tag_name']='<script>';expect(release_normalize($raw)===null);")
    def test_assets_cannot_link_outside_the_exact_release(self):
        self.run_php("$raw=$r;$raw['assets'][0]['browser_download_url']='https://other.example/Beam68.lha';$a=release_normalize($raw);expect($a['runtime']===null&&$a['sdk']!==null);$raw=$r;$raw['assets'][]=$raw['assets'][0];expect(release_normalize($raw)===null);")
    def test_missing_checksum_is_not_invented(self):
        self.run_php("$r['assets']=array_values(array_filter($r['assets'],fn($a)=>!str_ends_with($a['name'],'.sha256')));$a=release_normalize($r);expect($a['runtime']!==null&&$a['runtime']['checksum']===null);")
    def test_refresh_lock_prevents_parallel_api_fetches(self):
        self.run_php("latest_release($good,$d,$t);$handle=fopen($d.'/refresh.lock','c');flock($handle,LOCK_EX);$a=latest_release($good,$d,$t+601);expect($a['state']==='stale'&&$calls===1);flock($handle,LOCK_UN);fclose($handle);")
if __name__=='__main__':unittest.main()
