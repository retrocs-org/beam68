#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "plugin_health.h"
int main(void)
{
    struct BeamPluginHealth h,r; const struct BeamPluginHealthEntry *e; char text[160];
    beam_plugin_health_reset(&h);
    assert(beam_plugin_health_format(NULL,text,sizeof(text)) && strstr(text,"No recorded"));
    beam_plugin_health_record(&h,"PROGDIR:Plugins/Calc/Calc.plugin",BEAM_PLUGIN_HEALTH_SUCCESS);
    beam_plugin_health_record(&h,"progdir:plugins/calc/calc.plugin",BEAM_PLUGIN_HEALTH_ERROR);
    beam_plugin_health_record(&h,"PROGDIR:Plugins/Calc/Calc.plugin",BEAM_PLUGIN_HEALTH_TIMEOUT);
    e=beam_plugin_health_find(&h,"PROGDIR:Plugins/Calc/Calc.plugin");
    assert(e && e->successes==1 && e->errors==1 && e->timeouts==1);
    assert(beam_plugin_health_format(e,text,sizeof(text)) && strstr(text,"Last TIMEOUT"));
    assert(!beam_plugin_health_is_quarantined(&h,"PROGDIR:Plugins/Calc/Calc.plugin"));
    beam_plugin_health_record(&h,"PROGDIR:Plugins/Calc/Calc.plugin",BEAM_PLUGIN_HEALTH_ERROR);
    assert(beam_plugin_health_is_quarantined(&h,"PROGDIR:Plugins/Calc/Calc.plugin"));
    assert(beam_plugin_health_clear_quarantine(&h,"PROGDIR:Plugins/Calc/Calc.plugin"));
    assert(!beam_plugin_health_is_quarantined(&h,"PROGDIR:Plugins/Calc/Calc.plugin"));
    assert(beam_plugin_health_save(&h,BEAM_PLUGIN_HEALTH_PATH));
    assert(beam_plugin_health_load(&r,BEAM_PLUGIN_HEALTH_PATH));
    e=beam_plugin_health_find(&r,"progdir:plugins/calc/calc.plugin");
    assert(e && e->successes==1 && e->errors==2 && e->timeouts==1 && !e->quarantined);
    /* 0.1 flat-layout keys are migrated to package drawers on load. */
    {
        FILE *f=fopen("build/testsettings/ENVARC/Beam68/LegacyHealth.txt","wb");
        static struct BeamPluginHealth legacy;
        assert(f);
        fputs("3\t1\t0\t2\t3\t1\tPROGDIR:Plugins/Clock.plugin\n",f);
        fclose(f);
        assert(beam_plugin_health_load(&legacy,"ENVARC:Beam68/LegacyHealth.txt"));
        assert(beam_plugin_health_is_quarantined(&legacy,"PROGDIR:Plugins/Clock/Clock.plugin"));
    }
    puts("Plugin health counters, quarantine, reset and persistence: PASS");
    return 0;
}
