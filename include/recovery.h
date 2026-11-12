#ifndef BEAM68_RECOVERY_H
#define BEAM68_RECOVERY_H

/* Allocation retry policy for the resident index. Returns zero after the
 * minimum useful 16-entry capacity has already been attempted. */
int beam_index_retry_capacity(int current);

/* Two bounded lines for the launcher's empty-result area. */
void beam_no_results_assistance(int scanning, int indexed_items,
                                char *primary, unsigned int primary_size,
                                char *secondary, unsigned int secondary_size);

#endif
