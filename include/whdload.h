#ifndef BEAM68_WHDLOAD_H
#define BEAM68_WHDLOAD_H
/* Portable readers for WHDLoad installs (host-tested).
 *
 * A slave is a HUNK executable whose first hunk starts with the slave
 * structure: ws_Security(4) "WHDLOADS"(8) ws_Version(2) ... and, from
 * version 10, relative pointers to the game name, copyright and info texts
 * (offsets 36/38/40) and, from version 16, the Kickstart image name (42). */
struct BeamSlaveInfo {
    unsigned int version;
    char name[64];
    char copyright[64];
    char kick[48];       /* e.g. "kick34005.A500", empty when none */
};
/* data = the first bytes of the .slave file.  Returns 1 for a slave. */
int beam_slave_info(const unsigned char *data, unsigned long length,
                    struct BeamSlaveInfo *info);
/* "Name (copyright)" or just the name; empty when the slave has none. */
void beam_slave_title(const struct BeamSlaveInfo *info, char *out, unsigned int cap);
#endif
