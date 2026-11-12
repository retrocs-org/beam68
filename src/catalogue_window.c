/* Catalogue browser for Beam68Prefs (GadTools, AmigaOS 3.0+). */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <stdio.h>
#include <string.h>
#include "beam.h"
#include "catalogue.h"
#include "catalogue_window.h"
#include "plugin_install.h"
#include "dos_compat.h"
#include "prefs_locale.h"

#define CW_SOURCE 1
#define CW_LOAD 2
#define CW_LIST 3
#define CW_INFO 4
#define CW_INSTALL 5
#define CW_CLOSE 6
#define CAT_INDEX "T:Beam68-Catalogue.txt"

struct CatState {
    struct BeamCatalogueEntry entries[BEAM_CAT_MAX];
    struct Node nodes[BEAM_CAT_MAX];
    char labels[BEAM_CAT_MAX][96];
    struct List list;
    int count;
    char info[160];
};

static struct TextAttr cat_font={(STRPTR)"topaz.font",8,FS_NORMAL,FPF_ROMFONT};

static void init_list(struct List *l)
{
    l->lh_Head=(struct Node *)&l->lh_Tail;
    l->lh_Tail=NULL;
    l->lh_TailPred=(struct Node *)&l->lh_Head;
}

static int is_remote(const char *s) { return strstr(s,"://")!=NULL; }

static int fetch(const struct BeamConfig *c,const char *url,const char *file)
{
    char cmd[400];
    if (!beam_catalogue_command(c->download_command,url,file,cmd,sizeof(cmd))) return 0;
    DeleteFile((STRPTR)file);
    return beam_system(cmd,NULL,NULL,0)==0;
}

static int load(struct CatState *st,const struct BeamConfig *c,const char *source)
{
    static char line[512];
    const char *path=source;
    BPTR f;
    st->count=0;
    init_list(&st->list);
    if (!*source) return 0;
    if (is_remote(source)) {
        if (!fetch(c,source,CAT_INDEX)) return 0;
        path=CAT_INDEX;
    }
    f=Open((STRPTR)path,MODE_OLDFILE);
    if (!f) return 0;
    if (!FGets(f,line,sizeof(line)) || strncmp(line,"BEAM68CATALOGUE 1",17)) { Close(f); return 0; }
    while (st->count<BEAM_CAT_MAX && FGets(f,line,sizeof(line))) {
        struct BeamCatalogueEntry *e=&st->entries[st->count];
        if (!beam_catalogue_parse_line(line,e)) continue;
        {
            char label[96];
            sprintf(label,"%s %.40s %.12s - %.24s",
                    e->kind==BEAM_CAT_THEME?"[T]":"[P]",e->name,e->version,e->author);
            beam_copy(st->labels[st->count],label,sizeof(st->labels[0]));
        }
        st->nodes[st->count].ln_Name=(STRPTR)st->labels[st->count];
        AddTail(&st->list,&st->nodes[st->count]);
        st->count++;
    }
    Close(f);
    return 1;
}

static struct Gadget *add(struct Gadget *prev,APTR visual,ULONG kind,UWORD id,int x,int y,
                          int w,int h,const char *text,ULONG place,struct TagItem *tags)
{
    struct NewGadget ng;
    memset(&ng,0,sizeof(ng));
    ng.ng_LeftEdge=(WORD)x; ng.ng_TopEdge=(WORD)y; ng.ng_Width=(WORD)w; ng.ng_Height=(WORD)h;
    ng.ng_GadgetText=(UBYTE *)text; ng.ng_TextAttr=&cat_font; ng.ng_GadgetID=id;
    ng.ng_Flags=place; ng.ng_VisualInfo=visual;
    return CreateGadgetA(kind,prev,&ng,tags);
}

