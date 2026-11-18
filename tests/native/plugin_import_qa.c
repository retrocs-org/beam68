/* Native file-operation regression; all writes use a fresh T: test tree. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "beam.h"
#include "plugin_install.h"
#include "prefs_locale.h"
#include "stack_guard.h"
static struct BeamConfig cfg;
static char source[BEAM_MAX_PATH],manifest[BEAM_MAX_PATH],root[80],message[256];
static int mode,calls,saw_files;
const char *pstr(int id){return id>0 && id<=BEAM68PREFS_STRING_COUNT?beam68prefs_strings[id-1]:"";}
static int dir(const char *p){BPTR l=CreateDir((STRPTR)p);if(!l)return 0;UnLock(l);return 1;}
static int exists(const char *p){BPTR l=Lock((STRPTR)p,ACCESS_READ);if(l)UnLock(l);return l!=0;}
static int write_manifest(const char *version){FILE*f=fopen(manifest,"w");int ok;if(!f)return 0;ok=fprintf(f,"VERSION=1\nID=org.example.review\nPLUGIN_VERSION=%s\nMIN_BEAM68=0.1\nNAME=Review demo\nTRIGGER=~\nPREFIX=REVIEW:\nCOMMAND=PLUGINDIR:ReviewDemo\nCAPABILITIES=FILES\n",version)>0;return fclose(f)==0&&ok;}
int beam_plugin_install_qa_ask(const char *title,const char *text,const char *buttons)
{
    (void)title;(void)buttons;calls++;
    if(strstr(text,"Files to install:") && strstr(text,"ReviewDemo.plugin"))saw_files=1;
    if(mode==0 || (mode==1 && calls==2))return 0;
    if(mode==3 && calls==2)write_manifest("0.9");
    return 1;
}
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s; %s\n",__LINE__,#x,message);return 20;}}while(0)
static int run_qa(int argc,char **argv)
{
    char path[BEAM_MAX_PATH],target[BEAM_MAX_PATH];int i;BPTR l;FILE*f;
    static const unsigned char hunk[]={0,0,3,0xf3};
    (void)argc;(void)argv;
    for(i=0;i<100;i++){sprintf(root,"T:Beam68ImportQA%02d",i);l=CreateDir(root);if(l){UnLock(l);break;}}
    CHECK(i<100);sprintf(path,"%s/source",root);CHECK(dir(path));
    sprintf(source,"%s/source/ReviewDemo",root);CHECK(dir(source));
    sprintf(manifest,"%.160s/ReviewDemo.plugin",source);CHECK(write_manifest("0.1"));
    sprintf(path,"%.160s/ReviewDemo",source);f=fopen(path,"wb");CHECK(f);CHECK(fwrite(hunk,1,sizeof(hunk),f)==sizeof(hunk));CHECK(!fclose(f));
    beam_config_defaults(&cfg);cfg.plugin_file_count=cfg.disabled_plugin_count=0;
    sprintf(cfg.install_dir,"%s/target",root);CHECK(dir(cfg.install_dir));
    sprintf(target,"%.160s/Plugins/ReviewDemo",cfg.install_dir);
    mode=0;calls=0;CHECK(!beam_plugin_install(NULL,&cfg,source,message,sizeof(message)));CHECK(!cfg.plugin_file_count&&!exists(target));
    mode=1;calls=0;CHECK(!beam_plugin_install(NULL,&cfg,source,message,sizeof(message)));CHECK(calls==2&&saw_files&&!cfg.plugin_file_count&&!exists(target));
    puts("PASS: cancelling either review stage changes neither files nor registration");
    mode=2;calls=0;CHECK(beam_plugin_install(NULL,&cfg,source,message,sizeof(message)));CHECK(cfg.plugin_file_count==1&&!beam_plugin_enabled(&cfg,cfg.plugin_files[0]));
    CHECK(!beam_plugin_review_reference(NULL,&cfg,manifest,message,sizeof(message)));
    puts("PASS: new imports start disabled and duplicate references are rejected");
    CHECK(write_manifest("0.2"));calls=0;CHECK(beam_plugin_install(NULL,&cfg,source,message,sizeof(message)));CHECK(!beam_plugin_enabled(&cfg,cfg.plugin_files[0]));
    CHECK(beam_plugin_set_enabled(&cfg,cfg.plugin_files[0],1));CHECK(write_manifest("0.3"));calls=0;CHECK(beam_plugin_install(NULL,&cfg,source,message,sizeof(message)));CHECK(beam_plugin_enabled(&cfg,cfg.plugin_files[0]));
    puts("PASS: upgrades retain both disabled and enabled activation choices");
    mode=3;calls=0;CHECK(!beam_plugin_install(NULL,&cfg,source,message,sizeof(message)));
    sprintf(path,"%.160s/ReviewDemo.plugin",target);{static struct BeamPlugin p;CHECK(beam_plugin_read(&p,path));CHECK(!strcmp(p.plugin_version,"0.3"));}
    puts("PASS: manifest changes during review leave the original package intact");
    puts("Plugin import native review and replacement: PASS");return 0;
}
int main(int argc,char **argv){return beam_run_with_stack(65536UL,run_qa,argc,argv);}
