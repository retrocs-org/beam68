#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "plugin_reply.h"
int main(void)
{
    const char *dest="T:Beam68-atomic-test";
    char buf[150];
    FILE *f;
    remove("build/testsettings/T/Beam68-atomic-test");
    remove("build/testsettings/T/Beam68-atomic-test.part");
    assert(!beam_plugin_publish_reply(dest,"TEXT=Incomplete"));
    { char lengthy[134];
      memset(lengthy,'A',sizeof(lengthy));
      memcpy(lengthy,"TEXT=",5);lengthy[132]='\n';lengthy[133]=0;
      assert(!beam_plugin_publish_reply(dest,lengthy));
    }
    assert(!beam_plugin_publish_reply(dest,"TEXT=unsafe\033[31m\n"));
    assert(!beam_plugin_publish_reply(dest,"TEXT=multi\nline\n"));
    assert(!beam_plugin_publish_reply("T:bad*path","TEXT=ok\n"));
    assert(beam_plugin_publish_reply(dest,"TEXT=Ready\n"));
    f=fopen("build/testsettings/T/Beam68-atomic-test","rb");
    assert(f);assert(fgets(buf,sizeof(buf),f));assert(!strcmp(buf,"TEXT=Ready\n"));
    assert(fgetc(f)==EOF);fclose(f);
    assert(!beam_plugin_publish_reply(dest,"TEXT=Overwrite\n"));
    f=fopen("build/testsettings/T/Beam68-atomic-test.part","rb");
    assert(!f);
    remove("build/testsettings/T/Beam68-atomic-test");
    puts("Atomic plugin result publication, bounds and cleanup: PASS");
    return 0;
}
