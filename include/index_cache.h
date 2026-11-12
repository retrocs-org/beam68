#ifndef BEAM68_INDEX_CACHE_H
#define BEAM68_INDEX_CACHE_H
#include "beam.h"
#include "item_index.h"
/* Optional, manually readable ASCII cache (not authoritative directory data).
 * Config fingerprint invalidates entries when any indexing setting changes.
 * Manual F2 / Beam68Ctl REINDEX catches changes made inside scanned drawers.
 *
 * The cache lives next to the launcher, NOT in ENVARC: -- the standard
 * Startup-Sequence copies all of ENVARC: into RAM:ENV on every boot, so a
 * large cache there costs both boot time and RAM.  The launcher resolves
 * PROGDIR: itself before calling these functions.
 */
#define BEAM_INDEX_CACHE_PATH "PROGDIR:Beam68.index"
/* Schema-3 locations, removed once at startup to free ENV: RAM. */
#define BEAM_INDEX_CACHE_LEGACY_ENVARC "ENVARC:Beam68/Index.txt"
#define BEAM_INDEX_CACHE_LEGACY_ENV "ENV:Beam68/Index.txt"
int beam_cache_load(const struct BeamConfig *config, struct BeamIndex *index,
                    const char *path);
int beam_cache_save(const struct BeamConfig *config, const struct BeamIndex *index,
                    const char *path);
#endif
