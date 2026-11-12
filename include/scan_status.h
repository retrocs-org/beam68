#ifndef BEAM68_SCAN_STATUS_H
#define BEAM68_SCAN_STATUS_H
/* File-system counters, reported without allocating buffers during indexing. */
struct BeamScanStatus {
    unsigned long roots_done, roots_total, entries, applications;
    unsigned long unreadable, io_errors, overlong;
    int scanning, capped, cached;
};
/* Persist only complete, error-free scans. A cap hit is NOT complete. */
int beam_scan_cacheable(const struct BeamScanStatus *s);
/* Text is ASCII, single line, bounded. Returns 0 if the buffer is too small. */
int beam_scan_status_format(const struct BeamScanStatus *s, char *out,
                            unsigned int capacity);
/* Compact launcher-facing progress/result text. */
int beam_scan_progress_format(const struct BeamScanStatus *s, char *out,
                              unsigned int capacity);
#endif
