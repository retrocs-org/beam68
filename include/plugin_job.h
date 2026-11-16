#ifndef BEAM68_PLUGIN_JOB_H
#define BEAM68_PLUGIN_JOB_H

/* Small, portable state/validation layer for a single asynchronous plugin.
 * The process and timer lifecycle are owned by the Amiga-specific launcher.
 * Public protocol 1 supports native row messages and ARexx text replies.
 */
#define BEAM_PLUGIN_TICKS_PER_SECOND 20 /* 50ms scheduling tick */
#define BEAM_PLUGIN_WAIT_TICKS 600 /* default 30 seconds */

#define BEAM_REQUEST_NAME_MAX 64
#define BEAM_PLUGIN_REQUEST_LIMIT 3
#define BEAM_PLUGIN_PREVIEW_LIMIT 2
/* Independent of the visible job: cancellation/timeout does not release a slot. */
struct BeamPluginRequests { unsigned long serial[BEAM_PLUGIN_REQUEST_LIMIT]; };
int beam_request_format(unsigned int slot,unsigned long serial,const char *name,
                         int preview,int file_reply,int active,unsigned long age_ms,
                         unsigned long timeout_ms,char *out,unsigned int capacity);
unsigned int beam_requests_count(const struct BeamPluginRequests *requests);
int beam_requests_can_start(const struct BeamPluginRequests *requests,int preview);
int beam_requests_find(const struct BeamPluginRequests *requests,unsigned long serial);
int beam_requests_reserve(struct BeamPluginRequests *requests,unsigned long serial,int preview);
int beam_requests_complete(struct BeamPluginRequests *requests,unsigned long serial);

/* Known bundled read-only requests skip the SYSTEM double-Enter guard.
 * Music playback and unknown SYSTEM plugins retain confirmation. */
int beam_plugin_needs_confirmation(const char *id,const char *input,int system);

struct BeamPluginJob {
    unsigned int age;
    unsigned int timeout_ticks;
    int active;
};

void beam_job_begin(struct BeamPluginJob *job);
/* Configurable 5..120 second timeout. Invalid input falls back to 30. */
void beam_job_begin_timeout(struct BeamPluginJob *job, int seconds);
/* Elapsed-time deadlines do not stretch when event-loop ticks are delayed. */
int beam_job_elapsed(struct BeamPluginJob *job,unsigned long milliseconds);
int beam_job_accept_reply(const struct BeamPluginJob *job,unsigned long expected,
                          unsigned long received,int same_query,int visible);
/* Validate a protocol-1 row without reading past either fixed buffer. */
int beam_job_validate_row(unsigned int kind,const char *text,unsigned int text_cap,
                          const char *value,unsigned int value_cap);
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
