#ifndef BEAM68_CATALOGUE_H
#define BEAM68_CATALOGUE_H
/* Plugin and theme catalogue: a plain text index published anywhere
 * (website, Aminet, a disk).  Format:
 *
 *   BEAM68CATALOGUE 1
 *   PLUGIN|org.example.weather|Weather|1.2|Ann Author|Shows the forecast|http://example.org/Beam68-Plugin-Weather-1.2.lha
 *   THEME|org.example.amber|Amber|1.0|Ann Author|Warm amber colours|http://example.org/Amber.theme
 *
 * Fields may not contain '|'.  scripts/make-catalogue.py builds an index
 * from plugin packages and theme files. */
#define BEAM_CAT_PLUGIN 1
#define BEAM_CAT_THEME 2
#define BEAM_CAT_MAX 64
struct BeamCatalogueEntry {
    unsigned char kind;
    char id[64], name[48], version[16], author[48], description[120], url[200];
};
/* Parse one index line; 1 = entry, 0 = not an entry (blank/comment/bad). */
int beam_catalogue_parse_line(const char *line, struct BeamCatalogueEntry *e);
/* Build a download command from a template with {url} and {file};
 * refuses URLs/paths with quotes, spaces or control characters. */
int beam_catalogue_command(const char *template_text, const char *url,
                           const char *file, char *out, unsigned int cap);
/* ".lha", ".lzh", ".zip" or ".theme" from the URL, else NULL. */
const char *beam_catalogue_extension(const char *url);
#endif
