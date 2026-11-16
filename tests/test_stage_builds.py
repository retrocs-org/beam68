#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import tempfile
import unittest
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
spec=importlib.util.spec_from_file_location('stage_builds',ROOT/'scripts/stage-builds.py')
stage=importlib.util.module_from_spec(spec);spec.loader.exec_module(stage)
class StageBuilds(unittest.TestCase):
    def test_open_drawer_locks_survive_updates(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);source=root/'source';target=root/'target'
            for d in (source,target):(d/'installer').mkdir(parents=True)
            (source/'Install').write_text('new');(source/'installer/Icon.info').write_bytes(b'icon')
            (target/'Install').write_text('old');(target/'guest-metadata.uaem').write_text('keep')
            before=(target.stat().st_ino,(target/'installer').stat().st_ino)
            stage.copy_files(source,target,['Install','installer/Icon.info'])
            self.assertEqual(before,(target.stat().st_ino,(target/'installer').stat().st_ino))
            self.assertEqual((target/'Install').read_text(),'new')
            self.assertEqual((target/'guest-metadata.uaem').read_text(),'keep')
    def test_missing_outer_icon_is_restored_and_custom_icon_preserved(self):
        with tempfile.TemporaryDirectory() as temp:
            destination=Path(temp)
            stage.ensure_drawer_icon(ROOT/'installer/Beam68Drawer.info',destination)
            icon=destination/'Beam68.info'
            self.assertEqual(icon.read_bytes()[48],2)
            icon.write_bytes(b'user customized icon')
            stage.ensure_drawer_icon(ROOT/'installer/Beam68Drawer.info',destination)
            self.assertEqual(icon.read_bytes(),b'user customized icon')
    def test_linked_drawer_is_refused(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);source=root/'source';source.mkdir();(source/'data').write_text('new')
            target=root/'target';target.symlink_to(source,target_is_directory=True)
            with self.assertRaises(ValueError):stage.copy_files(source,target,['data'])
if __name__=='__main__':unittest.main()
