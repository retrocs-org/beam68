#ifndef BEAM68_LAUNCHER_UI_H
#define BEAM68_LAUNCHER_UI_H
/* Portable, allocation-free helpers for the AmigaOS Intuition launcher UI.
 * The cursor is a byte offset: Beam68's original matcher is ASCII-only. */
struct BeamLatency { unsigned long last, maximum, samples; };
/* Saturating counters keep diagnostics bounded even in long resident runs. */
unsigned long beam_elapsed_ms(unsigned long start_seconds,unsigned long start_micros,
                              unsigned long end_seconds,unsigned long end_micros);
unsigned long beam_elapsed_us(unsigned long start_seconds,unsigned long start_micros,
                              unsigned long end_seconds,unsigned long end_micros);
void beam_latency_record(struct BeamLatency *metric,unsigned long micros);
enum BeamInputKind { BEAM_INPUT_TEXT, BEAM_INPUT_RAW, BEAM_INPUT_OTHER };
/* Ignore raw key releases and unmapped keys when coalescing typed input.
 * Actions which consume the selected result must flush the pending search. */
int beam_input_flush_search(int kind,unsigned int code,int shift);
enum BeamEditOp {
    BEAM_EDIT_INSERT=1,
    BEAM_EDIT_BACKSPACE,
    BEAM_EDIT_DELETE,
    BEAM_EDIT_LEFT,
    BEAM_EDIT_RIGHT,
    BEAM_EDIT_HOME,
    BEAM_EDIT_END,
    BEAM_EDIT_CLEAR
};
int beam_query_edit(char *text, unsigned int capacity, int *cursor,
                    int operation, char value);
/* Result navigation: arrows wrap, control jumps to an edge, shift moves a page. */
int beam_result_navigate(int current, int total, int direction,
                         int shift, int control, int page_size);
int beam_result_rows(int row_top, int lineheight, int usable_bottom,
                     int maximum);
int beam_result_scroll(int selected, int first, int row_count, int total);
int beam_hit_result(int y, int row_top, int lineheight,
                    int usable_bottom, int result_count);
/* Word-wrap a byte string at a font-measured fit; returns bytes drawn. */
unsigned int beam_text_line(const char *text,unsigned int fit,unsigned int *next);
#endif
