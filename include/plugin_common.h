#ifndef BEAM68_PLUGIN_COMMON_H
#define BEAM68_PLUGIN_COMMON_H
int beam_plugin_input(int argc,char **argv,char *input,unsigned int cap);
int beam_plugin_text(int argc,char **argv,const char *text);
int beam_plugin_error(int argc,char **argv,const char *text);
int beam_plugin_line(int argc,char **argv,const char *line);
int beam_ascii_equal(const char *a,const char *b);
int beam_ascii_prefix(const char *text,const char *prefix);
int beam_ascii_contains(const char *text,const char *needle);
char *beam_plugin_trim(char *text);
void beam_plugin_copy(char *dst,const char *src,unsigned int cap);
#endif
