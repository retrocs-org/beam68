#include "locale_support.h"

#ifdef BEAM_HOST
/* Host tests: always the built-in English text. */
void beam_locale_open(void) { }
void beam_locale_close(void) { }
const char *beam_str(int id)
{
    return (id>=1 && id<=BEAM68_STRING_COUNT)?beam68_strings[id-1]:"";
}
#else
#include <exec/types.h>
#include <libraries/locale.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/locale.h>

struct LocaleBase *LocaleBase;
static struct Catalog *catalog;

void beam_locale_open(void)
{
    struct TagItem tags[2];
    LocaleBase=(struct LocaleBase *)OpenLibrary("locale.library",38);
    if (!LocaleBase) return;
    tags[0].ti_Tag=OC_BuiltInLanguage; tags[0].ti_Data=(ULONG)"english";
    tags[1].ti_Tag=TAG_DONE; tags[1].ti_Data=0;
    catalog=OpenCatalogA(NULL,(STRPTR)"Beam68.catalog",tags);
}

void beam_locale_close(void)
{
    if (catalog) { CloseCatalog(catalog); catalog=NULL; }
    if (LocaleBase) { CloseLibrary((struct Library *)LocaleBase); LocaleBase=NULL; }
}

const char *beam_str(int id)
{
    const char *builtin=(id>=1 && id<=BEAM68_STRING_COUNT)?beam68_strings[id-1]:"";
    return catalog?(const char *)GetCatalogStr(catalog,id,(STRPTR)builtin):builtin;
}
#endif
