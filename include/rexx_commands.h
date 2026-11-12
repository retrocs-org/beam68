#ifndef BEAM68_REXX_COMMANDS_H
#define BEAM68_REXX_COMMANDS_H
/* Pure, bounded command parser. Port transport is implemented in main.c.
 * This interface is an application command set, NOT the plugin ABI. */
#define BEAM_REXX_PORT "BEAM68"
#define BEAM_REXX_SHOW 1
#define BEAM_REXX_HIDE 2
#define BEAM_REXX_RELOAD 3
#define BEAM_REXX_REINDEX 4
#define BEAM_REXX_QUIT 5
#define BEAM_REXX_STATUS 6
#define BEAM_REXX_VERSION 7
#define BEAM_REXX_HELP 8
#define BEAM_REXX_PLUGINS 9
#define BEAM_REXX_PLUGIN 10
#define BEAM_REXX_INDEX 11
#define BEAM_REXX_RELOAD_PLUGINS 12
#define BEAM_REXX_TIMING 13
struct BeamRexxCommand { int action; int plugin_index; };
/* Return 1 on recognized syntax, 0 on invalid/unknown syntax. A plugin
 * index is zero-based internally but one-based in the script interface. */
int beam_rexx_parse(const char *input, struct BeamRexxCommand *out);
#endif
