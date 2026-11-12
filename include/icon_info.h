#ifndef BEAM68_ICON_INFO_H
#define BEAM68_ICON_INFO_H
/* Portable reader for the classic on-disk DiskObject (.info) layout, used
 * to recognise WHDLoad game icons without GetDiskObject(), which decodes all
 * imagery.  Layout: 78-byte DiskObject, optional 56-byte DrawerData, the
 * normal and selected Image headers + planes, then the Default Tool string
 * (ULONG length incl. NUL + bytes).  Returns the do_Type (WBTOOL=3,
 * WBPROJECT=4, ...) or 0 when the data is not a valid icon. */
#define BEAM_ICON_HEADER_BYTES 78
int beam_icon_info(const unsigned char *data, unsigned long length,
                   char *default_tool, unsigned int cap);
/* Value of tool type `key` (e.g. "SLAVE") in an icon; 1 when found. */
int beam_icon_tooltype(const unsigned char *data, unsigned long length,
                       const char *key, char *value, unsigned int cap);
/* 1 when default_tool runs WHDLoad (any path, case-insensitive). */
int beam_icon_is_whdload_tool(const char *default_tool);
#endif
