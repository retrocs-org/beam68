#ifndef BEAM_ALIASES_H
#define BEAM_ALIASES_H

#include "beam.h"

#define BEAM_ALIAS_CAP 64
#define BEAM_ALIAS_LEN 96
#define BEAM_ALIAS_ACTION_LABEL_LEN 32
#define BEAM_ALIAS_ACTION_COMMAND_LEN 240
#define BEAM_ALIASES_ENV "ENV:Beam68/Aliases.txt"
#define BEAM_ALIASES_ENVARC "ENVARC:Beam68/Aliases.txt"
#define BEAM_ALIASES_SESSION "T:Beam68/Aliases.txt"
#define BEAM_ALIASES_DEFAULT "PROGDIR:Aliases.txt"

struct BeamAlias {
    char path[BEAM_MAX_PATH];
    char words[BEAM_ALIAS_LEN];
    /* Optional per-application action. Both fields are empty or both set. */
    char action_label[BEAM_ALIAS_ACTION_LABEL_LEN];
    char action_command[BEAM_ALIAS_ACTION_COMMAND_LEN];
};

/* Entries are allocated on demand and grow in steps of 8 (608 bytes each),
 * so a system with few or no aliases does not keep a 39 KB table resident.
 * A BeamAliases must start zeroed (static storage or memset). */
struct BeamAliases {
    struct BeamAlias *entries;
    unsigned short count;
    unsigned short capacity;
};
void beam_aliases_free(struct BeamAliases *a);

void beam_aliases_clear(struct BeamAliases *a);
int beam_aliases_find(const struct BeamAliases *a,const char *path);
const char *beam_aliases_words(const struct BeamAliases *a,int index);
int beam_aliases_set(struct BeamAliases *a,const char *path,const char *words);
int beam_aliases_set_full(struct BeamAliases *a,const char *path,
                          const char *words,const char *action_label,
                          const char *action_command);
int beam_aliases_remove(struct BeamAliases *a,int index);
const char *beam_alias_action_label(const struct BeamAliases *a,int index);
const char *beam_alias_action_command(const struct BeamAliases *a,int index);
int beam_aliases_load(struct BeamAliases *a,const char *filename);
int beam_aliases_load_active(struct BeamAliases *a);
int beam_aliases_save(const struct BeamAliases *a,const char *filename);

#endif