int beam_catalogue_window(struct Screen *screen,APTR visual,struct BeamConfig *c,
                          int (*import_theme)(const char *path),char *msg,unsigned int cap)
{
    struct CatState *st;
    struct Gadget *glist=NULL,*last,*g_source,*g_list,*g_info;
    struct Window *cw;
    struct IntuiMessage *m;
    struct TagItem tags[4];
    int top,finished=0,installed=0,chosen=-1;
    msg[0]=0;
    st=(struct CatState *)AllocVec(sizeof(*st),MEMF_ANY|MEMF_CLEAR);
    if (!st) return 0;
    init_list(&st->list);
    top=screen->WBorTop+screen->Font->ta_YSize+6;
    last=CreateContext(&glist);
    tags[0].ti_Tag=GTST_String; tags[0].ti_Data=(ULONG)c->catalogue_url;
    tags[1].ti_Tag=GTST_MaxChars; tags[1].ti_Data=sizeof(c->catalogue_url)-1;
    tags[2].ti_Tag=TAG_DONE;
    last=g_source=add(last,visual,STRING_KIND,CW_SOURCE,70,top,360,14,pstr(PMSG_CAT_SOURCE),PLACETEXT_LEFT,tags);
    tags[0].ti_Tag=TAG_DONE;
    last=add(last,visual,BUTTON_KIND,CW_LOAD,440,top-1,80,15,pstr(PMSG_CAT_LOAD),PLACETEXT_IN,tags);
    tags[0].ti_Tag=GTLV_Labels; tags[0].ti_Data=(ULONG)&st->list;
    tags[1].ti_Tag=GTLV_ShowSelected; tags[1].ti_Data=0;
    tags[2].ti_Tag=TAG_DONE;
    last=g_list=add(last,visual,LISTVIEW_KIND,CW_LIST,10,top+20,510,96,NULL,0,tags);
    tags[0].ti_Tag=GTTX_Border; tags[0].ti_Data=TRUE; tags[1].ti_Tag=TAG_DONE;
    last=g_info=add(last,visual,TEXT_KIND,CW_INFO,10,top+120,510,14,NULL,0,tags);
    tags[0].ti_Tag=TAG_DONE;
    last=add(last,visual,BUTTON_KIND,CW_INSTALL,10,top+140,120,15,pstr(PMSG_CAT_INSTALL),PLACETEXT_IN,tags);
    last=add(last,visual,BUTTON_KIND,CW_CLOSE,400,top+140,120,15,pstr(PMSG_CAT_CLOSE),PLACETEXT_IN,tags);
    if (!last) { FreeGadgets(glist); FreeVec(st); return 0; }
    cw=OpenWindowTags(NULL,WA_PubScreen,(ULONG)screen,WA_Left,40,WA_Top,30,
        WA_Width,530,WA_Height,(ULONG)(top+164),WA_Title,(ULONG)pstr(PMSG_CAT_TITLE),
        WA_Gadgets,(ULONG)glist,WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,
        WA_Activate,TRUE,
        WA_IDCMP,LISTVIEWIDCMP|BUTTONIDCMP|STRINGIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW,
        TAG_DONE);
    if (!cw) { FreeGadgets(glist); FreeVec(st); return 0; }
    GT_RefreshWindow(cw,NULL);
    while (!finished) {
        WaitPort(cw->UserPort);
        while ((m=GT_GetIMsg(cw->UserPort))) {
            ULONG cls=m->Class; UWORD code=m->Code;
            struct Gadget *gad=(struct Gadget *)m->IAddress;
            GT_ReplyIMsg(m);
            if (cls==IDCMP_CLOSEWINDOW) { finished=1; continue; }
            if (cls==IDCMP_REFRESHWINDOW) { GT_BeginRefresh(cw); GT_EndRefresh(cw,TRUE); continue; }
            if (cls!=IDCMP_GADGETUP || !gad) continue;
            if (gad->GadgetID==CW_CLOSE) finished=1;
            else if (gad->GadgetID==CW_LOAD || gad->GadgetID==CW_SOURCE) {
                struct StringInfo *si=(struct StringInfo *)g_source->SpecialInfo;
                if (si && si->Buffer) beam_copy(c->catalogue_url,beam_trim((char *)si->Buffer),sizeof(c->catalogue_url));
                GT_SetGadgetAttrs(g_list,cw,NULL,GTLV_Labels,~0UL,TAG_DONE);
                chosen=-1;
                if (!load(st,c,c->catalogue_url)) beam_copy(st->info,pstr(PMSG_CAT_LOAD_FAILED),sizeof(st->info));
                else if (!st->count) beam_copy(st->info,pstr(PMSG_CAT_EMPTY),sizeof(st->info));
                else st->info[0]=0;
                GT_SetGadgetAttrs(g_list,cw,NULL,GTLV_Labels,(ULONG)&st->list,TAG_DONE);
                GT_SetGadgetAttrs(g_info,cw,NULL,GTTX_Text,(ULONG)st->info,TAG_DONE);
            } else if (gad->GadgetID==CW_LIST && code<(UWORD)st->count) {
                chosen=code;
                beam_copy(st->info,st->entries[chosen].description,sizeof(st->info));
                GT_SetGadgetAttrs(g_info,cw,NULL,GTTX_Text,(ULONG)st->info,TAG_DONE);
            } else if (gad->GadgetID==CW_INSTALL && chosen>=0) {
                const struct BeamCatalogueEntry *e=&st->entries[chosen];
                char local[BEAM_MAX_PATH];
                int ok=0;
                if (is_remote(e->url)) {
                    sprintf(local,"T:Beam68-Download%s",beam_catalogue_extension(e->url));
                    if (!fetch(c,e->url,local)) {
                        beam_copy(st->info,pstr(PMSG_CAT_DOWNLOAD_FAILED),sizeof(st->info));
                        GT_SetGadgetAttrs(g_info,cw,NULL,GTTX_Text,(ULONG)st->info,TAG_DONE);
                        continue;
                    }
                } else beam_copy(local,e->url,sizeof(local));
                if (e->kind==BEAM_CAT_THEME) {
                    ok=import_theme && import_theme(local);
                    beam_copy(msg,pstr(ok?PMSG_CAT_THEME_OK:PMSG_THEME_INVALID),cap);
                } else ok=beam_plugin_install(cw,c,local,msg,cap);
                if (is_remote(e->url)) DeleteFile((STRPTR)local);
                if (ok) installed=1;
                beam_copy(st->info,msg,sizeof(st->info));
                GT_SetGadgetAttrs(g_info,cw,NULL,GTTX_Text,(ULONG)st->info,TAG_DONE);
            }
        }
    }
    CloseWindow(cw);
    FreeGadgets(glist);
    FreeVec(st);
    DeleteFile((STRPTR)CAT_INDEX);
    return installed;
}
