/* Test-only build: exercise the real renderer and event handler in a disposable guest. */
#define main beam68_production_main
#include "../../src/main.c"
#undef main
#include <stdio.h>
static int failures;
static UWORD qa_qualifier;
static void check_ui(int ok,const char *name)
{printf("%s: %s\n",ok?"PASS":"FAIL",name);fflush(stdout);if(!ok)++failures;}
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
static void qa_plugin_rows(int omit_selected)
{
    struct Beam68ReplyMessage *message=AllocVec(sizeof(*message),MEMF_PUBLIC|MEMF_CLEAR);
    check_ui(message!=NULL,"Allocate synthetic plugin reply");if(!message)return;
    ++plugin_serial;beam_requests_reserve(&plugin_requests,plugin_serial,0);
    beam_job_begin_timeout(&plugin_job,5);GetSysTime(&plugin_started);
    strcpy(plugin_original_query,query);strcpy(plugin_active_name,"Selection QA");
    strcpy(plugin_active_manifest,"T:SelectionQA.plugin");plugin_file_reply=0;preview_job=omit_selected==2;
    message->message.mn_Length=sizeof(*message);message->magic=BEAM68_REPLY_MAGIC;
    message->version=BEAM68_REPLY_VERSION;message->serial=plugin_serial;message->count=omit_selected?1:2;
    message->rows[0].kind=BEAM68_ROW_QUERY;
    strcpy(message->rows[0].text,omit_selected?"Alpha":"Beta changed");
    strcpy(message->rows[0].value,omit_selected?"app:Alpha":"app:Beta");
    if(!omit_selected){message->rows[1].kind=BEAM68_ROW_QUERY;strcpy(message->rows[1].text,"Alpha");strcpy(message->rows[1].value,"app:Alpha");}
    if(omit_selected>=2){message->rows[0].kind=BEAM68_ROW_TEXT;message->rows[0].value[0]=0;strcpy(message->rows[0].text,omit_selected==2?"Preview only":"No matches");}
    PutMsg(reply_port,(struct Message *)message);drain_replies();
}

