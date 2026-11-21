#include <exec/types.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <intuition/screens.h>
#include <proto/wb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beam68plugin.h"
#include "plugin_common.h"
#include "aminet.h"
#include "aminet_http.h"
#include "control.h"
#include "stack_guard.h"
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *WorkbenchBase;
static LONG aminet_ask(struct Window *parent,struct EasyStruct *prompt,ULONG *args)
{
#ifdef BEAM_AMINET_QA
    extern int beam_aminet_qa_ask(const struct EasyStruct *);
    (void)parent;(void)args;return beam_aminet_qa_ask(prompt);
#else
    return EasyRequestArgs(parent,prompt,NULL,args);
#endif
}
static struct Window *progress_window;
static char input[121],path[401],temporary[240],error_text[121],text[121],value[240];
static struct BeamAminetResults results;
static struct BeamAminetDetails details;
static unsigned long last_progress,progress_bytes,progress_total,last_progress_tick,last_progress_total;
static char progress_name[108];
static unsigned long transfer_start,transfer_rate,transfer_remaining;
static int cancelled;
static void query_row(const char *label,const char *verb,const char *argument)
{
    sprintf(value,"aminet:%s%s%s",verb,*argument?" ":"",argument);
    beam68_row(BEAM68_ROW_QUERY,label,value);
}
static void draw_progress(void)
{
    struct RastPort *rp;struct DrawInfo *di;
    unsigned long background=0,foreground=1,fill=3,shadow=1,shine=2;
    int x=14,right,y,height,percent,n;char label[160];
    if(!progress_window || !GfxBase)return;
    rp=progress_window->RPort;right=progress_window->Width-15;
    di=GetScreenDrawInfo(progress_window->WScreen);
    if(di) {
        background=di->dri_Pens[BACKGROUNDPEN];foreground=di->dri_Pens[TEXTPEN];
        fill=di->dri_Pens[FILLPEN];shadow=di->dri_Pens[SHADOWPEN];shine=di->dri_Pens[SHINEPEN];
        FreeScreenDrawInfo(progress_window->WScreen,di);
    }
    SetDrMd(rp,JAM1);SetAPen(rp,background);
    RectFill(rp,progress_window->BorderLeft,progress_window->BorderTop,
        progress_window->Width-progress_window->BorderRight-1,progress_window->Height-progress_window->BorderBottom-1);
    height=rp->TxHeight;y=progress_window->BorderTop+height+8;
    n=strlen(progress_name);while(n>0 && TextLength(rp,(STRPTR)progress_name,n)>right-x)n--;
    SetAPen(rp,foreground);Move(rp,x,y);Text(rp,(STRPTR)progress_name,n);
    percent=beam_aminet_percent(progress_bytes,progress_total);
    if(percent<0){if(progress_bytes<1024UL && progress_bytes)sprintf(label,"%lu bytes downloaded (size unknown)",progress_bytes);else if(progress_bytes)sprintf(label,"%lu KB downloaded (size unknown)",progress_bytes/1024UL);else strcpy(label,"Connecting to Aminet...");}
    else if(progress_total<1024UL)sprintf(label,"%lu / %lu bytes (%d%%)",progress_bytes,progress_total,percent);
    else sprintf(label,"%lu / %lu KB (%d%%)",progress_bytes/1024UL,progress_total/1024UL,percent);
    y+=height+8;Move(rp,x,y);Text(rp,(STRPTR)label,strlen(label));
    y+=12;
    SetAPen(rp,shadow);RectFill(rp,x,y,right,y+17);
    SetAPen(rp,shine);RectFill(rp,x+1,y+1,right-1,y+16);
    SetAPen(rp,background);RectFill(rp,x+2,y+2,right-2,y+15);
    if(percent>0){SetAPen(rp,fill);RectFill(rp,x+2,y+2,x+1+(right-x-3)*percent/100,y+15);}
    if(percent<0 && progress_bytes){
        int span=right-x-20,position=(int)((progress_bytes/1024UL)%((unsigned long)(span>0?span:1)));
        SetAPen(rp,fill);RectFill(rp,x+2+position,y+2,x+14+position,y+15);
    }
    SetAPen(rp,foreground);Move(rp,x,y+height+25);
    if(!transfer_rate)strcpy(label,"Measuring transfer speed...");
    else if(transfer_remaining==~0UL)sprintf(label,"%lu bytes/s | time remaining unknown",transfer_rate);
    else sprintf(label,"%lu bytes/s | about %lu:%02lu remaining",transfer_rate,transfer_remaining/60UL,transfer_remaining%60UL);
    Text(rp,(STRPTR)label,strlen(label));
    Move(rp,x,y+2*height+32);Text(rp,(STRPTR)"Close this window to cancel.",27);
}
static int progress(unsigned long bytes,unsigned long total)
{
    struct IntuiMessage *message;struct DateStamp stamp;unsigned long now;int repaint=0;
    if(!progress_window)return 1;
    progress_bytes=bytes;progress_total=total;
    while((message=(struct IntuiMessage *)GetMsg(progress_window->UserPort))) {
        ULONG cls=message->Class;ReplyMsg((struct Message *)message);
        if(cls==IDCMP_CLOSEWINDOW)cancelled=1;
        else if(cls==IDCMP_REFRESHWINDOW){BeginRefresh(progress_window);draw_progress();EndRefresh(progress_window,TRUE);}
    }
    if(bytes/4096UL!=last_progress)repaint=1;
    if(total && (bytes==total || bytes==0))repaint=1;
    DateStamp(&stamp);now=(unsigned long)stamp.ds_Days*4320000UL+(unsigned long)stamp.ds_Minute*3000UL+(unsigned long)stamp.ds_Tick;
    if(last_progress==~0UL)transfer_start=now;
    beam_aminet_transfer_stats(bytes,total,now-transfer_start,&transfer_rate,&transfer_remaining);
    if(now-last_progress_tick>=25UL)repaint=1;
    if(repaint && (last_progress==~0UL || now-last_progress_tick>=5UL || total!=last_progress_total || (total && bytes==total))) {
        int percent=beam_aminet_percent(bytes,total);
        if(percent>=0)sprintf(text,"Aminet download - %d%%",percent);
        else strcpy(text,bytes?"Aminet download - size unknown":"Aminet download - connecting");
        last_progress=bytes/4096UL;last_progress_tick=now;last_progress_total=total;
        SetWindowTitles(progress_window,(STRPTR)text,(STRPTR)~0UL);draw_progress();
    }
    return !cancelled;
}
static int reserve_drawer(const char *parent,char *out,unsigned int cap)
{
    int i;BPTR lock;char name[24];
    for(i=0;i<100;i++) {
        beam_plugin_copy(out,parent,cap);sprintf(name,"Beam68Aminet%02d",i);
        if(!AddPart(out,name,cap)) return 0;
        lock=CreateDir((STRPTR)out);if(lock) { UnLock(lock);return 1; }
        if(IoErr()!=ERROR_OBJECT_EXISTS) return 0;
    }
    return 0;
}
static int metadata(const char *remote,char *drawer)
{
    if(!reserve_drawer("T:",drawer,240)) {
        strcpy(error_text,"Cannot reserve a temporary Aminet drawer.");return 0;
    }
    strcpy(temporary,drawer);AddPart(temporary,"response",sizeof(temporary));
    if(beam_aminet_fetch(remote,temporary,262144UL,20,0,NULL,error_text,sizeof(error_text))) return 1;
    DeleteFile((STRPTR)drawer);return 0;
}
static void clean_metadata(const char *drawer)
{ DeleteFile((STRPTR)temporary);DeleteFile((STRPTR)drawer); }
static int parse_offset(char **argument,int *offset)
{
    char *s=*argument;unsigned long n=0;
    if(*s<'0'||*s>'9') return 0;
    while(*s>='0'&&*s<='9') { if(n>99995) return 0;n=n*10+(unsigned long)(*s++-'0'); }
    if(*s!=' ' || n>999950UL) return 0;
    while(*s==' ') s++;
    *argument=s;*offset=(int)n;return 1;
}
static int search(int argc,char **argv,char *query,int offset)
{
    char encoded[241],drawer[240],argument[100];FILE *file;int i,valid;
    if(!beam_aminet_encode_query(query,encoded,sizeof(encoded)))
        return beam_plugin_error(argc,argv,"Use 2-80 search characters. Example: aminet:search tracker");
    sprintf(path,"/search?query=%s&page=%d",encoded,offset/50+1);
    if(!metadata(path,drawer)) return beam_plugin_error(argc,argv,error_text);
    file=fopen(temporary,"r");valid=file && beam_aminet_search(file,offset%50,&results);
    if(file) fclose(file);
    clean_metadata(drawer);
    if(!valid) return beam_plugin_error(argc,argv,"Cannot read Aminet search results; the page format may have changed.");
    beam68_rows_begin();
    for(i=0;i<results.count;i++) {
        const char *name=strrchr(results.rows[i].path,'/')+1;
        sprintf(text,"%.48s - %.68s",name,results.rows[i].title);
        query_row(text,"info",results.rows[i].path);
    }
    if(!results.count) beam68_row(BEAM68_ROW_TEXT,"No packages on this page. Try a narrower search.","");
    if(offset>0) {
        sprintf(argument,"%d %s",offset%50==0?offset-2:(offset>=6?offset-6:0),query);
        query_row("Previous results","page",argument);
    }
    if(results.count && (offset%50+results.count<results.seen || results.seen>=50)) {
        sprintf(argument,"%d %s",offset+results.count,query);query_row("Next results","page",argument);
    }
    return beam68_rows_send(argc,argv,0);
}
static int show_readme(int argc,char **argv,const char *package,int offset,int overview)
{
    char drawer[240],line[512],argument[120];FILE *file;int valid=0,i=0,count=0,more=0;
    if(!beam_aminet_readme_path(package,path,sizeof(path)))
        return beam_plugin_error(argc,argv,"Use a package path such as util/wb/Beam68-0.1.lha.");
    if(!metadata(path,drawer)) return beam_plugin_error(argc,argv,error_text);
    file=fopen(temporary,"r");beam68_rows_begin();
    if(file && overview) {
        valid=beam_aminet_details(file,&details);
        if(valid) {
            beam68_row(BEAM68_ROW_TEXT,details.summary,"");
            sprintf(text,"Version: %.40s | %.65s",details.version,details.architecture);
            beam68_row(BEAM68_ROW_TEXT,text,"");
            if(details.author[0]) { sprintf(text,"Author: %.111s",details.author);beam68_row(BEAM68_ROW_TEXT,text,""); }
            if(details.requires[0]) { sprintf(text,"Requires: %.109s",details.requires);beam68_row(BEAM68_ROW_TEXT,text,""); }
            sprintf(argument,"0 %s",package);query_row("Read full readme","read",argument);
            query_row("Download archive...","get",package);
            query_row("Download and open for installation...","install",package);
        }
    } else if(file) {
        while(fgets(line,sizeof(line),file)) {
            beam_aminet_plain(line,text,sizeof(text));if(!*text) continue;
            if(i++<offset) continue;
            if(count==6) { more=1;break; }
            beam68_row(BEAM68_ROW_TEXT,text,"");count++;
        }
        valid=!ferror(file);
        if(!count) beam68_row(BEAM68_ROW_TEXT,"End of readme.","");
        if(offset>0) { sprintf(argument,"%d %s",offset>=6?offset-6:0,package);query_row("Previous readme lines","read",argument); }
        else query_row("Package details","info",package);
        if(more) { sprintf(argument,"%d %s",offset+count,package);query_row("Next readme lines","read",argument); }
    }
    if(file) fclose(file);
    clean_metadata(drawer);
    if(!valid) return beam_plugin_error(argc,argv,"Cannot read this package's Aminet readme.");
    return beam68_rows_send(argc,argv,0);
}
static int download_directory(char *out)
{
    static const char *files[]={"ENVARC:Beam68/Plugins/AminetBrowser.cfg","ENV:Beam68/Plugins/AminetBrowser.cfg","PROGDIR:AminetBrowser.cfg",NULL};
    BPTR file=0;char line[256];int i;unsigned int n;
    strcpy(out,"RAM:Beam68Downloads");
    for(i=0;files[i]&&!file;i++) file=Open((STRPTR)files[i],MODE_OLDFILE);
    if(file) {
        while(FGets(file,line,sizeof(line))) {
            char *s=beam_plugin_trim(line);
            if(beam_ascii_prefix(s,"DOWNLOAD_DIR=")) {
                s=beam_plugin_trim(s+13);
                if(strlen(s)>160) { Close(file);return 0; }
                strcpy(out,s);
            }
        }
        Close(file);
    }
    n=(unsigned int)strlen(out);
    if(!n || !strchr(out,':')) return 0;
    for(i=0;out[i];i++) if((unsigned char)out[i]<32 || out[i]=='"' || out[i]=='*' || out[i]=='`') return 0;
    return 1;
}
static int archive_type(const char *s)
{
    const char *dot=strrchr(s,'.');
    return dot && (beam_ascii_equal(dot,".lha")||beam_ascii_equal(dot,".lzh")||beam_ascii_equal(dot,".zip")||beam_ascii_equal(dot,".tgz")||beam_ascii_equal(dot,".gz"));
}
static int open_download_object(const char *name)
{
#ifdef BEAM_AMINET_QA
    extern int beam_aminet_qa_open(const char *);return beam_aminet_qa_open(name);
#else
    int ok;WorkbenchBase=OpenLibrary("workbench.library",44);
    ok=WorkbenchBase && OpenWorkbenchObjectA((STRPTR)name,NULL);
    if(WorkbenchBase){CloseLibrary(WorkbenchBase);WorkbenchBase=NULL;}
    if(!ok)strcpy(error_text,"Cannot open this item. Open it manually from the download drawer (Workbench v44+ required).");
    return ok;
#endif
}
static int view_download_readme(const char *package,const char *directory)
{
    char remote[240],target[240],drawer[240],part[240],command[512];
    const char *filename;BPTR lock,in,out;LONG rc;
    if(!beam_aminet_readme_path(package,remote,sizeof(remote)))return 0;
    filename=strrchr(remote,'/');filename=filename?filename+1:remote;
    strcpy(target,directory);
    if(!AddPart(target,(STRPTR)filename,sizeof(target))){strcpy(error_text,"Readme path is too long.");return 0;}
    lock=Lock((STRPTR)target,ACCESS_READ);
    if(lock)UnLock(lock);
    else {
        if(!reserve_drawer(directory,drawer,sizeof(drawer))){strcpy(error_text,"Cannot create a readme staging drawer.");return 0;}
        strcpy(part,drawer);AddPart(part,"readme.part",sizeof(part));
        beam_plugin_copy(progress_name,filename,sizeof(progress_name));last_progress=~0UL;cancelled=0;
        if(!beam_aminet_fetch(remote,part,262144UL,30,0,progress,error_text,sizeof(error_text))){
            DeleteFile((STRPTR)part);DeleteFile((STRPTR)drawer);return 0;
        }
        if(!Rename((STRPTR)part,(STRPTR)target)){
            strcpy(error_text,"Cannot save readme; existing files were preserved.");
            DeleteFile((STRPTR)part);DeleteFile((STRPTR)drawer);return 0;
        }
        DeleteFile((STRPTR)drawer);
    }
#ifdef BEAM_AMINET_QA
    {extern int beam_aminet_qa_open(const char *);(void)command;(void)in;(void)out;(void)rc;
     return beam_aminet_qa_open(target);}
#else
    lock=Lock("C:MultiView",ACCESS_READ);
    if(!lock){strcpy(error_text,"Readme saved in the download drawer. MultiView is unavailable.");return 0;}UnLock(lock);
    in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
    if(!in||!out){if(in)Close(in);if(out)Close(out);strcpy(error_text,"Cannot open readme viewer streams.");return 0;}
    sprintf(command,"C:MultiView \"%s\"",target);
    rc=SystemTags(command,SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,32768UL,TAG_DONE);
    if(rc<0){Close(in);Close(out);strcpy(error_text,"Cannot start MultiView; the readme was saved.");return 0;}
    return 1;
#endif
}
static int verify_download_archive(const char *package,const char *target)
{
    unsigned char header[8];size_t n=0;FILE *file=fopen(target,"rb");
    if(file){n=fread(header,1,sizeof(header),file);fclose(file);}
    if(!beam_aminet_archive_header(package,header,(unsigned int)n)){
        strcpy(error_text,"This file is not a supported archive. Open it manually from the download drawer.");return 0;
    }return 1;
}
static void download_complete(const char *package,const char *target,const char *directory)
{
    struct EasyStruct prompt={sizeof(struct EasyStruct),0,"Aminet download complete","Downloaded:\n%s\nInstall opens your archive handler; follow the package instructions.",NULL};
    ULONG args[1];int choice,ok,archive=archive_type(package);
    prompt.es_GadgetFormat=(STRPTR)(archive?"Open drawer|View readme|Install...|Close":"Open drawer|View readme|Close");args[0]=(ULONG)target;
    while((choice=aminet_ask(progress_window,&prompt,args))!=0){
        ok=choice==1?open_download_object(directory):choice==2?view_download_readme(package,directory):
            verify_download_archive(package,target)&&open_download_object(target);
        if(!ok){struct EasyStruct error={sizeof(struct EasyStruct),0,"Aminet Browser",(STRPTR)error_text,"Close"};aminet_ask(progress_window,&error,NULL);}
    }
}
static int download(int argc,char **argv,const char *package,int install)
{
    struct MsgPort *guard=NULL;struct EasyStruct es;ULONG args[1];BPTR lock;
    char directory[240],drawer[240],target[240],part[240];int delivered=0,ok=0,added=0;
    const char *filename=strrchr(package,'/')+1;
    if(install && !archive_type(package))
        return beam_plugin_error(argc,argv,"Installation opens archives only (.lha, .lzh, .zip, .tgz or .gz).");
    if(!download_directory(directory)) return beam_plugin_error(argc,argv,"Set a valid DOWNLOAD_DIR in AminetBrowser settings.");
    guard=CreateMsgPort();if(!guard) return beam_plugin_error(argc,argv,"Not enough memory for the download session.");
    guard->mp_Node.ln_Name=(STRPTR)"BEAM68_AMINET_DOWNLOAD";
    Forbid();
    if(!FindPort(guard->mp_Node.ln_Name)) { AddPort(guard);added=1; }
    Permit();
    if(!added) { DeleteMsgPort(guard);return beam_plugin_error(argc,argv,"An Aminet download is already open. Close it before starting another."); }
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",37);
    if(!IntuitionBase) goto cleanup;
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",37);
    if(!GfxBase)goto cleanup;
    es.es_StructSize=sizeof(es);es.es_Flags=0;es.es_Title=(STRPTR)"Aminet Browser";
    es.es_TextFormat=(STRPTR)(install?
        "Download %s and open it with your archive handler?\nThen follow the package's own installer or instructions.\nNo installed files are replaced by Beam68.":
        "Download %s?\nExisting files will not be overwritten.");
    es.es_GadgetFormat=(STRPTR)"Download|Cancel";args[0]=(ULONG)package;
    if(!aminet_ask(NULL,&es,args)) { beam_plugin_text(argc,argv,"Aminet download cancelled.");delivered=1;goto cleanup; }
    lock=Lock((STRPTR)directory,ACCESS_READ);
    if(!lock) lock=CreateDir((STRPTR)directory);
    if(!lock) { strcpy(error_text,"Cannot open or create the download drawer.");goto cleanup; }
    UnLock(lock);strcpy(target,directory);
    if(!AddPart(target,filename,sizeof(target))) { strcpy(error_text,"Download path is too long.");goto cleanup; }
    lock=Lock((STRPTR)target,ACCESS_READ);
    if(lock) { UnLock(lock);strcpy(error_text,"File already exists; rename it or select another download drawer.");goto cleanup; }
retry_transfer:
    if(!reserve_drawer(directory,drawer,sizeof(drawer))) { strcpy(error_text,"Cannot reserve a download staging drawer.");goto cleanup; }
    strcpy(part,drawer);AddPart(part,"archive.part",sizeof(part));
    if(!progress_window)progress_window=OpenWindowTags(NULL,WA_Title,(ULONG)"Aminet download - close to cancel",
        WA_Width,440,WA_Height,150,WA_DragBar,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW,TAG_DONE);
    if(!progress_window) { strcpy(error_text,"Cannot open the download window.");DeleteFile((STRPTR)drawer);goto cleanup; }
    /* The visible, individually cancellable transfer owns its lifecycle after
     * this acknowledgement. A separate public-port guard bounds it to one. */
#ifndef BEAM_AMINET_QA
    if(!delivered && beam_plugin_text(argc,argv,"Aminet download started. Close its window to cancel.")!=0) {
        strcpy(error_text,"Cannot acknowledge the download request.");DeleteFile((STRPTR)drawer);goto cleanup;
    }
#endif
    delivered=1;cancelled=0;last_progress=~0UL;progress_bytes=progress_total=transfer_rate=0;transfer_remaining=~0UL;
    beam_plugin_copy(progress_name,filename,sizeof(progress_name));draw_progress();
    /* Transfer owns its window now; leave a newer launcher query alone. */
    if(argc==5)beam_control_plugin_handoff(strtoul(argv[4],NULL,10));
    sprintf(path,"/%s",package);
    ok=beam_aminet_fetch(path,part,67108864UL,300,1,progress,error_text,sizeof(error_text));
    if(ok && install) {
        unsigned char signature[8];size_t size=0;FILE *archive=fopen(part,"rb");
        if(archive) { size=fread(signature,1,sizeof(signature),archive);fclose(archive); }
        if(!beam_aminet_archive_header(package,signature,(unsigned int)size)) {
            strcpy(error_text,"Downloaded data is not the expected archive format.");ok=0;
        }
    }
    if(ok && !Rename((STRPTR)part,(STRPTR)target)) { strcpy(error_text,"Cannot commit download; existing files are preserved.");ok=0; }
    DeleteFile((STRPTR)part);DeleteFile((STRPTR)drawer);
    if(ok){
        if(install && !open_download_object(target)){
            struct EasyStruct warning={sizeof(struct EasyStruct),0,"Archive downloaded",(STRPTR)error_text,"Close"};aminet_ask(progress_window,&warning,NULL);
        }
        download_complete(package,target,directory);
    }else if(!cancelled){
        es.es_TextFormat=(STRPTR)"%s\nPartial data removed; existing files preserved.\nRetry starts a new download.";
        es.es_GadgetFormat=(STRPTR)"Retry|Close";args[0]=(ULONG)error_text;
        if(aminet_ask(progress_window,&es,args)==1)goto retry_transfer;
    }

cleanup:
    if(progress_window) { CloseWindow(progress_window);progress_window=NULL; }
    if(GfxBase){CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;}
    if(IntuitionBase) { CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL; }
    RemPort(guard);DeleteMsgPort(guard);
    if(!delivered) return beam_plugin_error(argc,argv,*error_text?error_text:"Cannot start the Aminet download.");
    return 0;
}
static int run(int argc,char **argv)
{
    char *s,*argument,*separator;int offset=0;
    if(!beam_plugin_input(argc,argv,input,sizeof(input))) return 20;
    s=beam_plugin_trim(input);
    if(!*s || beam_ascii_equal(s,"help")) {
        beam68_rows_begin();query_row("Search example: tracker","search","tracker");
        beam68_row(BEAM68_ROW_TEXT,"Type aminet:search followed by 2-80 search characters.","");
        beam68_row(BEAM68_ROW_TEXT,"Select a package, then press Enter again for details.","");
        beam68_row(BEAM68_ROW_TEXT,"Downloads and installation preparation always ask first.","");
        return beam68_rows_send(argc,argv,0);
    }
    separator=argument=strchr(s,' ');
    if(argument) { *argument++=0;while(*argument==' ') argument++; }
    if(argument && (beam_ascii_equal(s,"page")||beam_ascii_equal(s,"read"))) {
        if(!parse_offset(&argument,&offset)) return beam_plugin_error(argc,argv,"Invalid page offset.");
        if(beam_ascii_equal(s,"page")) return search(argc,argv,argument,offset);
        if(!beam_aminet_path_valid(argument)) return beam_plugin_error(argc,argv,"Invalid Aminet package path.");
        return show_readme(argc,argv,argument,offset,0);
    }
    if(argument && beam_ascii_equal(s,"search")) return search(argc,argv,argument,0);
    if(argument && (beam_ascii_equal(s,"info")||beam_ascii_equal(s,"get")||beam_ascii_equal(s,"install"))) {
        if(!beam_aminet_path_valid(argument)) return beam_plugin_error(argc,argv,"Invalid Aminet package path.");
        if(beam_ascii_equal(s,"info")) return show_readme(argc,argv,argument,0,1);
        return download(argc,argv,argument,beam_ascii_equal(s,"install"));
    }
    if(separator) *separator=' ';
    if(beam_aminet_path_valid(s)) return show_readme(argc,argv,s,0,1);
    return search(argc,argv,s,0);
}
int main(int argc,char **argv)
{ return beam_run_with_stack(32768UL,run,argc,argv); }
