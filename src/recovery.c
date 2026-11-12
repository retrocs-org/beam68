#include "recovery.h"
#include "locale_support.h"

static void copy_text(char *dest, const char *src, unsigned int capacity)
{
    unsigned int i=0;
    if (!dest || !capacity) return;
    while (src[i] && i+1<capacity) { dest[i]=src[i]; ++i; }
    dest[i]=0;
}

int beam_index_retry_capacity(int current)
{
    int next;
    if (current<=16) return 0;
    next=current/2;
    if (next<16) next=16;
    return next;
}

void beam_no_results_assistance(int scanning, int indexed_items,
                                char *primary, unsigned int primary_size,
                                char *secondary, unsigned int secondary_size)
{
    if (scanning) {
        copy_text(primary,beam_str(MSG_NORESULT_SCANNING1),
                  primary_size);
        copy_text(secondary,beam_str(MSG_NORESULT_SCANNING2),
                  secondary_size);
    } else if (indexed_items<=0) {
        copy_text(primary,beam_str(MSG_NORESULT_EMPTY1),primary_size);
        copy_text(secondary,beam_str(MSG_NORESULT_EMPTY2),
                  secondary_size);
    } else {
        copy_text(primary,beam_str(MSG_NORESULT_NONE1),primary_size);
        copy_text(secondary,beam_str(MSG_NORESULT_NONE2),
                  secondary_size);
    }
}
