"""Exercise the production native ZIP directory reader against real archives."""
import ctypes as C
from pathlib import Path
import struct,subprocess,tempfile,unittest,zipfile
ROOT=Path(__file__).resolve().parents[1]
class ArchiveSafety(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory();root=Path(cls.temp.name)
        source=(ROOT/'src/plugin_install.c').read_text()
        reader=source[source.index('static unsigned long zip16'):source.index('/* A scratch directory')]
        harness=r'''
#include <stdio.h>
#include <string.h>
#include <exec/types.h>
typedef unsigned char UBYTE;
#include <dos/dos.h>
#include "beam.h"
#include "plugin_package.h"
#define OFFSET_BEGINNING -1
#define OFFSET_CURRENT 0
#define OFFSET_END 1
static BPTR zip_open(STRPTR path,LONG mode){(void)mode;return fopen(path,"rb");}
static LONG zip_close(BPTR f){return fclose(f)==0?-1:0;}
static LONG zip_read(BPTR f,void *data,LONG n){return (LONG)fread(data,1,(size_t)n,f);}
static LONG zip_seek(BPTR f,LONG pos,LONG mode){long old=ftell(f);if(fseek(f,pos,mode==1?SEEK_END:mode==0?SEEK_CUR:SEEK_SET))return -1;return old;}
#define Open zip_open
#define Close zip_close
#define Read zip_read
#define Seek zip_seek
'''+reader+'\nint check_zip(const char *path){return zip_list_safe(path);}\n'
        (root/'reader.c').write_text(harness)
        subprocess.run(['cc','-shared','-fPIC','-std=c99','-Wall','-Wextra','-Werror','-I'+str(ROOT/'tests/fake_ndk'),'-I'+str(ROOT/'include'),str(root/'reader.c'),str(ROOT/'src/plugin_package.c'),str(ROOT/'src/config.c'),str(ROOT/'src/appearance.c'),str(ROOT/'tests/fake_dos.c'),'-o',str(root/'reader.so')],check=True)
        cls.lib=C.CDLL(str(root/'reader.so'));cls.lib.check_zip.argtypes=[C.c_char_p]
    @classmethod
    def tearDownClass(cls):cls.temp.cleanup()
    def check(self,entries,comment=b'',change=None):
        path=Path(self.temp.name)/'input.zip'
        with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED) as archive:
            for name,data in entries:archive.writestr(name,data)
            archive.comment=comment
        if change:
            data=bytearray(path.read_bytes());change(data);path.write_bytes(data)
        return self.lib.check_zip(bytes(path))
    def test_normal_plugin_and_comment(self):
        self.assertEqual(self.check([('Weather/Weather.plugin',b'ID=weather'),('Weather/Weather',b'\0\0\3\xf3')],b'Plugin release'),1)
    def test_traversal_and_control_names(self):
        for name in ('../startup','SYS:S/Startup-Sequence','A//file','A/new\nfile'):
            self.assertEqual(self.check([(name,b'x')]),0,name)
    def test_symlink_is_rejected(self):
        info=zipfile.ZipInfo('Weather/link');info.create_system=3;info.external_attr=0o120777<<16
        self.assertEqual(self.check([(info,b'../../outside')]),0)
    def test_mismatched_local_name_is_rejected(self):
        self.assertEqual(self.check([('Weather/file',b'x')],change=lambda data:data.__setitem__(30,ord('X'))),0)
    def test_truncated_archive_is_rejected(self):
        self.assertEqual(self.check([('Weather/file',b'x')],change=lambda data:data.__delitem__(slice(-8,None))),0)
    def test_size_limit_is_rejected(self):
        def enlarge(data):
            offset=data.index(b'PK\1\2');struct.pack_into('<I',data,offset+24,16*1024*1024+1)
        self.assertEqual(self.check([('Weather/file',b'x')],change=enlarge),0)
if __name__=='__main__':unittest.main()
