/* Beam68 public plugin protocol 1: asynchronous selectable-result messages.
 * Native invocation: <plugin> <HEX> <reply-path> BEAM68_REPLY <serial>.
 * Empty input is "-". Results contain up to eight rows. The reply-path is
 * reserved for the ARexx text transport; native plugins use the public port.
 * Keep buffers static for a 4 KB Shell stack; target 68000 / AmigaOS 3.0+.
 */
#ifndef BEAM68PLUGIN_H
#define BEAM68PLUGIN_H
#include <exec/ports.h>

#define BEAM68_PLUGIN_PROTOCOL 1
#define BEAM68_PLUGIN_MAX_INPUT 120
#define BEAM68_PLUGIN_MAX_RESULT 120

/* VERSION=1 in every manifest. Native replies are one-way Exec messages;
 * the launcher takes ownership and frees their public memory. */
#define BEAM68_REPLY_PORT "BEAM68_REPLY"
#define BEAM68_REPLY_MAGIC 0x42363852UL /* 'B68R' */
#define BEAM68_REPLY_VERSION 1
#define BEAM68_MAX_ROWS 8
#define BEAM68_ROW_TEXT 0   /* display; Enter copies the text */
#define BEAM68_ROW_OPEN 1   /* value is a path: Enter opens it via Workbench */
#define BEAM68_ROW_QUERY 2  /* value replaces the launcher's search text */

struct Beam68ReplyRow {
    unsigned char kind;
    char text[BEAM68_PLUGIN_MAX_RESULT+1];
    char value[240];
};
struct Beam68ReplyMessage {
    struct Message message;   /* mn_ReplyPort NULL: the launcher frees it */
    unsigned long magic;
    unsigned long version;
    unsigned long serial;     /* argv[4], so late replies are ignored */
    unsigned short is_error;
    unsigned short count;
    struct Beam68ReplyRow rows[BEAM68_MAX_ROWS];
};

/* Collect rows, then send them. Delivery failure returns 20; there is no text fallback. */
void beam68_rows_begin(void);
int beam68_row(int kind, const char *text, const char *value);
int beam68_rows_send(int argc, char **argv, int is_error);

/* Decode argv[1] into `input`.  Returns 1 when argc/argv are a valid
 * five-argument native invocation; the reserved reply path is argv[2]. */
int beam68_plugin_input(int argc, char **argv, char *input, unsigned int cap);

/* Publish the reply.  Both return the exit code to return from main():
 * 0 on success, 20 when the reply could not be delivered. */
int beam68_plugin_text(int argc, char **argv, const char *text);
int beam68_plugin_error(int argc, char **argv, const char *text);

/* Small ASCII helpers (case-insensitive). */
char *beam68_trim(char *text);
int beam68_equal(const char *a, const char *b);
int beam68_prefix(const char *text, const char *prefix);
int beam68_contains(const char *text, const char *needle);
void beam68_copy(char *dst, const char *src, unsigned int cap);

/* Optional localisation through locale.library (OS 2.1+).  Open your
 * catalog once, then fetch strings by number with an English fallback:
 *     beam68_catalog_open("MyPlugin.catalog");
 *     beam68_plugin_text(argc,argv, beam68_str(1, "Hello"));
 *     beam68_catalog_close();
 * Ship the catalog in your package drawer as Catalogs/<language>/; the
 * plugin's PROGDIR: is that drawer, where locale.library looks first. */
void beam68_catalog_open(const char *catalog_name);
const char *beam68_str(long id, const char *english);
void beam68_catalog_close(void);

#endif
