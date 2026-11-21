/* Test-only build: exercise the real renderer and event handler in a disposable guest. */
#define main beam68_production_main
#include "../../src/main.c"
#undef main
#include <stdio.h>
static int failures;
static UWORD qa_qualifier;
static void check_ui(int ok,const char *name)
{printf("%s: %s\n",ok?"PASS":"FAIL",name);if(!ok)++failures;}
static void send_ui(ULONG cls,UWORD code,int x,int y)
{
    struct IntuiMessage message;struct MsgPort *reply=CreateMsgPort();
    if(!reply){++failures;return;}
    memset(&message,0,sizeof(message));message.ExecMessage.mn_ReplyPort=reply;
    message.ExecMessage.mn_Length=sizeof(message);message.Class=cls;message.Code=code;
    message.Qualifier=qa_qualifier;
    message.MouseX=(WORD)x;message.MouseY=(WORD)y;message.IDCMPWindow=window;
    PutMsg(window->UserPort,(struct Message *)&message);handle_window_messages();
    GetMsg(reply);DeleteMsgPort(reply);
}
static void screen_ui(int height,int compact)
{
    struct Screen *test_screen;
    int lh,top,bottom,stride;
    test_screen=OpenScreenTags(NULL,SA_Width,640,SA_Height,(ULONG)height,SA_Depth,3,
        SA_DisplayID,height>256?0x8004UL:0x8000UL,SA_Type,CUSTOMSCREEN,TAG_DONE);
    check_ui(test_screen!=NULL,"Open native test screen");if(!test_screen)return;
    window=OpenWindowTags(NULL,WA_CustomScreen,(ULONG)test_screen,WA_Left,4UL,WA_Top,18UL,
        WA_Width,620UL,WA_Height,(ULONG)(height-30),WA_DragBar,TRUE,
        WA_IDCMP,IDCMP_RAWKEY|IDCMP_VANILLAKEY|IDCMP_MOUSEBUTTONS,TAG_DONE);
    check_ui(window!=NULL,"Open native launcher window");if(!window){CloseScreen(test_screen);return;}
    compact_active=compact;palette.text=1;palette.background=0;palette.fill=3;
    palette.fill_text=2;palette.shadow=1;cfg.icon_preview=0;cfg.detailed_rows=1;
    strcpy(query,"Editor");query_cursor=6;search_items();render();
    check_ui(result_count==2,"Initial query has app and file matches");
    {char suggested[121];
     choose_category(1);strcpy(query,"app:eD");query_cursor=6;search_items();
     check_ui(inline_suggestion(suggested,sizeof(suggested)) && !strcmp(suggested,"app:eDitor"),"Inline completion preserves scope and typed case");
     qa_qualifier=IEQUALIFIER_CONTROL;send_ui(IDCMP_RAWKEY,RAW_RIGHT,0,0);qa_qualifier=0;
     check_ui(!strcmp(query,"app:eDitor") && query_cursor==(int)strlen(query),"Ctrl+Right explicitly accepts local suggestion");
     check_ui(!inline_suggestion(suggested,sizeof(suggested)),"Complete names do not suggest duplicate text");
     strcpy(query,"Editor");query_cursor=6;search_items();render();}

    lh=launcher_lineheight(window->RPort);top=result_top();
    bottom=window->Height-window->BorderBottom-lh-8;stride=result_stride();
    check_ui(beam_result_rows(top,stride,bottom,result_count)>=1,"Result rows fit above footer");
    check_ui(detailed_results()==!compact,"Detailed rows and compact fallback");
    check_ui(beam_hit_result(top+stride+1,top,stride,bottom,2)==1,"Second row hit uses rendered stride");
    send_ui(IDCMP_MOUSEBUTTONS,SELECTDOWN,window->BorderLeft+6+(window->Width-window->BorderLeft-window->BorderRight-12)/5+4,
        window->BorderTop+lh+8);
    check_ui(!strcmp(query,"app:Editor") && result_count==1 && items[results[0]].kind==BEAM_ITEM_WORKBENCH,"Apps category filters without losing query");
    send_ui(IDCMP_RAWKEY,RAW_TAB,0,0);
    check_ui(action_menu && action_count>=4,"Tab opens applicable action menu");
    send_ui(IDCMP_VANILLAKEY,9,0,0);
    check_ui(action_menu && action_choice==0,"Vanilla Tab does not select an action twice");
    send_ui(IDCMP_RAWKEY,RAW_DOWN,0,0);
    check_ui(action_menu && action_choice==1,"Arrow navigates action menu");
    send_ui(IDCMP_RAWKEY,RAW_ESCAPE,0,0);send_ui(IDCMP_VANILLAKEY,27,0,0);
    check_ui(window!=NULL && !action_menu,"Escape closes only menu, retaining launcher");
    send_ui(IDCMP_RAWKEY,RAW_TAB,0,0);send_ui(IDCMP_RAWKEY,RAW_DOWN,0,0);
    send_ui(IDCMP_VANILLAKEY,13,0,0);
    check_ui(!action_menu && args_item>=0,"Enter chooses Run with arguments without launching");
    send_ui(IDCMP_RAWKEY,RAW_ESCAPE,0,0);
    check_ui(args_item<0 && window!=NULL,"Escape leaves argument entry safely");
    choose_category(0);strcpy(query,"Editor");query_cursor=6;search_items();
    send_ui(IDCMP_MOUSEBUTTONS,MENUDOWN,0,0);
    check_ui(action_menu,"Right-click exposes result actions");
    action_menu=0;choose_category(2);
    check_ui(result_count==1 && items[results[0]].kind==BEAM_ITEM_FILE,"Files category filters results");
    check_ui(open_action_menu() && action_count==4,"Files omit executable-only actions");
    action_menu=0;choose_category(1);cfg.shell_commands=0;
    check_ui(open_action_menu(),"Open menu with shell commands disabled");
    {int i,found=0;for(i=0;i<action_count;i++)if(action_choices[i]==BEAM_ACTION_ARGS)found=1;
     check_ui(!found,"Disabled shell commands remove argument action");}
    cfg.shell_commands=1;action_menu=0;cfg.detailed_rows=0;render();check_ui(!detailed_results(),"Compact row preference honored");
    CloseWindow(window);window=NULL;CloseScreen(test_screen);
}
static int ui_qa_main(int argc,char **argv)
{
    (void)argc;(void)argv;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    CxBase=OpenLibrary("commodities.library",39);IconBase=OpenLibrary("icon.library",39);
    WorkbenchBase=OpenLibrary("workbench.library",0);
    if(!IntuitionBase || !GfxBase || !CxBase || !IconBase || !init_timer())return 20;
    beam_config_defaults(&cfg);cfg.plugin_file_count=cfg.disabled_plugin_count=0;
    plugins_loaded=1;args_item=-1;plugin_discovery=-1;
    if(!beam_index_init(&beam_index,64))return 20;
    index_add("SYS:Tools/Editor",BEAM_ITEM_WORKBENCH,BEAM_ROOT_NONE);
    index_add("Work:Documents/Editor.txt",BEAM_ITEM_FILE,BEAM_ROOT_NONE);
    screen_ui(200,1);screen_ui(400,0);
    beam_index_free(&beam_index);cleanup_timer();
    if(WorkbenchBase)CloseLibrary(WorkbenchBase);
    CloseLibrary(IconBase);CloseLibrary(CxBase);
    CloseLibrary((struct Library *)GfxBase);CloseLibrary((struct Library *)IntuitionBase);
    puts(failures?"Launcher UI QA: FAIL":"Launcher UI QA: PASS");return failures?20:0;
}

int main(int argc,char **argv)
{return beam_run_with_stack(BEAM_STACK_MINIMUM,ui_qa_main,argc,argv);}
