"""Release verification rejects changed code, changed binaries and missing files."""
import importlib.util,json,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class Provenance(unittest.TestCase):
    def test_verified_and_stale_release(self):
        spec=importlib.util.spec_from_file_location('beam_provenance',ROOT/'scripts/build-provenance.py')
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'scripts').mkdir();(root/'src').mkdir()
            for name in ('Makefile','Makefile.cross','VERSION','scripts/build-amiga.sh','src/main.c'):(root/name).write_text('0.1')
            binary=root/'Demo';binary.write_bytes(b'\0\0\3\xf3native')
            module.ROOT=root;module.RECORD=root/'BUILD-PROVENANCE.json';module.BINARIES=['Demo']
            record={'version':'0.1','code_sha256':module.code_hash(),'binaries':{'Demo':module.digest(binary)}}
            module.RECORD.write_text(json.dumps(record));self.assertEqual(module.verify(),record)
            binary.write_bytes(b'changed')
            with self.assertRaises(ValueError):module.verify()
            binary.write_bytes(b'\0\0\3\xf3native');(root/'src/main.c').write_text('changed code')
            with self.assertRaises(ValueError):module.verify()
            (root/'src/main.c').write_text('0.1');binary.unlink()
            with self.assertRaises(ValueError):module.verify()
if __name__=='__main__':unittest.main()
