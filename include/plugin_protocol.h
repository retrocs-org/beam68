#ifndef BEAM_PLUGIN_PROTOCOL_H
#define BEAM_PLUGIN_PROTOCOL_H
/* Public protocol 1 uses selectable native replies; see beam68plugin.h.
 * ARexx scripts use the atomic TEXT=/ERROR= file transport. */
#define BEAM_PROTOCOL_VERSION 1
#define BEAM_PLUGIN_MAX_INPUT 120
#define BEAM_PLUGIN_MAX_RESULT 120

/* Decode the command line's hex-encoded user input. */
int beam_decode_hex(const char *src, char *dst, unsigned int size);
#endif
