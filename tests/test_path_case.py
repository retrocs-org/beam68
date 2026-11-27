#!/usr/bin/env python3
import sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from path_case import exact_file,unique_paths,check_runtime,check_headers,PathCaseError
class PathCases(unittest.TestCase):
    def test_exact_components_even_on_insensitive_hosts(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);(root/'Plugins').mkdir();(root/'Plugins'/'Clock').write_text('tool')
            exact_file(root,'Plugins/Clock')
            for name in ['plugins/Clock','Plugins/clock']:
                with self.assertRaisesRegex(PathCaseError,'Case mismatch'):exact_file(root,name)
    def test_case_collisions_include_parent_directories(self):
        for names in [['A/Tool','A/tool'],['Plugins/A','plugins/B']]:
            with self.assertRaisesRegex(PathCaseError,'Case-colliding'):unique_paths(names)
    def test_manifest_reference_must_be_in_package_with_exact_case(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);(root/'Clock').write_text('tool');(root/'Clock.plugin').write_text('COMMAND=PLUGINDIR:clock\n')
            with self.assertRaisesRegex(PathCaseError,'Case mismatch'):check_runtime(root,['Clock','Clock.plugin'])
            (root/'Clock.plugin').write_text('COMMAND=PLUGINDIR:Clock\n')
            with self.assertRaisesRegex(PathCaseError,'absent from package'):check_runtime(root,['Clock.plugin'])
            check_runtime(root,['Clock','Clock.plugin'])
    def test_config_and_installer_references(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);(root/'Beam68.cfg').write_text('PLUGIN=PROGDIR:clock.plugin\n');(root/'Clock.plugin').write_text('')
            with self.assertRaisesRegex(PathCaseError,'Case mismatch'):check_runtime(root,['Beam68.cfg','Clock.plugin'])
            (root/'Install').write_text('(set #src "clock.plugin")')
            with self.assertRaisesRegex(PathCaseError,'Case mismatch'):check_runtime(root,['Install','Clock.plugin'])
    def test_quoted_header_case(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);(root/'include').mkdir();(root/'include'/'Clock.h').write_text('');(root/'test.c').write_text('#include "clock.h"\n')
            with self.assertRaisesRegex(PathCaseError,'capitalized header'):check_headers(root,['test.c'])
            (root/'test.c').write_text('#include "Clock.h"\n');check_headers(root,['test.c'])
    def test_archive_writers_refuse_case_collisions(self):
        from release_support import lha_write,zip_write,ReleaseError
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(ReleaseError,'Case-colliding'):
                lha_write(Path(tmp)/'bad.lha',{'A/Tool':b'a','A/tool':b'b'})
            with self.assertRaisesRegex(ReleaseError,'Case-colliding'):
                zip_write(Path(tmp)/'bad.zip',[("Plugins/A",b'a',False),("plugins/B",b'b',False)])
    def test_links_and_unsafe_names(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);(root/'file').write_text('');(root/'link').symlink_to(root/'file')
            for name in ['../file','/file','link']:
                with self.assertRaises(PathCaseError):exact_file(root,name)
if __name__=='__main__':unittest.main()
