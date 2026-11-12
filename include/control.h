#ifndef BEAM68_CONTROL_H
#define BEAM68_CONTROL_H
#include <exec/types.h>
#include <exec/ports.h>
#include "beam.h"

/* One-way local Exec message protocol, private to Beam68 0.1. */
#define BEAM_PORT_NAME "BEAM68.CONTROL"
#define BEAM_CONTROL_MAGIC 0x42453638UL
#define BEAM_CONTROL_VERSION 1UL
#define BEAM_CTL_RELOAD 1UL
#define BEAM_CTL_REINDEX 2UL
#define BEAM_CTL_SHOW 3UL
#define BEAM_CTL_QUIT 4UL
#define BEAM_CTL_HIDE 5UL
#define BEAM_CTL_RELOAD_PLUGINS 6UL
#define BEAM_CTL_PREVIEW_THEME 7UL

struct BeamControlMessage {
    struct Message message;
    ULONG magic, version, command;
};

struct BeamThemeControlMessage {
    struct BeamControlMessage control;
    ULONG theme;
    ULONG custom_colors[BEAM_THEME_COLOR_COUNT];
};

int beam_control_send(unsigned long command);
int beam_control_send_theme(unsigned long theme,
                            const unsigned long *custom_colors);
int beam_control_running(void);
#endif
