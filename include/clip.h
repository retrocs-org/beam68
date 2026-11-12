#ifndef BEAM68_CLIP_H
#define BEAM68_CLIP_H
/* Plain-text clipboard (clipboard.device unit 0, IFF FTXT/CHRS).
 * Requests that take longer than ~2 seconds are aborted. */
int beam_clip_write(const char *text);
/* Reads the first CHRS text into out (cap bytes); 1 when text was found.
 * Always finishes the read so the unit is released for other programs. */
int beam_clip_read(char *out, unsigned int cap);
#endif
