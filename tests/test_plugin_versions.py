#!/usr/bin/env python3
"""Version guards use temporary plugin sources; never change the real baseline."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('plugin_versions',ROOT/'scripts/plugin-versions.py')
versions=importlib.util.module_from_spec(spec);spec.loader.exec_module(versions)

class PluginVersions(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
        for name in ('One','Two'):
            folder=self.root/'Plugins'/name;folder.mkdir(parents=True)
            (folder/(name+'.plugin')).write_text('NAME='+name+'\nPLUGIN_VERSION=0.1\n')
            (folder/(name+'.c')).write_text('#include "shared.h"\nint main(void){return 0;}\n')
        (self.root/'include').mkdir()
        (self.root/'include/shared.h').write_text('#define VALUE 1\n')
        (self.root/'Makefile').write_text(''.join(
            'Plugins/'+n+'/'+n+':\n\t$(CC) -o $@ Plugins/'+n+'/'+n+'.c Plugins/'+n+'/version.c\n'
            for n in ('One','Two')))
        self.record=self.root/'versions.json'
        versions.check(self.root,self.record,True)
    def tearDown(self):self.temp.cleanup()
    def test_changed_plugin_requires_bump_even_when_recording(self):
        p=self.root/'Plugins/One/One.c';p.write_text(p.read_text()+'/* change */\n')
        for accept in (False,True):
            with self.assertRaisesRegex(ValueError,'increase PLUGIN_VERSION'):
                versions.check(self.root,self.record,accept)
        p=self.root/'Plugins/One/One.plugin';p.write_text(p.read_text().replace('0.1','0.2'))
        result=versions.check(self.root,self.record,True)
        self.assertEqual(result['One']['version'],'0.2');self.assertEqual(result['Two']['version'],'0.1')
        versions.check(self.root,self.record)
    def test_shared_dependency_requires_affected_plugins_bump(self):
        (self.root/'include/shared.h').write_text('#define VALUE 2\n')
        with self.assertRaisesRegex(ValueError,'increase PLUGIN_VERSION'):
            versions.check(self.root,self.record)
    def test_generated_native_tag_cannot_drift(self):
        (self.root/'Plugins/One/version.c').write_text('wrong version')
        with self.assertRaisesRegex(ValueError,'native version differs'):
            versions.check(self.root,self.record)
    def test_app_version_not_coupled(self):
        p=self.root/'include/beam.h';p.write_text('#define BEAM68_VERSION "0.2"\n#define BEAM68_VERSION_DATE "today"\n')
        (self.root/'include/shared.h').write_text('#include "beam.h"\n')
        # Establish a separate initial record for this dependency layout.
        self.record.unlink();versions.check(self.root,self.record,True)
        p.write_text('#define BEAM68_VERSION "0.3"\n#define BEAM68_VERSION_DATE "tomorrow"\n')
        versions.check(self.root,self.record)
    def test_numeric_comparison(self):
        self.assertTrue(versions.version_greater('0.10','0.9'))
        self.assertFalse(versions.version_greater('0.1.0','0.1'))

if __name__=='__main__':unittest.main()
