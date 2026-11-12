#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "search_scope.h"
int main(void)
{
    const char *text;
    static char excluded[BEAM_MAX_EXCLUDED_DIRS][BEAM_MAX_PATH];
    assert(beam_search_scope_parse("app: screen",&text)==BEAM_SCOPE_APP);
    assert(!strcmp(text,"screen"));
    assert(beam_search_scope_parse("PLUGIN:music",&text)==BEAM_SCOPE_PLUGIN);
    assert(!strcmp(text,"music"));
    assert(beam_search_scope_parse("ordinary:query",&text)==BEAM_SCOPE_ALL);
    assert(!strcmp(text,"ordinary:query"));
    assert(beam_search_scope_kind_allowed(BEAM_SCOPE_APP,BEAM_ITEM_WORKBENCH));
    assert(!beam_search_scope_kind_allowed(BEAM_SCOPE_APP,BEAM_ITEM_FILE));
    assert(beam_search_scope_kind_allowed(BEAM_SCOPE_DRAWER,BEAM_ITEM_DRAWER));
    assert(beam_filename_patterns_match("readme.txt","*.txt,README.???"));
    assert(beam_filename_patterns_match("picture.iff","*.info;*.iff"));
    assert(!beam_filename_patterns_match("program","*.txt;*.iff"));
    assert(beam_filename_patterns_match("anything",""));
    strcpy(excluded[0],"SYS:Trashcan");
    strcpy(excluded[1],"Work:Cache");
    assert(beam_path_is_excluded("SYS:Trashcan/old",excluded,2));
    assert(!beam_path_is_excluded("SYS:Trashcan2/file",excluded,2));
    assert(beam_path_is_excluded("work:cache",excluded,2));
    assert(beam_search_scope_parse("GAME: turrican",&text)==BEAM_SCOPE_GAME && !strcmp(text,"turrican"));
    assert(beam_search_scope_kind_allowed(BEAM_SCOPE_GAME,BEAM_ITEM_GAME));
    assert(!beam_search_scope_kind_allowed(BEAM_SCOPE_APP,BEAM_ITEM_GAME));
    assert(beam_search_scope_kind_allowed(BEAM_SCOPE_ALL,BEAM_ITEM_GAME));
    assert(beam_search_scope_parse("doc:report",&text)==BEAM_SCOPE_DOC && !strcmp(text,"report"));
    assert(beam_search_scope_parse("hidden:",&text)==BEAM_SCOPE_HIDDEN && !*text);
    assert(beam_search_scope_parse("mod:debris",&text)==BEAM_SCOPE_MODULE && !strcmp(text,"debris"));
    assert(beam_search_scope_parse("web:aminet",&text)==BEAM_SCOPE_WEB);
    assert(beam_search_scope_parse("snip:sig",&text)==BEAM_SCOPE_SNIPPET);
    assert(beam_search_scope_parse("stats:",&text)==BEAM_SCOPE_STATS);
    assert(beam_search_scope_kind_allowed(BEAM_SCOPE_WEB,BEAM_ITEM_BOOKMARK) &&
           !beam_search_scope_kind_allowed(BEAM_SCOPE_WEB,BEAM_ITEM_FILE));
    puts("Search scopes, filename patterns and excluded drawers: PASS");
    return 0;
}
