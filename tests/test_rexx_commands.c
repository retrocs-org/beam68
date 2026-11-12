#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "rexx_commands.h"
#include "plugin_diagnostics.h"
static void check(const char *s, int action, int index)
{
    struct BeamRexxCommand c;
    assert(beam_rexx_parse(s,&c));
    assert(c.action==action && c.plugin_index==index);
}
static void bad(const char *s)
{
    struct BeamRexxCommand c;
    assert(!beam_rexx_parse(s,&c));
    assert(!c.action);
}
int main(void)
{
    char b[300], tiny[5];
    check("SHOW",BEAM_REXX_SHOW,-1);
    check("  show   ",BEAM_REXX_SHOW,-1);
    check("HiDe",BEAM_REXX_HIDE,-1);
    check("RELOAD",BEAM_REXX_RELOAD,-1);
    check("reloadplugins",BEAM_REXX_RELOAD_PLUGINS,-1);
    check("reindex",BEAM_REXX_REINDEX,-1);
    check("QUIT",BEAM_REXX_QUIT,-1);
    check("STATUS",BEAM_REXX_STATUS,-1);
    check("Version",BEAM_REXX_VERSION,-1);
    check("Help",BEAM_REXX_HELP,-1);
    check("plugins",BEAM_REXX_PLUGINS,-1);
    check("plugin 1",BEAM_REXX_PLUGIN,0);
    check("PLUGIN\t16 ",BEAM_REXX_PLUGIN,15);
    bad("");bad(" ");bad("SHOW NOW");bad("SHOW;QUIT");bad("REINDEX 1");
    bad("PLUGIN");bad("PLUGIN 0");bad("PLUGIN 17");bad("PLUGIN -1");
    bad("PLUGIN 2x");bad("PLUGIN 99999999999999999");
    bad(" PLUGIN 2 MORE");bad("STATUS\nQUIT");bad("run shell");
    assert(!beam_rexx_parse("SHOW",NULL));
    assert(!beam_rexx_parse(NULL,(struct BeamRexxCommand *)&b));
    assert(!strcmp(beam_plugin_diagnostic(BEAM_PLUGIN_DISABLED),"DISABLED"));
    assert(!strcmp(beam_plugin_diagnostic(BEAM_PLUGIN_CONFLICT),"TRIGGER_CONFLICT"));
    assert(!strcmp(beam_plugin_diagnostic(BEAM_PLUGIN_QUARANTINED),
                   "QUARANTINED_AFTER_FAILURES"));
    assert(beam_plugin_diagnostic_line(2,BEAM_PLUGIN_CONFLICT,'=',"SYS:Tools/X.plugin",b,sizeof(b)));
    assert(!strcmp(b,"2 TRIGGER_CONFLICT trigger== path=SYS:Tools/X.plugin"));
    assert(!beam_plugin_diagnostic_line(2,BEAM_PLUGIN_CONFLICT,'=',"SYS:Tools/X.plugin",tiny,sizeof(tiny)));
    assert(beam_plugin_diagnostic_line(16,BEAM_PLUGIN_DISABLED,0,"a",b,sizeof(b)));
    assert(!strcmp(b,"16 DISABLED path=a"));
    check(" INDEX ",BEAM_REXX_INDEX,-1);
    bad("INDEX foo");
    assert(strstr(beam_plugin_advice(BEAM_PLUGIN_MANIFEST_ERROR),"field lengths"));
    assert(strstr(beam_plugin_advice(BEAM_PLUGIN_QUARANTINED),"dependencies"));
    puts("rexx command and plugin diagnostic tests: PASS");
    return 0;
}
