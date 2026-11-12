#ifndef BEAM68_DOS_COMPAT_H
#define BEAM68_DOS_COMPAT_H
/* SystemTagList() wrapper that works on every AmigaOS 3.x release.
 *
 * dos.library V47 (OS 3.2) accepts SYS_InName/SYS_OutName, which make the
 * child own freshly opened streams.  Older releases silently ignore those
 * tags, and an asynchronous child would then share -- and on exit close --
 * the caller's own Input()/Output().  On pre-V47 DOS this opens explicit
 * handles and passes them with SYS_Input/SYS_Output instead.
 *
 * in_name/out_name may be NULL for "NIL:".  Returns SystemTagList()'s result
 * (-1 when the command could not be started).
 */
long beam_system(const char *command, const char *in_name,
                 const char *out_name, int asynchronous);
#endif
