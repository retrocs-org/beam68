#include <exec/types.h>
#include <exec/ports.h>
#include <clib/alib_protos.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <errno.h>
#include <libraries/gadtools.h>
#include <graphics/text.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beam68plugin.h"
#include "plugin_common.h"
#include "wikipedia.h"
#include "launcher_ui.h"
#include "control.h"
#include "stack_guard.h"
struct IntuitionBase *IntuitionBase;
struct Library *GadToolsBase;
static struct BeamWikiResults results;
static char language[4]="en",curl[64]="C:curl",openurl[64]="C:OpenURL";
static char input[121],url[1024],command[1600],temporary[240],directory[240];
static char error[121];
static int safe_tool(const char *tool)
{return *tool && strchr(tool,':') && !strpbrk(tool,"\"*`\r\n");}
static void settings(void)
{
    char line[256],*key,*value;BPTR file=Open((STRPTR)"ENVARC:Beam68/Plugins/Wikipedia.cfg",MODE_OLDFILE);
    if(!file)return;
    while(FGets(file,line,sizeof(line))) {
        key=beam_plugin_trim(line);value=strchr(key,'=');if(!value || *key=='#' || *key==';')continue;
        *value++=0;key=beam_plugin_trim(key);value=beam_plugin_trim(value);
        if(beam_ascii_equal(key,"LANGUAGE") && beam_wiki_language(value))beam_plugin_copy(language,value,sizeof(language));
        if(beam_ascii_equal(key,"CURL"))beam_plugin_copy(curl,value,sizeof(curl));
        if(beam_ascii_equal(key,"OPENURL"))beam_plugin_copy(openurl,value,sizeof(openurl));
    }
    Close(file);
}
static int fetch(unsigned long serial)
{
    BPTR lock;FILE *file;int attempt,ok=0;long size;
    if(!safe_tool(curl)){strcpy(error,"Set a full curl executable path in Wikipedia Settings.");return 0;}
    lock=Lock((STRPTR)curl,ACCESS_READ);
    if(!lock){strcpy(error,"HTTPS downloader not found. Install curl and set its path in Wikipedia Settings.");return 0;}UnLock(lock);
    for(attempt=0;attempt<32;attempt++) {
        sprintf(directory,"T:Beam68Wiki-%lx-%lx-%x",(unsigned long)FindTask(NULL),serial,attempt);
        lock=CreateDir((STRPTR)directory);if(lock){UnLock(lock);break;}
    }
    if(attempt==32){strcpy(error,"Cannot create a temporary Wikipedia download drawer.");return 0;}
    beam_plugin_copy(temporary,directory,sizeof(temporary));AddPart(temporary,(STRPTR)"reply.json",sizeof(temporary));
    sprintf(command,"\"%s\" -fsS --max-time 20 --connect-timeout 10 --max-filesize 65536 --proto =https --tlsv1.2 -A \"Beam68Wikipedia/0.2 (https://beam68.retrocs.org)\" -o \"%s\" \"%s\"",curl,temporary,url);
    if(strlen(command)>=512)strcpy(error,"Wikipedia search is too long; try fewer words.");
    else if(SystemTags((STRPTR)command,SYS_Input,0,SYS_Output,0,TAG_DONE)!=0)
        strcpy(error,"Wikipedia request failed. Check the TCP/IP stack and curl HTTPS support.");
    else {
        file=fopen(temporary,"rb");
        if(file) {
            if(fseek(file,0,SEEK_END)==0 && (size=ftell(file))>0 && size<=65536L && fseek(file,0,SEEK_SET)==0)
                ok=beam_wiki_read(file,&results);
            fclose(file);
        }
        if(!ok)strcpy(error,"Wikipedia returned an invalid or oversized response.");
    }
    DeleteFile((STRPTR)temporary);DeleteFile((STRPTR)directory);return ok;
}
static int open_article(unsigned long id)
{
    char article[240];
    if(!safe_tool(openurl) || !beam_wiki_article_url(language,id,article,sizeof(article),0))return 0;
    {BPTR lock=Lock((STRPTR)openurl,ACCESS_READ);if(!lock)return 0;UnLock(lock);}
    sprintf(command,"\"%s\" \"%s\"",openurl,article);
    return SystemTags((STRPTR)command,SYS_Asynch,TRUE,SYS_Input,0,SYS_Output,0,TAG_DONE)==0;
}
static int summary(int argc,char **argv,unsigned long id,unsigned long serial)
{
    static char body[2100],lines[96][100];static struct Node nodes[96];
    struct MsgPort *guard=NULL,*existing;struct Screen *screen=NULL;void *visual=NULL;
    struct Gadget *list=NULL,*last,*view;struct Window *window=NULL;struct List labels;
    struct TextAttr font={(STRPTR)"topaz.font",8,0,0};struct NewGadget ng;
    struct IntuiMessage *message;const char *cursor;int count=0,width,height,border,done=0,ack=0,scroll=0,visible;
    char title[144],article[240];
    if(!beam_wiki_article_url(language,id,url,sizeof(url),1) || !fetch(serial))return beam_plugin_error(argc,argv,error);
    if(!results.count || !results.pages[0].extract[0])return beam_plugin_error(argc,argv,"No article summary is available for this result.");
    guard=CreateMsgPort();if(!guard)return beam_plugin_error(argc,argv,"Not enough memory to open Wikipedia.");
    guard->mp_Node.ln_Name=(STRPTR)"BEAM68_WIKIPEDIA_SUMMARY";
    Forbid();existing=FindPort(guard->mp_Node.ln_Name);if(!existing)AddPort(guard);Permit();
    if(existing){DeleteMsgPort(guard);return beam_plugin_error(argc,argv,"A Wikipedia summary is already open. Close it before opening another.");}
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",37);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    if(!IntuitionBase || !GadToolsBase)goto cleanup;
    screen=LockPubScreen(NULL);if(!screen)goto cleanup;
    visual=GetVisualInfoA(screen,NULL);if(!visual)goto cleanup;
    width=(int)screen->Width-12;if(width>720)width=720;if(width<440)goto cleanup;
    beam_wiki_article_url(language,id,article,sizeof(article),0);
    sprintf(body,"%s\n\n%s\n\nSource: Wikipedia contributors\n%s\nText: CC BY-SA 4.0\nhttps://creativecommons.org/licenses/by-sa/4.0/\nExcerpt shortened; unsupported display characters replaced.",results.pages[0].title,results.pages[0].extract,article);
    NewList(&labels);memset(nodes,0,sizeof(nodes));cursor=body;
    while(*cursor && count<96) {
        unsigned int next,n=beam_text_line(cursor,(unsigned int)(width-44)/8,&next);
        if(!next)break;
        if(n>=sizeof(lines[0]))n=sizeof(lines[0])-1;
        memcpy(lines[count],cursor,n);lines[count][n]=0;nodes[count].ln_Name=lines[count];AddTail(&labels,&nodes[count]);
        cursor+=next;count++;
    }
    border=(int)screen->WBorTop+(int)screen->Font->ta_YSize+8;
    height=border+count*8+44;if(height>(int)screen->Height-(int)screen->BarHeight-4)height=(int)screen->Height-(int)screen->BarHeight-4;
    if(height<140)goto cleanup;
    last=CreateContext(&list);if(!last)goto cleanup;
    memset(&ng,0,sizeof(ng));ng.ng_TextAttr=&font;ng.ng_VisualInfo=visual;
    ng.ng_LeftEdge=12;ng.ng_TopEdge=border;ng.ng_Width=width-24;ng.ng_Height=height-border-40;ng.ng_GadgetID=1;
    last=CreateGadget(LISTVIEW_KIND,last,&ng,GTLV_Labels,(ULONG)&labels,GTLV_ReadOnly,TRUE,TAG_DONE);if(!last)goto cleanup;
    view=last;visible=(height-border-40)/8;if(visible<1)visible=1;
    ng.ng_TopEdge=height-27;ng.ng_Width=155;ng.ng_Height=18;ng.ng_GadgetText=(STRPTR)"Open article";ng.ng_Flags=PLACETEXT_IN;ng.ng_GadgetID=2;
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto cleanup;
    ng.ng_LeftEdge=width-112;ng.ng_Width=100;ng.ng_GadgetText=(STRPTR)"Close";ng.ng_GadgetID=3;
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto cleanup;
    sprintf(title,"Wikipedia - %.120s",results.pages[0].title);
    window=OpenWindowTags(NULL,WA_PubScreen,(ULONG)screen,WA_Title,(ULONG)title,
        WA_Left,(ULONG)(((int)screen->Width-width)/2),WA_Top,(ULONG)((int)screen->BarHeight+2),
        WA_Width,(ULONG)width,WA_Height,(ULONG)height,WA_Gadgets,(ULONG)list,
        WA_DragBar,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,WA_SmartRefresh,TRUE,
        WA_IDCMP,IDCMP_CLOSEWINDOW|LISTVIEWIDCMP|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,TAG_DONE);
    if(!window)goto cleanup;
    GT_RefreshWindow(window,NULL);
    if(beam_plugin_text(argc,argv,"Wikipedia summary opened.")!=0)goto cleanup;
    ack=1;beam_control_plugin_handoff(serial);
    while(!done) {
        WaitPort(window->UserPort);
        while((message=GT_GetIMsg(window->UserPort))) {
            ULONG cls=message->Class;UWORD code=message->Code;
            struct Gadget *gadget=(struct Gadget *)message->IAddress;UWORD gid=cls==IDCMP_GADGETUP && gadget?gadget->GadgetID:0;
            GT_ReplyIMsg(message);if(done)continue;
            if(cls==IDCMP_CLOSEWINDOW || gid==3 || (cls==IDCMP_RAWKEY && code==0x45))done=1;
            else if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(window);GT_EndRefresh(window,TRUE);}
            else if(cls==IDCMP_RAWKEY && (code==0x4c || code==0x4d)) {
                scroll+=code==0x4c?-1:1;
                if(scroll>count-visible)scroll=count-visible;
                if(scroll<0)scroll=0;
                GT_SetGadgetAttrs(view,window,NULL,GTLV_Top,(ULONG)scroll,TAG_DONE);
            }
            else if(gid==2 && !open_article(id))SetWindowTitles(window,(STRPTR)"Check OpenURL executable in Wikipedia Settings",(STRPTR)~0UL);
        }
    }
