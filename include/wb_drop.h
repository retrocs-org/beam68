#ifndef BEAM_WB_DROP_H
#define BEAM_WB_DROP_H
/* Workbench drop filtering is independent of the Amiga libraries so that
 * rules can be tested on a build host. Paths must already be resolved from
 * AppMessage WBArg locks by the native caller. */
#define BEAM_DROP_DIRECTORY 1
#define BEAM_DROP_PLUGIN 2
#define BEAM_DROP_FAVORITE 3
#define BEAM_DROP_THEME 4
int beam_drop_candidate(const char *absolute_path, int is_directory,
                        int target, char *output, unsigned int capacity);
#endif
