#ifndef BEAM68_ITEM_INDEX_H
#define BEAM68_ITEM_INDEX_H
#include "beam.h"

/* Compact resident index for 68000 machines.
 *
 * Paths live in one string pool instead of a fixed 240-byte array per item,
 * and the displayed name is a pointer into the path (FilePart), so an item
 * costs about 20 bytes plus its actual path length.  A small open-addressing
 * hash makes duplicate detection O(1) during a scan instead of O(n).
 *
 * Rescans are mark-and-sweep: items under the rescanned roots are marked
 * stale, re-found items are refreshed in place, and only when the scan
 * completes are vanished items swept out.  The live index therefore never
 * becomes empty while a background rescan is running.
 */
#define BEAM_POOL_BYTES_PER_ITEM 56
#define BEAM_ROOT_NONE 0xFF
#define BEAM_ROOT_SOURCES 30  /* REXX:, bookmarks, snippets */
#define BEAM_ITEM_FLAG_STALE 1
#define BEAM_ITEM_FLAG_HIDDEN 2  /* hidden by the user (Shift+Del) */

struct BeamIndex {
    struct BeamItem *items;
    int count;
    int capacity;
    int full;               /* an add was refused: capacity or pool exhausted */
    /* The path pool grows on demand; borrowed string pointers may move
       on add/title, as well as on sweep. Item indices remain stable on add. */
    char *pool;
    unsigned long pool_used;
    unsigned long pool_size;
    unsigned short *hash;   /* item index + 1, 0 = empty slot */
    unsigned int hash_mask; /* hash size - 1 (power of two) */
};

#ifdef BEAM_INDEX_TEST
/* Host tests only; -1 disables deterministic allocation failure injection. */
void beam_index_test_fail_after(long count);
#endif
int beam_index_reconcile(struct BeamIndex *index,int complete,int *tracked,unsigned int tracked_count);
int beam_index_init(struct BeamIndex *x, int capacity);
void beam_index_free(struct BeamIndex *x);
void beam_index_clear(struct BeamIndex *x);
int beam_index_find(const struct BeamIndex *x, const char *path);
/* Returns the item index (new or refreshed existing), -1 when full,
 * -2 for an invalid path.  *created is set when a new item was appended. */
int beam_index_add(struct BeamIndex *x, const char *path, int kind, int root,
                   int *created);
/* Add or replace a title. Allocation failure preserves the old index and
 * title; text remains adjacent to its path. Returns 1 if set. */
int beam_index_set_title(struct BeamIndex *x, int index, const char *title);
/* Mark items as stale: those whose root bit is set in root_mask, plus
 * volume items when include_volumes is non-zero. */
void beam_index_mark_stale(struct BeamIndex *x, unsigned long root_mask,
                           int include_volumes);
/* Case-insensitive equal/descendant test with drawer boundaries. */
int beam_index_path_within(const char *path,const char *drawer);
/* Mark descendants only, keeping the drawer record and all siblings. */
void beam_index_mark_subtree(struct BeamIndex *x,const char *drawer);
/* Drop stale items, compact the pool, rebuild the hash.  Returns removed. */
/* Remove one path without dropping stale entries of an unfinished scan. */
int beam_index_remove(struct BeamIndex *x,const char *path,int *tracked,unsigned int tracked_count);
int beam_index_sweep(struct BeamIndex *x);
/* Display name for a path: the part after the last '/' or ':', or the whole
 * path for a volume root such as "RAM:". */
const char *beam_index_name_for(const char *path);
#endif
