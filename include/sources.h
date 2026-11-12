#ifndef BEAM68_SOURCES_H
#define BEAM68_SOURCES_H
/* Extra index sources (portable helpers, host-tested):
 *   - music modules found in the indexed drawers (mod: scope),
 *   - browser bookmarks from HTML hotlists (web: scope),
 *   - text snippets from ENVARC:Beam68/Snippets.txt (snip: scope),
 *   - ARexx scripts in REXX: (script: scope). */
#define BEAM_SNIPPETS_PATH "ENVARC:Beam68/Snippets.txt"
/* mod.*, *.mod, *.med, *.xm, *.s3m, *.it, *.ahx, ... (case-insensitive). */
int beam_module_name(const char *filename);
/* One HTML line with <a href="URL">Title</a> (NetSurf/AWeb hotlists,
 * IBrowse HTML export).  Only http/https/ftp/gopher/file URLs. */
int beam_bookmark_parse(const char *line, char *url, unsigned int ucap,
                        char *title, unsigned int tcap);
/* General scanner for any hotlist layout: finds the next bookmark in
 * `text` - an <a href> anchor (several per line allowed) or, outside HTML,
 * a bare http(s):// URL titled by the text before it.  Returns a pointer
 * just past the match, or NULL when there is none.  `*partial` is set when
 * the text ends inside an unfinished anchor (caller keeps that tail and
 * appends the next line). */
const char *beam_bookmark_next(const char *text, char *url, unsigned int ucap,
                               char *title, unsigned int tcap, int *partial);
/* "name<TAB>text" (comments start with '#'). */
int beam_snippet_parse(const char *line, char *name, unsigned int ncap,
                       char *text, unsigned int tcap);
/* Replace every `key` (e.g. "{file}") in a command template with `value`;
 * refuses values with quotes, '*', backquote or control characters. */
int beam_template_fill(const char *template_text, const char *key,
                       const char *value, char *out, unsigned int cap);
#endif
