#include "log.h"

static inline void log_default_printf(const char * file,int line,const char * func,int level,const char *format,...)
{
	va_list list;
   	va_start(list,format);
	vprintf(format,list);
	va_end(list);
}

LOG_PRINTF G_LOG_PRINTF = log_default_printf;

LOG_PRINTF set_log_printf(LOG_PRINTF log)
{
	LOG_PRINTF old = G_LOG_PRINTF;
	G_LOG_PRINTF = log;
	return old;
}
