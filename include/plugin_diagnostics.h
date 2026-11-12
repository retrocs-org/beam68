#ifndef BEAM68_PLUGIN_DIAGNOSTICS_H
#define BEAM68_PLUGIN_DIAGNOSTICS_H
#define BEAM_PLUGIN_OK 1
#define BEAM_PLUGIN_DISABLED 2
#define BEAM_PLUGIN_MANIFEST_ERROR 3
#define BEAM_PLUGIN_EXECUTABLE_MISSING 4
#define BEAM_PLUGIN_PATH_ERROR 5
#define BEAM_PLUGIN_CONFLICT 6
#define BEAM_PLUGIN_QUARANTINED 7
const char *beam_plugin_diagnostic(int status);
const char *beam_plugin_advice(int status);
/* Writes a compact, bounded human-readable record. 1 if successful. */
int beam_plugin_diagnostic_line(int one_based_index, int state,
    char trigger, const char *manifest, char *out, unsigned int capacity);
#endif
