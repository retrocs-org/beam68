#ifndef BEAM68_CONTROL_H
#define BEAM68_CONTROL_H
#include <exec/types.h>
#include <exec/ports.h>
#include "beam.h"

/* Private local Exec control protocol. Plugin Protocol 1 is independent. */
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
#define BEAM_CTL_QUERY_VERSION 8UL
#define BEAM_VERSION_REPLY_PREFIX "BEAM68.VERSION."

struct BeamControlMessage {
    struct Message message;
    ULONG magic, version, command;
};

struct BeamThemeControlMessage {
    struct BeamControlMessage control;
    ULONG theme;
    ULONG custom_colors[BEAM_THEME_COLOR_COUNT];
};

struct BeamVersionRequest {
    struct BeamControlMessage control;
    ULONG serial;
    char reply_port[64];
};
struct BeamVersionReply {
    struct Message message;
    ULONG magic,version,serial;
    char launcher_version[16];
};

/* About-only query: at most 25 ticks. 1 reported, 0 stopped, -1 unavailable.
 * Replies use a named port, never a borrowed pointer that can outlive timeout. */
int beam_control_version(char *out,unsigned int cap);
void beam_control_reply_version(const struct BeamControlMessage *request);
int beam_control_send(unsigned long command);
int beam_control_send_theme(unsigned long theme,
                            const unsigned long *custom_colors);
int beam_control_running(void);
#endif