static void qa_late_reply(unsigned long serial)
{
    struct Beam68ReplyMessage *message=AllocVec(sizeof(*message),MEMF_PUBLIC|MEMF_CLEAR);
    check_ui(message!=NULL,"Allocate delayed native reply");if(!message)return;
    message->message.mn_Length=sizeof(*message);message->magic=BEAM68_REPLY_MAGIC;
    message->version=BEAM68_REPLY_VERSION;message->serial=serial;message->count=1;
    message->rows[0].kind=BEAM68_ROW_TEXT;strcpy(message->rows[0].text,"Obsolete reply");
    PutMsg(reply_port,(struct Message *)message);drain_replies();
}
static void qa_request_lifecycle(void)
{
    int i,slot;unsigned long old,current;struct Window *saved_window=window;
    memset(&plugin_requests,0,sizeof(plugin_requests));memset(plugin_request_file,0,sizeof(plugin_request_file));
    rows_active=0;strcpy(query,"new query");strcpy(status,"Keep current feedback");
    old=++plugin_serial;beam_requests_reserve(&plugin_requests,old,0);
    beam_job_begin(&plugin_job);plugin_cancel();
    current=++plugin_serial;beam_requests_reserve(&plugin_requests,current,0);
    strcpy(plugin_original_query,query);beam_job_begin(&plugin_job);GetSysTime(&plugin_started);
    strcpy(status,"Keep current feedback");qa_late_reply(old);
    check_ui(plugin_job.active && !rows_active && !strcmp(status,"Keep current feedback") &&
        beam_requests_count(&plugin_requests)==1,"Late cancelled reply releases only its slot and cannot replace newer results");
    window=NULL;qa_late_reply(current);window=saved_window;
    check_ui(!rows_active && !strcmp(status,"Keep current feedback") && !beam_requests_count(&plugin_requests),
        "Hidden-window reply is discarded while completed capacity is recovered");
    plugin_abandon();
    old=++plugin_serial;beam_requests_reserve(&plugin_requests,old,0);
    strcpy(query,"original");strcpy(plugin_original_query,query);beam_job_begin(&plugin_job);GetSysTime(&plugin_started);
    strcpy(query,"other");schedule_preview();strcpy(query,"original");schedule_preview();
    strcpy(status,"Keep current feedback");qa_late_reply(old);
    check_ui(!plugin_job.active && !rows_active && !strcmp(status,"Keep current feedback"),
        "Queued away-and-back edits cannot revive a stale reply for identical query text");
    for(i=0;i<BEAM_PLUGIN_REQUEST_LIMIT;i++)beam_requests_reserve(&plugin_requests,++plugin_serial,0);
    check_ui(!beam_requests_can_start(&plugin_requests,0),"Detached work stays bounded rather than being expired while still running");
    for(i=0;i<BEAM_PLUGIN_REQUEST_LIMIT;i++)qa_late_reply(plugin_requests.serial[i]);
    check_ui(!beam_requests_count(&plugin_requests),"All detached native completions recover request capacity");
    for(i=0;i<12;i++) {
        BPTR file;char part[BEAM_MAX_PATH+6];
        old=++plugin_serial;slot=beam_requests_reserve(&plugin_requests,old,0);
        check_ui(slot>=0,"Repeated cancelled file request can reserve a slot");if(slot<0)break;
        plugin_request_file[slot]=1;sprintf(plugin_request_path[slot],"T:Beam68RetiredQA-%d",i);
        beam_copy(part,plugin_request_path[slot],BEAM_MAX_PATH);strcat(part,".part");
        file=Open(part,MODE_NEWFILE);if(file){FPuts(file,"Writer in progress");Close(file);}
        window=NULL;plugin_retired_poll();window=saved_window;
        check_ui(beam_requests_count(&plugin_requests)==1,"Unpublished file reply is not deleted or prematurely released");
        file=Open(plugin_request_path[slot],MODE_NEWFILE);if(file){FPuts(file,"TEXT=Finished\n");Close(file);}
        window=NULL;plugin_retired_poll();window=saved_window;
        {BPTR lock=Lock(part,ACCESS_READ);check_ui(!lock && !beam_requests_count(&plugin_requests),"Late atomic reply and leftover part are cleaned without leaking slots");if(lock)UnLock(lock);}
    }
    plugin_health_flush_ticks=0;rows_active=0;plugin_row_count=0;selection_lost=0;
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
    check_ui(open_action_menu() && action_count==6,"Files include Open with and exclusion, omitting executable-only actions");
    action_menu=0;choose_category(1);cfg.shell_commands=0;
    check_ui(open_action_menu(),"Open menu with shell commands disabled");
    {int i,found=0;for(i=0;i<action_count;i++)if(action_choices[i]==BEAM_ACTION_ARGS)found=1;
     check_ui(!found,"Disabled shell commands remove argument action");}
    cfg.shell_commands=1;action_menu=0;cfg.detailed_rows=0;render();check_ui(!detailed_results(),"Compact row preference honored");
    {char first[80],second[100],converted[80];int old_top=result_top(),old_count=item_count;
     cfg.plugin_file_count=1;strcpy(plugin_routes[0].name,"Aminet Browser");
     strcpy(plugin_routes[0].prefix,"aminet:");plugin_routes[0].trigger=']';
     plugin_state[0]=BEAM_PLUGIN_DISABLED;strcpy(query,"aminet:search editor");
     search_items();render();
     check_ui(!result_count && unavailable_plugin_query(first,sizeof(first),second,sizeof(second)),"Disabled plugin has its own empty state");
     check_ui(strstr(first,"disabled") && strstr(second,"F5"),"Disabled route explains how to enable plugin");
     cfg.plugin_file_count=0;memset(plugin_routes,0,sizeof(plugin_routes));
     strcpy(query,"Editor");search_items();scan_active=1;scan_root=0;scan_roots=1;scan_count=17;
     render();check_ui(result_top()>old_top,"Indexing banner reserves result space");
     send_ui(IDCMP_MOUSEBUTTONS,SELECTDOWN,20,result_top()-2);
     check_ui(!scan_active && item_count==old_count,"Index banner cancels without removing results");
     check_ui(beam_calc_result("90 min to h",converted,sizeof(converted)) && !strcmp(converted,"1.5 h"),"Unit conversion on 68000 target");
     check_ui(beam_calc_result("1 GiB to B",converted,sizeof(converted)) && !strcmp(converted,"1073741824 B"),"Large byte conversion without FPU");
    }
    {char removed[BEAM_MAX_PATH];
     strcpy(query,"Editor");query_cursor=6;selection_is_item=selection_lost=0;search_items();
     selected=0;render();beam_copy(removed,items[results[selected]].path,sizeof(removed));
     beam_index_remove(&beam_index,removed,results,(unsigned int)result_count);++index_generation;
     search_pending=1;render();
     check_ui(selection_lost,"Disappeared indexed target is not silently replaced");
     launch_selected();check_ui(window && selection_lost,"Enter cannot launch the replacement target");
     send_ui(IDCMP_RAWKEY,RAW_DOWN,0,0);check_ui(!selection_lost,"Explicit navigation acknowledges the new selection");
     index_add(removed,strstr(removed,".txt")?BEAM_ITEM_FILE:BEAM_ITEM_WORKBENCH,BEAM_ROOT_NONE);
     selection_is_item=selection_lost=0;
    }
    {reply_port=CreateMsgPort();check_ui(reply_port!=NULL,"Create isolated plugin reply port");
     if(reply_port){
       strcpy(query,"selectionqa:");selection_is_item=selection_lost=0;rows_active=1;plugin_row_count=2;
       plugin_rows[0].kind=plugin_rows[1].kind=BEAM68_ROW_QUERY;
       strcpy(plugin_rows[0].text,"Alpha");strcpy(plugin_rows[0].value,"app:Alpha");
       strcpy(plugin_rows[1].text,"Beta");strcpy(plugin_rows[1].value,"app:Beta");
       search_items();selected=1;qa_plugin_rows(0);
       check_ui(!selection_lost && selected==0 && !strcmp(plugin_rows[results[selected]].value,"app:Beta"),"Plugin reorder and changed label retain target identity");
       qa_plugin_rows(2);check_ui(rows_active && !strcmp(plugin_rows[results[selected]].value,"app:Beta"),"Preview replies cannot overwrite actionable result rows");
       qa_plugin_rows(1);check_ui(selection_lost,"Disappeared plugin row requires a new selection");
       launch_selected();check_ui(!strcmp(query,"selectionqa:"),"Enter does not execute a replacement plugin row");
       qa_plugin_rows(3);check_ui(!rows_active && !strcmp(plugin_feedback,"No matches"),"Single text reply replaces rows and renders its feedback");
       qa_request_lifecycle();
       rows_active=0;plugin_row_count=0;selection_lost=selection_is_item=plugin_selection_valid=0;
       DeleteMsgPort(reply_port);reply_port=NULL;plugin_health_flush_ticks=0;
     }}
    CloseWindow(window);window=NULL;CloseScreen(test_screen);
}
static int qa_write_file(const char *path)
{
    BPTR file=Open((STRPTR)path,MODE_NEWFILE);if(!file)return 0;
    FPuts(file,"QA fixture\n");Close(file);return 1;
}
static void qa_finish_scan(void)
{
    int steps=0;while(scan_active && steps++<1000)scan_step();
    check_ui(!scan_active,"Incremental scan finishes within bounded steps");
}
static void qa_incremental_index(void)
{
    char root[100],a[120],b[120],nested[140],old[160],fresh[160],keep[160],other[160],more[180];
    BPTR lock=0;int i,before,kept;struct BeamSubWatch *watch=NULL;
    for(i=0;i<100;i++){sprintf(root,"T:Beam68IncrementalQA%02d",i);lock=CreateDir(root);if(lock)break;}
    check_ui(lock!=0,"Create private incremental fixture");if(!lock)return;UnLock(lock);
    sprintf(a,"%s/A",root);sprintf(b,"%s/B",root);sprintf(nested,"%s/B/Nested",root);
    lock=CreateDir(a);if(lock)UnLock(lock);lock=CreateDir(b);if(lock)UnLock(lock);lock=CreateDir(nested);if(lock)UnLock(lock);
    sprintf(old,"%s/old.txt",a);sprintf(fresh,"%s/new.txt",a);sprintf(keep,"%s/keep.txt",b);
    sprintf(other,"%s/other.txt",b);sprintf(more,"%s/more.txt",nested);
    check_ui(qa_write_file(old)&&qa_write_file(keep)&&qa_write_file(other)&&qa_write_file(more),"Write sibling subtree fixtures");
    beam_index_free(&beam_index);check_ui(beam_index_init(&beam_index,128),"Allocate fixture index");
    beam_config_defaults(&cfg);cfg.max_items=128;cfg.dir_count=1;strcpy(cfg.dirs[0],root);
    cfg.plugin_file_count=cfg.disabled_plugin_count=0;cfg.index_files=cfg.index_drawers=1;
    cfg.scan_icons=cfg.scan_executables=cfg.index_rexx=cfg.index_cache=0;cfg.max_depth=-1;
    strcpy(cfg.file_patterns,"*");query[0]=0;selection_lost=selection_is_item=0;result_count=0;
    notify_start();scan_begin(all_roots());qa_finish_scan();before=scan_count;
    kept=beam_index_find(&beam_index,keep);check_ui(kept>=0,"Full fixture scan indexes sibling");
    for(i=0;i<notify_child_count;i++)if(beam_equal(notify_children[i]->path,a))watch=notify_children[i];
    check_ui(watch!=NULL,"Visited subdrawer receives a bounded notification watch");
    if(watch){
        DeleteFile(old);check_ui(qa_write_file(fresh),"Replace a file in one subtree");Delay(5);notify_drain();
        check_ui(watch->pending || notify_pending_roots,"Filesystem change reaches notification queue");
        notify_pending_roots=0;for(i=0;i<notify_child_count;i++)notify_children[i]->pending=0;watch->pending=1;
        check_ui(notify_scan_next() && !scan_full_rebuild,"Queued subdrawer starts a partial scan");
        check_ui(!(items[kept].flags&BEAM_ITEM_FLAG_STALE),"Sibling entries never become stale during subtree scan");
        qa_finish_scan();
        check_ui(scan_count<before,"Subtree update scans fewer entries than a full root walk");
        check_ui(beam_index_find(&beam_index,old)<0 && beam_index_find(&beam_index,fresh)>=0 && beam_index_find(&beam_index,keep)>=0,
            "Partial reconciliation removes old file, adds new file and keeps sibling");
        scan_begin_subtree(watch);scan_cancel();
        check_ui(beam_index_find(&beam_index,fresh)>=0 && beam_index_find(&beam_index,keep)>=0,"Cancelled partial scan retains existing entries");
    }
    check_ui(apply_drawer_exclusion(a,0),"Apply exclusion for this disposable session");
    check_ui(!scan_active && beam_index_find(&beam_index,fresh)<0 && beam_index_find(&beam_index,a)<0 && beam_index_find(&beam_index,keep)>=0,
        "Exclusion immediately prunes the drawer and contents without rescanning siblings");
    {static struct BeamConfig restored;check_ui(beam_config_read(&restored,BEAM_CONFIG_SESSION) && restored.excluded_dir_count==1,
        "Session exclusion is persisted for restart without touching saved settings");}
    {char marker[320],line[320];BPTR output,actual,expected;struct Message *reply=NULL;int ticks;
     DeleteFile("T:Beam68OpenWithQA.txt");wb_reply_port=CreateMsgPort();
     check_ui(wb_reply_port && beam_wb_open_with(wb_reply_port,"Builds:OpenWithReceiver",keep),"Open with starts an explicit tool using WBStartup");
     for(ticks=0;wb_reply_port && ticks<200 && !(reply=GetMsg(wb_reply_port));ticks++)Delay(1);
     check_ui(reply!=NULL,"Explicit tool replies after consuming file argument");if(reply)beam_wb_launch_reply(reply);
     output=Open("T:Beam68OpenWithQA.txt",MODE_OLDFILE);marker[0]=line[0]=0;
     if(output){FGets(output,marker,sizeof(marker));FGets(output,line,sizeof(line));Close(output);}
     {unsigned int n=(unsigned int)strlen(line);while(n && (line[n-1]=='\n'||line[n-1]=='\r'))line[--n]=0;}
     actual=line[0]?Lock(line,ACCESS_READ):0;expected=Lock(keep,ACCESS_READ);
     check_ui(!strncmp(marker,"PASS",4) && actual && expected && SameLock(actual,expected)==LOCK_SAME,
        "Chosen application receives the exact selected file and parent lock");
     if(actual)UnLock(actual);
     if(expected)UnLock(expected);
     check_ui(!beam_wb_open_with(wb_reply_port,"Builds:MissingApplication",keep) && !beam_wb_launch_outstanding(),
        "Missing Open with executable leaves no startup request or locks outstanding");
     if(wb_reply_port)DeleteMsgPort(wb_reply_port);
     wb_reply_port=NULL;
    }
    notify_stop();notify_refresh_ticks=0;
}

