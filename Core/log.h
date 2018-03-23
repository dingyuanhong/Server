#ifndef LOG_UTIL_H
#define LOG_UTIL_H

#include <stdio.h>
#include <stdarg.h>

typedef void (*LOG_PRINTF)(const char * file,int line,const char * func,int level,const char *format,...);
extern LOG_PRINTF G_LOG_PRINTF;
LOG_PRINTF set_log_printf(LOG_PRINTF log);

#ifdef NO_LOG
	#define LOG
#else
	#ifndef LOG
		#ifdef DEBUG
			#define LOG(LEVEL,FMT,...) G_LOG_PRINTF(__FILE__,__LINE__,__FUNCTION__,LEVEL,FMT,##__VA_ARGS__);
		#else
			#define LOG(LEVEL,FMT,...) G_LOG_PRINTF(NULL,0,NULL,LEVEL,FMT,##__VA_ARGS__);
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
#define LOG_ABORT 0x400	//停止
#
#define LOGE(FMT,...) LOG(LOG_ERROR,FMT,##__VA_ARGS__)
#define LOGD(FMT,...) LOG(LOG_DEBUG,FMT,##__VA_ARGS__)
#define LOGI(FMT,...) LOG(LOG_INFO,FMT,##__VA_ARGS__)

#define LOGA(_exp,FMT,...) if((!(_exp))){ LOG(LOG_ASSERT, FMT,##__VA_ARGS__);}

#define ABORT abort
#define ABORTL(FMT,...) {LOG(LOG_ABORT,FMT,##__VA_ARGS__);ABORT();}
#define ABORTI(_exp) if((_exp)){ ABORTL(#_exp);}
#define ABORTIF(_exp,FMT,...) if((_exp)){ ABORTL(FMT,##__VA_ARGS__);}

#define ASSERT(_exp) if((!(_exp))){ LOG(LOG_ASSERT, #_exp);}
#define ASSERTIF(_exp,FMT,...) if((!(_exp))){ LOG(LOG_ASSERT,FMT,##__VA_ARGS__);}
#define WARN(_exp) if((_exp)){ LOG(LOG_WARN, #_exp);}
#define WARNIF(_exp) if((_exp)){ LOG(LOG_WARN, #_exp);}

#endif