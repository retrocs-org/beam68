#ifndef BEAM68_ACTIONS_H
#define BEAM68_ACTIONS_H
#include "beam.h"
#define BEAM_ACTION_LAUNCH 0
#define BEAM_ACTION_DRAWER 1
#define BEAM_ACTION_CUSTOM 2
#define BEAM_ACTION_INFO 3     /* Workbench Information window */
#define BEAM_ACTION_COPY 4     /* copy the path to the clipboard */
#define BEAM_ACTION_ARGS 5     /* run with arguments typed next */
#define BEAM_ACTION_DOPUS 6    /* show the drawer in Directory Opus */
#define BEAM_RESTART_NONE 0
#define BEAM_RESTART_AMIGA 1
#define BEAM_RESTART_WORKBENCH 2
/* Return parent drawer for SYS:Tool and SYS:Games/Foo/Bar.
 * Avoids OS-dependent FilePart() in the portable test suite. */
int beam_parent_drawer(const char *path,char *parent,unsigned int cap);
int beam_next_action(int current,int has_custom);
const char *beam_action_name(int action,const char *custom_label);
/* Expand {path} and {drawer} into quoted AmigaDOS arguments. */
int beam_action_expand(const char *template_text,const char *path,
                       char *output,unsigned int cap);
/* Recognise an explicit, case-insensitive shell: prefix or the compact >
 * prefix. The returned command points inside query after whitespace and may
 * be empty. */
int beam_shell_query(const char *query,const char **command);
/* Copy the recognised command without leading/trailing horizontal whitespace.
 * Returns zero for a non-shell query or insufficient output capacity. */
int beam_shell_command_copy(const char *query,char *command,unsigned int cap);
/* Recognise exact, case-insensitive "restart amiga" / "restart:amiga" and
 * Workbench equivalents. A bare "restart" remains an ordinary search. */
int beam_restart_query(const char *query);
#endif
