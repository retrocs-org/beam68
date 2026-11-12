#ifndef BEAM_MATCHER_H
#define BEAM_MATCHER_H
/* ASCII-only ranking, portable to 68000 and testable on a host. */
int beam_match_score(const char *needle, const char *haystack);
/* Case-folded character-presence mask: bits 0..25 letters, 26 digits,
 * 27 any other non-separator byte.  Used for a sound early reject: every
 * match class tolerates at most ONE distinct needle character that is absent
 * from the haystack (only the one-edit typo class tolerates any). */
unsigned long beam_text_mask(const char *s);
/* Positions (bit per character, first 32) of haystack that a query
 * matched, for highlighting.  0 when nothing matched. */
unsigned long beam_match_marks(const char *needle, const char *haystack);
#endif
