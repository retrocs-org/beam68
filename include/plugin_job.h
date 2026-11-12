#ifndef BEAM68_PLUGIN_JOB_H
#define BEAM68_PLUGIN_JOB_H

/* Small, portable state/validation layer for a single asynchronous plugin.
 * The process and timer lifecycle are owned by the Amiga-specific launcher.
 * Public protocol 1 supports native row messages and ARexx text replies.
 */
#define BEAM_PLUGIN_TICKS_PER_SECOND 20 /* 50ms scheduling tick */
#define BEAM_PLUGIN_WAIT_TICKS 600 /* default 30 seconds */

struct BeamPluginJob {
    unsigned int age;
    unsigned int timeout_ticks;
    int active;
};

void beam_job_begin(struct BeamPluginJob *job);
/* Configurable 5..120 second timeout. Invalid input falls back to 30. */
void beam_job_begin_timeout(struct BeamPluginJob *job, int seconds);
void beam_job_end(struct BeamPluginJob *job);
/* 1 when expired; zero while running. */
int beam_job_tick(struct BeamPluginJob *job);
unsigned long beam_job_elapsed_ms(const struct BeamPluginJob *job);
/* Build a Shell-safe hex-only argument, or '-' for empty input.
 * Returns 0 on invalid input / insufficient output capacity. */
int beam_plugin_hex_argument(const char *input, char *out, unsigned int cap);
/* Returns 1 for a complete valid reply, 0 if there is no complete line yet,
 * or -1 for an invalid reply. Result excludes TEXT=/ERROR= and newline. */
int beam_plugin_parse_reply(const char *line, char *result, unsigned int cap,
                            int *is_error);

/* Construct the complete command, including protocol-1 reply routing, before
 * checking capacity. Paths are quoted and reject Shell escape characters. */
int beam_plugin_command(const char *program,const char *hex,const char *result,
                        int rexx,const char *port,unsigned long serial,
                        char *out,unsigned int cap);

#endif
