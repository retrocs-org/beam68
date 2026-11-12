#ifndef BEAM68_THEME_FILE_H
#define BEAM68_THEME_FILE_H
#include "beam.h"
int beam_theme_file_load(const char *path,unsigned long *colors);
int beam_theme_file_save(const char *path,const unsigned long *colors);
#endif
