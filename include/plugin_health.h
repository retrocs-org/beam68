#ifndef BEAM68_PLUGIN_HEALTH_H
#define BEAM68_PLUGIN_HEALTH_H
#include "beam.h"
#define BEAM_PLUGIN_HEALTH_PATH "ENVARC:Beam68/PluginHealth.txt"
#define BEAM_PLUGIN_HEALTH_SUCCESS 1
#define BEAM_PLUGIN_HEALTH_ERROR 2
#define BEAM_PLUGIN_HEALTH_TIMEOUT 3
#define BEAM_PLUGIN_QUARANTINE_THRESHOLD 3
struct BeamPluginHealthEntry {
    char path[BEAM_MAX_PATH];
    unsigned long successes,errors,timeouts;
    unsigned char last,consecutive_failures,quarantined;
};
struct BeamPluginHealth {
    struct BeamPluginHealthEntry entries[BEAM_MAX_PLUGINS];
    unsigned short count,pending;
};
void beam_plugin_health_reset(struct BeamPluginHealth *h);
const struct BeamPluginHealthEntry *beam_plugin_health_find(
    const struct BeamPluginHealth *h,const char *path);
void beam_plugin_health_record(struct BeamPluginHealth *h,const char *path,int outcome);
int beam_plugin_health_is_quarantined(const struct BeamPluginHealth *h,const char *path);
int beam_plugin_health_clear_quarantine(struct BeamPluginHealth *h,const char *path);
int beam_plugin_health_format(const struct BeamPluginHealthEntry *e,char *out,unsigned int cap);
int beam_plugin_health_load(struct BeamPluginHealth *h,const char *path);
int beam_plugin_health_save(struct BeamPluginHealth *h,const char *path);
#endif
