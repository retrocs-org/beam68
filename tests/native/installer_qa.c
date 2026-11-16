/* Interactive installer file-operation review. Never changes live assigns. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>
#include "stack_guard.h"
struct IntuitionBase *IntuitionBase;
#define ROOT "RAM:Beam68InstallerQA"
#define APP ROOT "/System/Utilities/Beam68"
#define CFG ROOT "/Saved/Beam68/Beam68.cfg"
#define REPORT ROOT "/report.txt"
static char summary[240]="No checks yet. Choose Install / update to begin.";
static char config_text[65536];
static BPTR report;
static int failures,checks;
static const char *protected_files[]={CFG,ROOT "/Environment/Beam68/Beam68.cfg",
    ROOT "/Session/Beam68.cfg",ROOT "/Session/Beam68.cfg.bak",ROOT "/Session/Beam68.cfg.new",
    ROOT "/Saved/Beam68/Aliases.txt",ROOT "/Saved/Beam68/Snippets.txt",
    APP "/Aliases.txt",APP "/Themes/Amber.theme",APP "/Themes/GreenScreen.theme",
    APP "/Themes/MagicGrey.theme",APP "/Themes/Paper.theme",APP "/Plugins/QAExtra/keep.txt"};
#define PROTECTED_COUNT (sizeof(protected_files)/sizeof(protected_files[0]))
static ULONG before[PROTECTED_COUNT];
static unsigned char had_file[PROTECTED_COUNT];
static int file_exists(const char *path)
{
    BPTR lock=Lock((STRPTR)path,ACCESS_READ);struct FileInfoBlock *fib;int ok=0;
    if(!lock)return 0;
    fib=AllocDosObject(DOS_FIB,NULL);
    if(fib){ok=Examine(lock,fib) && fib->fib_DirEntryType<0;FreeDosObject(DOS_FIB,fib);}
    UnLock(lock);return ok;
}
static int directory(const char *path)
{
    BPTR lock=Lock((STRPTR)path,ACCESS_READ);
    if(lock){UnLock(lock);return 1;}
    lock=CreateDir((STRPTR)path);if(!lock)return 0;UnLock(lock);return 1;
}
static int prepare(void)
{
    static const char *paths[]={ROOT,ROOT "/System",ROOT "/System/Utilities",
        ROOT "/System/Prefs",ROOT "/System/WBStartup",ROOT "/Saved",
        ROOT "/Environment",ROOT "/Session",ROOT "/Locale",ROOT "/Locale/Catalogs"};
    unsigned int i;for(i=0;i<sizeof(paths)/sizeof(paths[0]);i++)if(!directory(paths[i]))return 0;
    return 1;
}
static void check(int ok,const char *label)
{
    char line[600];checks++;if(!ok)failures++;
    sprintf(line,"%s: %s\n",ok?"PASS":"FAIL",label);
    if(report)FPuts(report,line);
}
static int hash_file(const char *path,ULONG *hash)
{
    unsigned char buf[512];LONG n,i;BPTR f=Open((STRPTR)path,MODE_OLDFILE);
    ULONG value=2166136261UL;
    if(!f)return 0;
    while((n=Read(f,buf,sizeof(buf)))>0)for(i=0;i<n;i++)value=(value^buf[i])*16777619UL;
    Close(f);if(n<0)return 0;*hash=value;return 1;
}
static void seed_session(void)
{
    static const char *files[]={ROOT "/Session/Beam68.cfg",ROOT "/Session/Beam68.cfg.bak",ROOT "/Session/Beam68.cfg.new"};
    unsigned int i;BPTR f;
    for(i=0;i<sizeof(files)/sizeof(files[0]);i++){
        f=Open((STRPTR)files[i],MODE_NEWFILE);
        if(f){FPuts(f,"QA temporary settings: must survive update, disappear on Remove settings.\n");Close(f);}
    }
}
static void snapshot(void)
{
    unsigned int i;BPTR f;
    directory(APP "/Plugins/QAExtra");
    if(!file_exists(APP "/Plugins/QAExtra/keep.txt")){
        f=Open((STRPTR)APP "/Plugins/QAExtra/keep.txt",MODE_NEWFILE);
        if(f){FPuts(f,"Third-party QA data: preserve during update.\n");Close(f);}
    }
    seed_session();
    for(i=0;i<PROTECTED_COUNT;i++)had_file[i]=(unsigned char)hash_file(protected_files[i],&before[i]);
}
static int has_line(const char *line)
{
    const char *p=config_text;unsigned int len=(unsigned int)strlen(line);
    while(*p){if(!strncmp(p,line,len) && (p[len]=='\r'||p[len]=='\n'||!p[len]))return 1;
        p=strchr(p,'\n');if(!p)break;p++;}
    return 0;
}
static void inspect(int preserve,int removed)
{
    BPTR f;LONG n;char *p;unsigned int count=0,i;ULONG hash;
    report=Open((STRPTR)REPORT,MODE_NEWFILE);failures=checks=0;
    if(!report){failures=1;strcpy(summary,"Cannot write the QA report.");return;}
    FPuts(report,"Beam68 Installer QA 0.2\nTest root: " ROOT "\n\n");
    if(removed){
        check(!file_exists(APP "/Beam68"),"Launcher removed from QA destination");
        check(!file_exists(ROOT "/System/Prefs/Beam68Prefs"),"Preferences removed from QA destination");
        check(!file_exists(ROOT "/System/WBStartup/Beam68Start"),"QA startup entry removed");
        check(!file_exists(ROOT "/System/WBStartup/Beam68Start.info"),"QA startup icon removed");
        if(!file_exists(CFG)){
            check(!file_exists(ROOT "/Session/Beam68.cfg"),"Temporary Use settings removed");
            check(!file_exists(ROOT "/Session/Beam68.cfg.bak"),"Temporary backup removed");
            check(!file_exists(ROOT "/Session/Beam68.cfg.new"),"Temporary pending save removed");
        }
    }else{
        static const char *required[]={APP "/Beam68",APP "/Beam68.info",APP "/Beam68Ctl",
            ROOT "/System/Prefs/Beam68Prefs",ROOT "/System/Prefs/Beam68Prefs.info",
            APP "/Beam68.guide",APP "/Aliases.txt",CFG,
            ROOT "/Environment/Beam68/Beam68.cfg",ROOT "/Saved/Beam68/InstallDir"};
        for(i=0;i<sizeof(required)/sizeof(required[0]);i++)check(file_exists(required[i]),required[i]);
        f=Open((STRPTR)CFG,MODE_OLDFILE);config_text[0]=0;
        if(f){n=Read(f,config_text,sizeof(config_text)-1);Close(f);if(n>=0)config_text[n]=0;check(n>=0 && n<(LONG)sizeof(config_text)-1,"Settings readable and within checker limit");}
        p=config_text;
        while(*p){
            if(!strncmp(p,"PLUGIN=PROGDIR:Plugins/",23)){
                char path[512],relative[256],*end=strchr(p,'\n');unsigned int len=end?(unsigned int)(end-p):(unsigned int)strlen(p);
                if(len && p[len-1]=='\r')len--;
                if(len>15 && len-15<sizeof(relative)){
                    memcpy(relative,p+15,len-15);relative[len-15]=0;
                    if(!strstr(relative,"..") && !strchr(relative,':') && !strchr(relative,'"') && !strchr(relative,'*')){
                        sprintf(path,APP "/%s",relative);check(file_exists(path),path);
                        if(strlen(path)>7 && !strcmp(path+strlen(path)-7,".plugin")){
                            path[strlen(path)-7]=0;check(file_exists(path),path);
                        }
                    }else check(0,"Plugin path stays inside QA destination");
                }else check(0,"Plugin path within checker limit");
                count++;
            }
            p=strchr(p,'\n');if(!p)break;p++;
        }
        check(count>=18,"All 18 bundled plugins registered");
        if(!preserve){
            static const char *names[]={"Calc","Clock","Memory","FileFinder","SystemCommands","DosTools","TaskManager","ClipboardTools","DateUtilities","WindowSwitcher","DeveloperTools","ARexxRunner","MusicControls","AminetBrowser","MusicLibrary","WHDLoad","DOpus","Wikipedia"};
            char disabled[256];
            for(i=0;i<18;i++){
                sprintf(disabled,"PLUGIN_DISABLED=PROGDIR:Plugins/%s/%s.plugin",names[i],names[i]);
                check(has_line(disabled)==(i>=6),names[i]);
            }
        }
        if(preserve)for(i=0;i<PROTECTED_COUNT;i++)if(had_file[i])
            check(hash_file(protected_files[i],&hash) && hash==before[i],protected_files[i]);
        sprintf(summary,"Selected plugins checked: %u / 18 bundled.\n",count);FPuts(report,summary);
    }
    sprintf(summary,"%d checks: %d passed, %d failed.\nReport: " REPORT,checks,checks-failures,failures);
    FPuts(report,summary);FPuts(report,"\nScope: installer files only; application launch/shutdown are not exercised.\n");Close(report);report=0;
}
static void notice(const char *text)
{
    struct EasyStruct es={sizeof(es),0,(STRPTR)"Beam68 Installer QA",(STRPTR)"%s",(STRPTR)"OK"};
    ULONG args[1];args[0]=(ULONG)text;EasyRequestArgs(NULL,&es,NULL,args);
}
static LONG run_installer(int uninstall)
{
    const char *installer=file_exists("SYS:System/Installer")?"SYS:System/Installer":"C:Installer";
    BPTR program=GetProgramDir(),package,previous,input,output;char path[512],command[800];LONG result;
    if(!program || !NameFromLock(program,path,sizeof(path)) || !AddPart(path,"Package",sizeof(path)))return -1;
    package=Lock(path,ACCESS_READ);if(!package)return -1;
    {
        const char *marker=uninstall?ROOT "/completed-uninstall":ROOT "/completed-install";
        DeleteFile((STRPTR)marker);
        if(file_exists(marker)){UnLock(package);return -1;}
    }
    sprintf(command,"\"%s\" \"%s\" APPNAME \"Beam68 Installer QA\" MINUSER %s DEFUSER %s LOGFILE " ROOT "/installer.log",
        installer,uninstall?"Uninstall-QA":"Install","NOVICE","NOVICE");
    input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
    if(!input || !output){if(input)Close(input);if(output)Close(output);UnLock(package);return -1;}
    previous=CurrentDir(package);
    result=SystemTags(command,SYS_Input,input,SYS_Output,output,NP_StackSize,65536UL,TAG_DONE);
    CurrentDir(previous);Close(input);Close(output);UnLock(package);return result;
}
static int qa_main(int argc,char **argv)
{
    LONG choice,result;int update,removed=0;
    struct EasyStruct menu={sizeof(menu),0,(STRPTR)"Beam68 Installer QA 0.2",
        (STRPTR)"Test installation lives in RAM:Beam68InstallerQA.\nYour installed Beam68 and settings are left alone.\n\nInstall once, then repeat to test an update.",
        (STRPTR)"Install / update|Check|Uninstall|Report|Quit"};
    if(!prepare())return 20;
    if(argc>1 && !strcmp(argv[1],"CHECK")){inspect(0,0);PutStr(summary);PutStr("\n");return failures?20:0;}
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);if(!IntuitionBase)return 20;
    for(;;){
        choice=EasyRequestArgs(NULL,&menu,NULL,NULL);if(choice==0 || choice==5)break;
        if(choice==1){
            update=file_exists(APP "/Beam68");if(update)snapshot();
            result=run_installer(0);
            if(result!=0 || !file_exists(ROOT "/completed-install")){sprintf(summary,"Installer returned %ld. It may have been cancelled.\nSee " ROOT "/installer.log.",(long)result);notice(summary);continue;}
            inspect(update,0);removed=0;notice(summary);
        }else if(choice==2){inspect(0,removed);notice(summary);}
        else if(choice==3){
            result=run_installer(1);
            if(result!=0 || !file_exists(ROOT "/completed-uninstall")){notice("Uninstall was cancelled or failed. See the installer log.");continue;}
            if(file_exists(APP "/Beam68")){notice("The test installation remains; removal was not confirmed.");continue;}
            removed=1;inspect(0,1);notice(summary);
        }else if(choice==4){notice(summary);}
    }
    CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;return 0;
}
int main(int argc,char **argv){return beam_run_with_stack(65536UL,qa_main,argc,argv);}
