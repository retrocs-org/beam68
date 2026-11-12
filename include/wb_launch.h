#ifndef BEAM68_WB_LAUNCH_H
#define BEAM68_WB_LAUNCH_H
#include <exec/ports.h>
/* Workbench-style launch for systems whose workbench.library predates V44
 * (AmigaOS 3.0/3.1), where OpenWorkbenchObjectA() does not exist.
 *
 * Loads the tool (or a project's default tool), builds a WBStartup message
 * and starts the process exactly as Workbench would.  The WBStartup comes
 * back to reply_port when the program exits; pass every message received
 * there to beam_wb_launch_reply() so the code and locks are released.
 *
 * Returns 1 on success, 0 on failure (nothing is left allocated). */
int beam_wb_launch(struct MsgPort *reply_port, const char *path);
void beam_wb_launch_reply(struct Message *msg);
/* Number of launched programs whose WBStartup has not come back yet. */
int beam_wb_launch_outstanding(void);
#endif
