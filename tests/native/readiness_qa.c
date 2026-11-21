#define main beam68_preferences_production_main
#include "../../src/prefs.c"
#undef main
static int readiness_failed;
static void check_ready(int ok,const char *name)
{printf("%s: %s\n",ok?"PASS":"FAIL",name);if(!ok)readiness_failed++;}
static int check_server(const char *argument)
{
    char command[160];int ticks;
    sprintf(command,"Run >NIL: Builds:ReadinessService %s",argument);
    if(SystemTags((STRPTR)command,NP_StackSize,16384UL,TAG_DONE)!=0)return 0;
    for(ticks=0;ticks<100 && !dependency_port(AMIHASS_PORT);ticks++)Delay(1);
    return dependency_port(AMIHASS_PORT);
}
static int readiness_main(int argc,char **argv)
{
    static struct BeamPlugin p;struct MsgPort *port;char status[240];int ticks;
    (void)argc;(void)argv;
    if(dependency_port(AMIHASS_PORT))return 20;
    strcpy(p.id,"org.beam68.amihass");plugin_readiness(&p,status,sizeof(status));
    check_ready(strstr(status,"Service unavailable")!=NULL,"Absent AmiHASS service reported locally");
    plugin_test_status(&p,status,sizeof(status));check_ready(strstr(status,"Service unavailable")!=NULL,"Missing service test exits without starting it");
    check_ready(check_server("ready"),"Fake read-only STATUS service started");
    plugin_test_status(&p,status,sizeof(status));check_ready(strstr(status,"connected server")!=NULL,"Connection test interprets connected STATUS");
    for(ticks=0;ticks<50 && dependency_port(AMIHASS_PORT);ticks++)Delay(1);
    check_ready(check_server("unconfigured"),"Fake unconfigured STATUS service started");
    plugin_test_status(&p,status,sizeof(status));check_ready(strstr(status,"Not configured")!=NULL,"Connection test distinguishes missing configuration");
    memset(&p,0,sizeof(p));strcpy(p.id,"org.beam68.dopus");p.setting_count=1;
    strcpy(p.setting_defs[0].key,"PORT");strcpy(p.setting_defs[0].default_value,"Beam68ReadinessQA");
    plugin_readiness(&p,status,sizeof(status));check_ready(strstr(status,"Service unavailable")!=NULL,"DOpus missing configured port reported");
    port=CreateMsgPort();if(!port)return 20;port->mp_Node.ln_Name="Beam68ReadinessQA";AddPort(port);
    plugin_readiness(&p,status,sizeof(status));check_ready(strstr(status,"Ready:")!=NULL,"DOpus configured port detected without commands");RemPort(port);DeleteMsgPort(port);
    p.setting_defs[0].default_value[0]=0;plugin_readiness(&p,status,sizeof(status));
    check_ready(strstr(status,"Not configured")!=NULL,"Blank DOpus port distinguished from unavailable service");
    memset(&p,0,sizeof(p));strcpy(p.id,"org.beam68.whdload");p.setting_count=1;
    strcpy(p.setting_defs[0].key,"COMMAND");strcpy(p.setting_defs[0].default_value,"RAM:DefinitelyNoWHDLoad");
    plugin_readiness(&p,status,sizeof(status));check_ready(strstr(status,"Missing WHDLoad")!=NULL,"Missing WHDLoad executable reported");
    memset(&p,0,sizeof(p));strcpy(p.id,"org.beam68.wikipedia");p.setting_count=1;
    strcpy(p.setting_defs[0].key,"CURL");strcpy(p.setting_defs[0].default_value,"RAM:DefinitelyNoCurl");
    plugin_readiness(&p,status,sizeof(status));check_ready(strstr(status,"Missing curl") || strstr(status,"Missing TCP/IP"),"Wikipedia dependencies checked without contacting server");
    puts(readiness_failed?"Plugin readiness QA: FAIL":"Plugin readiness QA: PASS");return readiness_failed?20:0;
}
int main(int argc,char **argv){return beam_run_with_stack(32768UL,readiness_main,argc,argv);}