cleanup:
    if(window)CloseWindow(window);
    if(list)FreeGadgets(list);
    if(visual)FreeVisualInfo(visual);
    if(screen)UnlockPubScreen(NULL,screen);
    if(GadToolsBase){CloseLibrary(GadToolsBase);GadToolsBase=NULL;}
    if(IntuitionBase){CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;}
    Forbid();RemPort(guard);Permit();DeleteMsgPort(guard);
    return ack?0:beam_plugin_error(argc,argv,"Cannot open the Wikipedia summary window.");
}
static int run(int argc,char **argv)
{
    char *query,*p,*end;unsigned long serial,id;int i;
    if(!beam_plugin_input(argc,argv,input,sizeof(input)))return 20;
    settings();serial=strtoul(argv[4],NULL,10);query=beam_plugin_trim(input);
    if(beam_ascii_prefix(query,"article ")) {
        query=beam_plugin_trim(query+8);p=strpbrk(query," \t");
        if(p){*p++=0;if(!beam_wiki_language(query))return beam_plugin_error(argc,argv,"Invalid Wikipedia language.");beam_plugin_copy(language,query,sizeof(language));query=beam_plugin_trim(p);}
        if(!*query || *query<'0' || *query>'9')return beam_plugin_error(argc,argv,"Invalid Wikipedia article number.");
        errno=0;id=strtoul(query,&end,10);if(errno || *end || !id || id>0xffffffffUL)return beam_plugin_error(argc,argv,"Invalid Wikipedia article number.");
        return summary(argc,argv,id,serial);
    }
    if(beam_ascii_prefix(query,"search "))query=beam_plugin_trim(query+7);
    if(!*query)return beam_plugin_text(argc,argv,"Use wiki:Amiga, select a result, then Enter again for its summary.");
    if(!beam_wiki_search_url(language,query,url,sizeof(url)))return beam_plugin_error(argc,argv,"Wikipedia search is too long or invalid.");
    if(!fetch(serial))return beam_plugin_error(argc,argv,error);
    if(!results.count)return beam_plugin_error(argc,argv,"No matching Wikipedia articles found.");
    beam68_rows_begin();
    for(i=0;i<results.count;i++) {
        sprintf(command,"wiki:article %s %lu",language,results.pages[i].id);
        beam68_row(BEAM68_ROW_QUERY,results.pages[i].title,command);
    }
    return beam68_rows_send(argc,argv,0);
}
int main(int argc,char **argv){return beam_run_with_stack(32768UL,run,argc,argv);}
