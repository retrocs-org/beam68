#ifndef BEAM_HISTORY_H
#define BEAM_HISTORY_H
/* Fixed bounded history; intentionally small for 68000/low RAM Workbench. */
#define BEAM_HISTORY_CAP 48
#define BEAM_HISTORY_PATH "ENVARC:Beam68/History.txt"
struct BeamHistoryEntry {
    char path[240];
    unsigned long launches;
};
struct BeamHistory {
    struct BeamHistoryEntry entries[BEAM_HISTORY_CAP];
    unsigned short count;
    unsigned short pending;
};
void beam_history_reset(struct BeamHistory *h);
int beam_history_load(struct BeamHistory *h,const char *path);
unsigned int beam_history_bonus(const struct BeamHistory *h,const char *path);
/* Zero is the most recently launched item; -1 means not present. */
int beam_history_recent_position(const struct BeamHistory *h,const char *path);
void beam_history_record(struct BeamHistory *h,const char *path);
int beam_history_save(struct BeamHistory *h,const char *path);
#endif