static void qa_mkdir(const char *path){BPTR lock=CreateDir((STRPTR)path);if(lock)UnLock(lock);}
static int qa_text(const char *path,const char *text)
{BPTR file=Open((STRPTR)path,MODE_NEWFILE);int ok;if(!file)return 0;ok=FPuts(file,(STRPTR)text)>=0;Close(file);return ok;}
static int qa_contains(const char *path,const char *text)
{BPTR file=Open((STRPTR)path,MODE_OLDFILE);char value[80];LONG n;if(!file)return 0;n=Read(file,value,sizeof(value)-1);Close(file);if(n<0)return 0;value[n]=0;return !strcmp(value,text);}
static void qa_package_cache(void)
{
    char first[BEAM_MAX_PATH],second[BEAM_MAX_PATH],helper[BEAM_MAX_PATH],extra[BEAM_MAX_PATH];
    struct FileInfoBlock *info;BPTR lock;struct DateStamp stamp;int have_stamp=0;
    qa_mkdir("T:Beam68CacheQA");qa_mkdir("T:Beam68CacheQA/A");qa_mkdir("T:Beam68CacheQA/B");
    qa_mkdir("T:Beam68CacheQA/A/Common");qa_mkdir("T:Beam68CacheQA/B/Common");
    qa_text("T:Beam68CacheQA/A/Common/Tool","Executable");qa_text("T:Beam68CacheQA/B/Common/Tool","Executable");
    qa_text("T:Beam68CacheQA/A/Common/helper","First");qa_text("T:Beam68CacheQA/B/Common/helper","Other");
    cfg.plugin_cache=1;memset(&plugin_requests,0,sizeof(plugin_requests));
    check_ui(plugin_cached_command("T:Beam68CacheQA/A/Common/Tool",first,sizeof(first)),"Cache first package including its helper");
    beam_requests_reserve(&plugin_requests,++plugin_serial,0);
    check_ui(plugin_cached_command("T:Beam68CacheQA/B/Common/Tool",second,sizeof(second)) && strcmp(first,second),"Same-named plugin drawers have separate caches while requests are outstanding");
    beam_parent_drawer(first,helper,sizeof(helper));AddPart(helper,"helper",sizeof(helper));
    beam_parent_drawer(second,extra,sizeof(extra));AddPart(extra,"helper",sizeof(extra));
    check_ui(qa_contains(helper,"First") && qa_contains(extra,"Other"),"Second plugin cannot replace the first child's supporting files");
    memset(&plugin_requests,0,sizeof(plugin_requests));
    lock=Lock("T:Beam68CacheQA/A/Common/helper",ACCESS_READ);info=AllocDosObject(DOS_FIB,NULL);
    if(lock && info && Examine(lock,info)){stamp=info->fib_Date;have_stamp=1;}
    if(lock)UnLock(lock);
    if(info)FreeDosObject(DOS_FIB,info);
    qa_text("T:Beam68CacheQA/A/Common/helper","Fresh");
    if(have_stamp)SetFileDate("T:Beam68CacheQA/A/Common/helper",&stamp);
    check_ui(plugin_cached_command("T:Beam68CacheQA/A/Common/Tool",first,sizeof(first)) && qa_contains(helper,"Fresh"),
        "Helper-only edits refresh even with unchanged executable, helper size and timestamp");
    DeleteFile("T:Beam68CacheQA/A/Common/helper");qa_mkdir("T:Beam68CacheQA/A/Common/sub");
    qa_text("T:Beam68CacheQA/A/Common/sub/data","Nested");
    check_ui(plugin_cached_command("T:Beam68CacheQA/A/Common/Tool",first,sizeof(first)),"Added nested files and removed helpers refresh the cached tree");
    lock=Lock(helper,ACCESS_READ);check_ui(!lock,"Deleted helper does not survive in cache");if(lock)UnLock(lock);
    beam_parent_drawer(first,extra,sizeof(extra));AddPart(extra,"sub/data",sizeof(extra));
    check_ui(qa_contains(extra,"Nested"),"Nested support data is copied completely");
    {char deep[BEAM_MAX_PATH];int depth;
     strcpy(deep,"T:Beam68CacheQA/A/Common");
     for(depth=0;depth<18;depth++){AddPart(deep,"d",sizeof(deep));qa_mkdir(deep);}
     check_ui(!plugin_cached_command("T:Beam68CacheQA/A/Common/Tool",first,sizeof(first)),
        "Deep package falls back without running an incomplete RAM copy");}
    cfg.plugin_cache=0;
}
static void qa_visual_reload(void)
{
    static struct BeamConfig next;struct MsgPort *watched;int frames,old_plugins=plugin_count;
    beam_config_defaults(&cfg);cfg.plugin_file_count=cfg.disabled_plugin_count=0;
    cfg.index_games=cfg.index_rexx=0;cfg.dir_count=1;strcpy(cfg.dirs[0],"T:Beam68CacheQA/A");
    check_ui(beam_config_write(&cfg,BEAM_CONFIG_SESSION),"Create isolated session configuration for reload test");
    beam_config_defaults(&next);check_ui(beam_config_read_active(&next),"Read normalized active configuration");cfg=next;
    notify_start();scan_begin(1);
    check_ui(scan_push(cfg.dirs[0],0,0),"Open an in-progress scan frame before reloading");
    scan_root=1;watched=notify_port;frames=scan_frames_used;
    next=cfg;next.theme=BEAM_THEME_LATTE;next.row_spacing=cfg.row_spacing==7?8:7;
    check_ui(config_runtime_equal(&cfg,&next) && !config_appearance_equal(&cfg,&next),"Appearance classification excludes runtime changes");
    check_ui(beam_config_write(&next,BEAM_CONFIG_SESSION) && reload_config(),"Apply appearance edit through actual configuration reload");
    check_ui(notify_port==watched && scan_frames_used==frames && scan_active && plugin_count==old_plugins && cfg.theme==BEAM_THEME_LATTE,
        "Appearance reload retains notification port, in-progress walk and plugin catalogue");
    next=cfg;next.plugin_timeout++;
    check_ui(!config_runtime_equal(&cfg,&next),"Runtime setting changes cannot use appearance shortcut");
    qa_finish_scan();notify_stop();
}
static void qa_offline_volume(void)
{
    BPTR lock;struct Process *process=(struct Process *)FindTask(NULL);APTR original=process->pr_WindowPtr;
    beam_index_clear(&beam_index);beam_config_defaults(&cfg);cfg.plugin_file_count=cfg.disabled_plugin_count=0;
    cfg.index_games=cfg.index_rexx=0;cfg.index_files=cfg.index_drawers=1;cfg.dir_count=1;
    strcpy(cfg.dirs[0],"Beam68OfflineQA:");lock=Lock("T:Beam68CacheQA/A",ACCESS_READ);
    check_ui(lock && AssignLock("Beam68OfflineQA",lock),"Create a disposable removable-volume stand-in");
    scan_begin(1);qa_finish_scan();
    check_ui(beam_index_find(&beam_index,"Beam68OfflineQA:Common/Tool")>=0,"Available volume is indexed");
    AssignLock("Beam68OfflineQA",0);scan_begin(1);qa_finish_scan();
    check_ui(scan_unreadable>0 && beam_index_find(&beam_index,"Beam68OfflineQA:Common/Tool")>=0,
        "Unavailable root reports failure and retains previous searchable entries");
    strcpy(query,"Tool");help_mode=rows_active=ext_active=browse_active=selection_lost=0;args_item=-1;
    action_mode=BEAM_ACTION_LAUNCH;results[0]=beam_index_find(&beam_index,"Beam68OfflineQA:Common/Tool");result_count=1;selected=0;
    launch_selected();check_ui(strstr(status,"Reconnect")!=NULL,"Actual launcher refuses an offline indexed result with recovery guidance");
    check_ui(!available_result("Beam68OfflineQA:Common/Tool") && strstr(status,"Reconnect") &&
        process->pr_WindowPtr==original,"Unavailable launch explains recovery without leaving requesters suppressed");
    lock=Lock("T:Beam68CacheQA/A",ACCESS_READ);check_ui(lock && AssignLock("Beam68OfflineQA",lock),"Reconnect disposable volume");
    scan_begin(1);qa_finish_scan();check_ui(!scan_unreadable && available_result("Beam68OfflineQA:Common/Tool"),"Explicit rescan recovers after reconnect");
    AssignLock("Beam68OfflineQA",0);notify_stop();
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
    qa_incremental_index();
    qa_package_cache();qa_visual_reload();qa_offline_volume();
    check_ui(1,"Begin native QA cleanup");
    beam_index_free(&beam_index);check_ui(1,"Index released");cleanup_timer();check_ui(1,"Timer released");
    if(WorkbenchBase)CloseLibrary(WorkbenchBase);
    check_ui(1,"Workbench library released");
    CloseLibrary(IconBase);check_ui(1,"Icon library released");
    CloseLibrary(CxBase);check_ui(1,"Commodities library released");
    CloseLibrary((struct Library *)GfxBase);check_ui(1,"Graphics library released");
    CloseLibrary((struct Library *)IntuitionBase);check_ui(1,"Intuition library released");
    puts(failures?"Launcher UI QA: FAIL":"Launcher UI QA: PASS");return failures?20:0;
}

int main(int argc,char **argv)
{return beam_run_with_stack(BEAM_STACK_MINIMUM,ui_qa_main,argc,argv);}
