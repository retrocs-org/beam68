#ifndef BEAM68_PLUGIN_PACKAGE_H
#define BEAM68_PLUGIN_PACKAGE_H
#include "beam.h"
/* Portable validation for third-party plugin packages (host-testable).
 *
 * A package is one drawer containing <Name>.plugin, the executable and
 * optional files, distributed as an LHA or ZIP archive or as a plain drawer.
 * Packages must be self-contained: COMMAND and ICON use PLUGINDIR: (ICON
 * may also be PROGDIR:Beam68, the launcher's own icon), and
 * SETTINGS (if any) is a flat file in ENVARC:Beam68/Plugins/ so it survives
 * upgrades and uninstalls. */
#define BEAM_ARCHIVE_NONE 0
#define BEAM_ARCHIVE_LHA 1
#define BEAM_ARCHIVE_ZIP 2

int beam_package_archive_kind(const char *path);
int beam_package_id_valid(const char *id);
#define BEAM_PKG_OK 0
#define BEAM_PKG_NO_MANIFEST 1
#define BEAM_PKG_BAD_ID 2
#define BEAM_PKG_NO_VERSION 3
#define BEAM_PKG_TOO_NEW 4
#define BEAM_PKG_BAD_COMMAND 5
#define BEAM_PKG_BAD_ICON 6
#define BEAM_PKG_BAD_GUIDE 7
#define BEAM_PKG_BAD_SETTINGS 8
int beam_package_manifest_problem(const struct BeamPlugin *p);
/* Returns 1 when the manifest is acceptable as a package; otherwise 0 with
 * a short human-readable reason in err. */
int beam_package_manifest_check(const struct BeamPlugin *p,
                                char *err, unsigned int cap);
/* One entry name from "unzip -Z1": rejects absolute, device-qualified and
 * parent-relative names ("/x", "Work:x", "../x", "a/../b", "a//b"). */
int beam_package_zip_name_safe(const char *name);
/* One line of "lha l" output.  The listing format is not fixed, so this is
 * a conservative screen for parent references only. */
int beam_package_lha_line_safe(const char *line);
/* Drawer names used under Plugins/: letters, digits, '_' '-' '.', 1..30. */
int beam_package_drawer_name_valid(const char *name);

/* LHA/LZH directory reader (no decompression).  Parses one header at the
 * start of buf (avail bytes).  Header levels 0, 1 and 2 are supported.
 * Returns:  >0  header bytes consumed (the entry's data follows: *skip
 *               bytes; next header at offset + return + *skip)
 *            0  end of archive (a zero byte)
 *           -1  invalid or unsupported header
 *           -2  need more bytes (header is longer than avail)
 * The entry's full name (directory + file, '/'-separated) goes to name. */
#define BEAM_LHA_HEADER_MAX 1024
long beam_lha_header(const unsigned char *buf, unsigned long avail,
                     char *name, unsigned int cap, unsigned long *skip);
/* One ZIP central-directory record. Rejects encrypted/special-file entries,
 * unsafe names and oversized data before invoking the external extractor. */
struct BeamZipEntry {
    unsigned long packed,unpacked,offset,crc;
    unsigned int flags,method;
};
long beam_zip_header(const unsigned char *buf,unsigned long avail,char *name,
                     unsigned int cap,struct BeamZipEntry *entry);
#endif
