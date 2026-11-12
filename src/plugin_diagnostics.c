#include "plugin_diagnostics.h"
#include <string.h>
const char *beam_plugin_diagnostic(int status)
{
    switch (status) {
    case BEAM_PLUGIN_OK: return "ACTIVE";
    case BEAM_PLUGIN_DISABLED: return "DISABLED";
    case BEAM_PLUGIN_MANIFEST_ERROR: return "MANIFEST_INVALID_OR_MISSING";
    case BEAM_PLUGIN_EXECUTABLE_MISSING: return "EXECUTABLE_MISSING";
    case BEAM_PLUGIN_PATH_ERROR: return "EXECUTABLE_PATH_INVALID";
    case BEAM_PLUGIN_CONFLICT: return "TRIGGER_CONFLICT";
    case BEAM_PLUGIN_QUARANTINED: return "QUARANTINED_AFTER_FAILURES";
    default: return "UNKNOWN";
    }
}
const char *beam_plugin_advice(int status)
{
    switch (status) {
    case BEAM_PLUGIN_OK: return "Ready. Optional host apps may still be needed by individual commands.";
    case BEAM_PLUGIN_DISABLED: return "Select Enable/Disable to enable this trusted plugin.";
    case BEAM_PLUGIN_MANIFEST_ERROR: return "Check the manifest exists, field lengths, VERSION and MIN_BEAM68; reinstall a valid package.";
    case BEAM_PLUGIN_EXECUTABLE_MISSING: return "Restore the executable named by COMMAND, or reinstall this plugin.";
    case BEAM_PLUGIN_PATH_ERROR: return "Check INSTALL_DIR and the manifest COMMAND path; use PLUGINDIR: for bundled code.";
    case BEAM_PLUGIN_CONFLICT: return "Disable the other plugin using the same trigger or prefix, then reload plugins.";
    case BEAM_PLUGIN_QUARANTINED: return "Inspect failure history and dependencies before clearing quarantine with Enable/Disable.";
    default: return "Select Details and inspect the plugin manifest and executable.";
    }
}
/* No snprintf dependency in target build. Copy segments with explicit bounds. */
static int append(char *dst, unsigned int cap, unsigned int *length,const char *src)
{
    while (*src) {
        if (*length+1>=cap) return 0;
        dst[(*length)++]=*src++;
    }
    dst[*length]=0;
    return 1;
}
int beam_plugin_diagnostic_line(int index,int state,char trigger,
    const char *manifest,char *out,unsigned int cap)
{
    char number[4]; unsigned int n=0;
    if (!out || !cap || index<1 || index>16 || !manifest) return 0;
    out[0]=0;
    number[0]=(char)('0'+index/10);
    number[1]=(char)('0'+index%10);
    number[2]=0;
    if (index<10) { number[0]=number[1]; number[1]=0; }
    if (!append(out,cap,&n,number) || !append(out,cap,&n," ") ||
        !append(out,cap,&n,beam_plugin_diagnostic(state))) return 0;
    if (state==BEAM_PLUGIN_OK || state==BEAM_PLUGIN_CONFLICT) {
        char trig[2]; trig[0]=trigger ? trigger : '?';trig[1]=0;
        if (!append(out,cap,&n," trigger=") || !append(out,cap,&n,trig)) return 0;
    }
    return append(out,cap,&n," path=") && append(out,cap,&n,manifest);
}
