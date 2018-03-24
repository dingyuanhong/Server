#ifndef TYPE_UTIL_H
#define TYPE_UTIL_H

#include "time_util.h"

#if (NGX_HAVE_SCHED_YIELD)
#define ngx_sched_yield()  sched_yield()
#else
#ifdef _WIN32
#define ngx_sched_yield()  SwitchToThread()
#elif __linux__
#include <pthread.h>
#define ngx_sched_yield()  pthread_yield()
#else
#define ngx_sched_yield()  usleep(1)
#endif
#endif

#endif
