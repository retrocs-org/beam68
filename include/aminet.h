#ifndef BEAM68_AMINET_H
#define BEAM68_AMINET_H
#include <stdio.h>
#define BEAM_AMINET_PATH 97
#define BEAM_AMINET_QUERY 81
#define BEAM_AMINET_ROWS 6
struct BeamAminetPackage { char path[BEAM_AMINET_PATH]; char title[121]; };
struct BeamAminetResults { struct BeamAminetPackage rows[BEAM_AMINET_ROWS]; int count,seen; };
struct BeamAminetDetails { char summary[121],version[81],author[121],architecture[121],requires[121]; };
int beam_aminet_archive_header(const char *package,const unsigned char *data,unsigned int size);
int beam_aminet_path_valid(const char *path);
int beam_aminet_encode_query(const char *query,char *out,unsigned int cap);
int beam_aminet_readme_path(const char *package,char *out,unsigned int cap);
int beam_aminet_search(FILE *file,int skip,struct BeamAminetResults *results);
int beam_aminet_details(FILE *file,struct BeamAminetDetails *details);
void beam_aminet_plain(const char *text,char *out,unsigned int cap);
/* Validate a complete HTTP header. -1 means no content length. */
int beam_aminet_http_header(const char *header,unsigned long limit,long *length);
#endif
