#ifndef BEAM68_PLUGIN_REPLY_H
#define BEAM68_PLUGIN_REPLY_H
/* Publish a complete protocol-1 TEXT=/ERROR= reply atomically on the
 * same AmigaDOS volume. For the ARexx text transport; native plugins use row messages.
 * Returns 1 on success; does not overwrite an existing reply. */
int beam_plugin_publish_reply(const char *destination, const char *line);
#endif
