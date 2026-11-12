#ifndef BEAM68_SEARCH_SCOPE_H
#define BEAM68_SEARCH_SCOPE_H
#include "beam.h"

#define BEAM_SCOPE_ALL 0
#define BEAM_SCOPE_APP 1
#define BEAM_SCOPE_FILE 2
#define BEAM_SCOPE_DRAWER 3
#define BEAM_SCOPE_PLUGIN 4
#define BEAM_SCOPE_RECENT 5
#define BEAM_SCOPE_VOLUME 6
#define BEAM_SCOPE_FAVORITE 7
#define BEAM_SCOPE_GAME 8
#define BEAM_SCOPE_DOC 9       /* recent documents (live list) */
#define BEAM_SCOPE_HIDDEN 10   /* results hidden with Shift+Del */
#define BEAM_SCOPE_MODULE 11   /* music modules */
#define BEAM_SCOPE_SCRIPT 12   /* ARexx scripts */
#define BEAM_SCOPE_WEB 13      /* browser bookmarks */
#define BEAM_SCOPE_SNIPPET 14  /* text snippets */
#define BEAM_SCOPE_PREFS 15    /* preference programs (SYS:Prefs) */
#define BEAM_SCOPE_STATS 16    /* most-launched items (live list) */

/* Returns a BEAM_SCOPE_* value and points text at the part after the prefix.
 * Unknown prefixes remain ordinary search text for backward compatibility. */
int beam_search_scope_parse(const char *query,const char **text);
int beam_search_scope_kind_allowed(int scope,int item_kind);
/* Empty pattern lists accept all names. Comma/semicolon separated patterns
 * support '*' and '?' without requiring dos.library ParsePattern buffers. */
int beam_filename_patterns_match(const char *name,const char *patterns);
int beam_path_is_excluded(const char *path,
                          const char excluded[][BEAM_MAX_PATH],int count);

#endif
