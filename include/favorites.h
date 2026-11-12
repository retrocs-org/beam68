#ifndef BEAM_FAVORITES_H
#define BEAM_FAVORITES_H

/* Beam68 0.1: bounded ASCII favorites, no heap and no per-query disk I/O.
 * Each non-comment line in Favorites.txt contains one complete app path.
 * Keep the entry width aligned with struct BeamItem.path. */
#include "beam.h"
#define BEAM_FAVORITES_CAP 48
#define BEAM_FAVS_ENV "ENV:Beam68/Favorites.txt"
#define BEAM_FAVS_ENVARC "ENVARC:Beam68/Favorites.txt"
#define BEAM_FAVS_SESSION "T:Beam68/Favorites.txt"
#define BEAM_FAVORITE_GROUP_LEN 24
#define BEAM_FAVORITE_DEFAULT_GROUP "General"

struct BeamFavorites {
    char paths[BEAM_FAVORITES_CAP][BEAM_MAX_PATH];
    char groups[BEAM_FAVORITES_CAP][BEAM_FAVORITE_GROUP_LEN];
    unsigned short count;
};
void beam_favorites_clear(struct BeamFavorites *f);
int beam_favorites_position(const struct BeamFavorites *f,const char *path);
int beam_favorites_contains(const struct BeamFavorites *f,const char *path);
int beam_favorites_add(struct BeamFavorites *f,const char *path);
int beam_favorites_add_group(struct BeamFavorites *f,const char *path,
                             const char *group);
int beam_favorites_set_group(struct BeamFavorites *f,const char *path,
                             const char *group);
const char *beam_favorites_group(const struct BeamFavorites *f,const char *path);
int beam_favorites_in_group(const struct BeamFavorites *f,const char *path,
                            const char *group);
int beam_favorites_remove(struct BeamFavorites *f,const char *path);
/* Returns 1 if newly pinned, 0 if removed, -1 on full/invalid list. */
int beam_favorites_toggle(struct BeamFavorites *f,const char *path);
int beam_favorites_load(struct BeamFavorites *f,const char *filename);
int beam_favorites_load_active(struct BeamFavorites *f);
/* Staged .new / .bak write, never truncate existing list on write error. */
int beam_favorites_save(const struct BeamFavorites *f,const char *filename);
#endif
