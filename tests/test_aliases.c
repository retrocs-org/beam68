#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aliases.h"

static void write_file(const char *path,const char *text)
{
    FILE *f=fopen(path,"wb");
    assert(f);
    assert(fputs(text,f)>=0);
    assert(fclose(f)==0);
}

int main(void)
{
    static struct BeamAliases a,b;
    const char *shell="SYS:System/Shell";
    if (system("mkdir -p build/testsettings/T/Beam68 build/testsettings/ENV/Beam68 build/testsettings/ENVARC/Beam68 build/testsettings/PROGDIR")!=0) { fputs("test setup command failed\n",stderr); return 1; }
    remove("build/testsettings/ENV/Beam68/Aliases.txt");
    remove("build/testsettings/ENV/Beam68/Aliases.txt.bak");
    remove("build/testsettings/ENVARC/Beam68/Aliases.txt");
    remove("build/testsettings/ENVARC/Beam68/Aliases.txt.bak");
    beam_aliases_clear(&a);
    assert(beam_aliases_set(&a,shell,"terminal cli console"));
    assert(a.count==1);
    assert(beam_aliases_find(&a,"sys:system/shell")==0);
    assert(beam_aliases_set(&a,"sys:system/shell","terminal command line"));
    assert(a.count==1);
    assert(!strcmp(beam_aliases_words(&a,0),"terminal command line"));
    assert(!beam_aliases_set(&a,"unsafe path","no colon"));
    assert(beam_aliases_set_full(&a,shell,"terminal command line","Edit","C:Ed {path}"));
    assert(!strcmp(beam_alias_action_label(&a,0),"Edit"));
    assert(!strcmp(beam_alias_action_command(&a,0),"C:Ed {path}"));
    assert(!beam_aliases_set_full(&a,shell,"words","Missing command",""));
    assert(beam_aliases_save(&a,BEAM_ALIASES_ENVARC));
    assert(beam_aliases_load(&b,BEAM_ALIASES_ENVARC));
    assert(b.count==1 && !strcmp(b.entries[0].words,"terminal command line"));
    assert(!strcmp(b.entries[0].action_label,"Edit"));
    assert(beam_aliases_remove(&b,0) && b.count==0);
    remove("build/testsettings/ENVARC/Beam68/Aliases.txt");
    write_file("build/testsettings/PROGDIR/Aliases.txt",
        "# template\nSYS:System/Shell\tterminal cli\n"
        "invalid line\nSYS:System/Shell\tconsole\tOpen CLI\t{path}\n");
    assert(beam_aliases_load_active(&b));
    assert(b.count==1 && !strcmp(b.entries[0].words,"console"));
    assert(!strcmp(b.entries[0].action_label,"Open CLI"));
    write_file("build/testsettings/ENV/Beam68/Aliases.txt",
        "SYS:Utilities/MultiView\tviewer documents\n");
    assert(beam_aliases_load_active(&b));
    assert(b.count==1 && !strcmp(b.entries[0].path,"SYS:Utilities/MultiView"));
    beam_aliases_clear(&a);
    assert(beam_aliases_set(&a,"RAM:SessionTool","temporary session alias"));
    assert(beam_aliases_save(&a,BEAM_ALIASES_SESSION));
    assert(beam_aliases_load_active(&b));
    assert(b.count==1 && !strcmp(b.entries[0].path,"RAM:SessionTool"));
    puts("Aliases: validation, replacement, session priority, persistence and fallback: PASS");
    return 0;
}
