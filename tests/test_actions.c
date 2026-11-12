#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "actions.h"
int main(void)
{
    char parent[BEAM_MAX_PATH],command[512],shell_command[64];
    const char *shell;
    assert(beam_parent_drawer("SYS:Tools/Ed",parent,sizeof(parent)));
    assert(!strcmp(parent,"SYS:Tools"));
    assert(beam_parent_drawer("SYS:Prefs",parent,sizeof(parent)));
    assert(!strcmp(parent,"SYS:"));
    assert(beam_parent_drawer("Work:Games/Doom/Doom",parent,sizeof(parent)));
    assert(!strcmp(parent,"Work:Games/Doom"));
    assert(beam_parent_drawer("Volume:App Name",parent,sizeof(parent)));
    assert(!strcmp(parent,"Volume:"));
    assert(!beam_parent_drawer("missing-volume",parent,sizeof(parent)));
    assert(!beam_parent_drawer("SYS:",parent,sizeof(parent)));
    assert(!beam_parent_drawer("SYS:Tools/Ed",parent,3));
    assert(beam_next_action(BEAM_ACTION_LAUNCH,1)==BEAM_ACTION_ARGS);
    assert(beam_next_action(BEAM_ACTION_ARGS,1)==BEAM_ACTION_DRAWER);
    assert(!strcmp(beam_action_name(BEAM_ACTION_ARGS,NULL),"Run with arguments"));
    assert(beam_next_action(BEAM_ACTION_DRAWER,1)==BEAM_ACTION_DOPUS);
    assert(beam_next_action(BEAM_ACTION_DOPUS,1)==BEAM_ACTION_INFO);
    assert(beam_next_action(BEAM_ACTION_INFO,1)==BEAM_ACTION_COPY);
    assert(beam_next_action(BEAM_ACTION_COPY,1)==BEAM_ACTION_CUSTOM);
    assert(beam_next_action(BEAM_ACTION_CUSTOM,1)==BEAM_ACTION_LAUNCH);
    assert(beam_next_action(BEAM_ACTION_COPY,0)==BEAM_ACTION_LAUNCH);
    assert(!strcmp(beam_action_name(BEAM_ACTION_INFO,NULL),"Information"));
    assert(!strcmp(beam_action_name(BEAM_ACTION_COPY,NULL),"Copy path"));
    assert(!strcmp(beam_action_name(BEAM_ACTION_DRAWER,NULL),"Open drawer"));
    assert(!strcmp(beam_action_name(BEAM_ACTION_CUSTOM,"Edit"),"Edit"));
    assert(beam_action_expand("C:Ed {path}","SYS:Tools/My App",command,sizeof(command)));
    assert(!strcmp(command,"C:Ed \"SYS:Tools/My App\""));
    assert(beam_action_expand("List {drawer}","Work:Apps/Tool",command,sizeof(command)));
    assert(!strcmp(command,"List \"Work:Apps\""));
    assert(!beam_action_expand("Run {path}","SYS:Bad\"Name",command,sizeof(command)));
    assert(!beam_action_expand("", "SYS:Tools/Ed",command,sizeof(command)));
    assert(beam_shell_query("shell:dir SYS:",&shell));
    assert(!strcmp(shell,"dir SYS:"));
    assert(beam_shell_query("SHELL:   info",&shell));
    assert(!strcmp(shell,"info"));
    assert(beam_shell_query("shell:",&shell) && !*shell);
    assert(beam_shell_query("> dir RAM:",&shell));
    assert(!strcmp(shell,"dir RAM:"));
    assert(beam_shell_command_copy("shell:  dir SYS:  ",shell_command,sizeof(shell_command)));
    assert(!strcmp(shell_command,"dir SYS:"));
    assert(beam_shell_command_copy(">  info\t",shell_command,sizeof(shell_command)));
    assert(!strcmp(shell_command,"info"));
    assert(!beam_shell_command_copy("find shell:here",shell_command,sizeof(shell_command)));
    assert(!beam_shell_command_copy("shell:too long",shell_command,4));
    assert(!beam_shell_query("shell",&shell));
    assert(!beam_shell_query("search:shell",&shell));
    assert(beam_restart_query("restart amiga")==BEAM_RESTART_AMIGA);
    assert(beam_restart_query(" RESTART:AMIGA ")==BEAM_RESTART_AMIGA);
    assert(beam_restart_query("restart   workbench")==BEAM_RESTART_WORKBENCH);
    assert(beam_restart_query("Restart: Workbench\t")==BEAM_RESTART_WORKBENCH);
    assert(beam_restart_query("restart")==BEAM_RESTART_NONE);
    assert(beam_restart_query("restart app")==BEAM_RESTART_NONE);
    assert(beam_restart_query("my restart amiga")==BEAM_RESTART_NONE);
    assert(!beam_action_expand("Run {path}","SYS:Bad*Name",command,sizeof(command)));
    assert(!beam_action_expand("Run {path}","SYS:`Execute`",command,sizeof(command)));
    assert(!beam_action_expand("Run {path}","SYS:Bad\tName",command,sizeof(command)));
    assert(!beam_action_expand("List {drawer}","SYS:Bad*/Tool",command,sizeof(command)));
    puts("Result actions, drawer parsing, shell/restart commands, navigation: PASS");
    return 0;
}
