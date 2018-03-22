#ifndef LOG_UTIL_H
#define LOG_UTIL_H

#include <stdio.h>
#include <stdarg.h>

static inline void log_default_printf(const char * file,int line,const char * func,int level,const char *format,...)
{
	va_list list;
   	va_start(list,format);
	vprintf(format,list);
	va_end(list);
}

typedef void (*LOGM_PRINTF)(const char * file,int line,const char * func,int level,const char *format,...);
extern LOGM_PRINTF g_log_print;

// #define DEBUG 1

#ifdef NO_LOGM
	#define LOGM
#else
	#ifndef LOGM
		#ifdef DEBUG
			#define LOGM(LEVEL,FMT,...) g_log_print(__FILE__,__LINE__,__FUNCTION__,LEVEL,FMT,##__VA_ARGS__);
		#else
			#define LOGM(LEVEL,FMT,...) g_log_print(NULL,0,NULL,LEVEL,FMT,##__VA_ARGS__);
		#endif
	#endif
#endif

#define LOG_VERBOSE 0x1
#define LOG_DEBUG 0x2	//调试
#define LOG_INFO 0x4	//信息
#define LOG_NOTICE 0x8 //通知
#define LOG_WARN 0x10	//警告
#define LOG_ALERT 0x20  //警报
#define LOG_ERROR 0x40  //错误
#define LOG_ASSERT 0x80 //断言
#define LOG_EMERG 0x100 //紧急
#define LOG_CRIT 0x200	//非常严重

#define LOG LOGM
#define LOGE(FMT,...) LOG(LOG_ERROR,FMT,##__VA_ARGS__)
#define LOGD(FMT,...) LOG(LOG_DEBUG,FMT,##__VA_ARGS__)
#define LOGI(FMT,...) LOG(LOG_INFO,FMT,##__VA_ARGS__)

#define LOGA(_exp,FMT,...) {if((!(_exp))) LOG(LOG_ASSERT, FMT,##__VA_ARGS__);}

#define ABORTL(FMT,...) {LOG(LOG_CRIT,"abort "#FMT,##__VA_ARGS__);abort();}

#define ABORTM(_exp) {if((_exp)) ABORTL(#_exp);}
#define ASSERTM(_exp) {if((!(_exp))) LOG(LOG_ASSERT, #_exp);}
#define WARNM(_exp) {if((_exp)) LOG(LOG_WARN, #_exp);}

#endif
