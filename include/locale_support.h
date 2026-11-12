#ifndef BEAM68_LOCALE_SUPPORT_H
#define BEAM68_LOCALE_SUPPORT_H
#include "beam68_strings.h"
/* User-interface strings via locale.library (OS 2.1+).  Without the
 * library or a catalog for the user's language, the built-in English text
 * is used.  Catalogs are looked up in PROGDIR:Catalogs/<language>/ and
 * LOCALE:Catalogs/<language>/Beam68.catalog. */
void beam_locale_open(void);
void beam_locale_close(void);
const char *beam_str(int id);
#endif
