#ifndef BEAM68_HIDDEN_H
#define BEAM68_HIDDEN_H
#include "beam.h"
/* Results the user hid (Shift+Del): one path per line in
 * ENVARC:Beam68/Hidden.txt.  `hidden:` in the launcher lists them, and
 * Shift+Del there shows them again. */
#define BEAM_HIDDEN_PATH "ENVARC:Beam68/Hidden.txt"
#define BEAM_HIDDEN_DEFAULT 32
#define BEAM_HIDDEN_MAX 256
/* Lists are allocated for a configured size (HIDDEN_MAX / RECENT_DOCS);
 * start zeroed and call beam_list_size() before use. */
typedef char BeamPathLine[BEAM_MAX_PATH];
struct BeamHidden {
    int count, cap;
    BeamPathLine *paths;
};
int beam_hidden_size(struct BeamHidden *h, int cap);
void beam_hidden_free(struct BeamHidden *h);
void beam_hidden_load(struct BeamHidden *h, const char *file);
int beam_hidden_save(const struct BeamHidden *h, const char *file);
int beam_hidden_contains(const struct BeamHidden *h, const char *path);
/* Hide when shown, show when hidden.  Returns 1 = now hidden, 0 = now
 * shown, -1 = list full or invalid path. */
int beam_hidden_toggle(struct BeamHidden *h, const char *path);

/* Recent documents: most recent first, ENVARC:Beam68/RecentDocs.txt. */
#define BEAM_RECENT_DOCS_PATH "ENVARC:Beam68/RecentDocs.txt"
#define BEAM_RECENT_DOCS_DEFAULT 16
#define BEAM_RECENT_DOCS_MAX 64
struct BeamRecentDocs {
    int count, cap;
    BeamPathLine *paths;
};
int beam_recent_docs_size(struct BeamRecentDocs *r, int cap);
void beam_recent_docs_free(struct BeamRecentDocs *r);
void beam_recent_docs_load(struct BeamRecentDocs *r, const char *file);
int beam_recent_docs_save(const struct BeamRecentDocs *r, const char *file);
/* Move or insert path at the front. */
void beam_recent_docs_record(struct BeamRecentDocs *r, const char *path);
#endif
