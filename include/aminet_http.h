#ifndef BEAM68_AMINET_HTTP_H
#define BEAM68_AMINET_HTTP_H
/* progress returns zero to cancel. HTTP/1.0, identity body, fixed Aminet host. */
int beam_aminet_fetch(const char *path,const char *destination,unsigned long limit,
                      unsigned long seconds,int require_length,int (*progress)(unsigned long),
                      char *error,unsigned int error_cap);
#endif
