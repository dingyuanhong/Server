#ifndef select_module_H
#define select_module_H

#define NGX_HAVE_SELECT 1

#if (NGX_HAVE_SELECT)

#ifndef NGX_READ_EVENT
#define NGX_READ_EVENT 0
#endif
#ifndef NGX_WRITE_EVENT
#define NGX_WRITE_EVENT 1
#endif
#ifndef NGX_ERROR_EVENT
#define NGX_ERROR_EVENT 2
#endif

#ifndef NGX_FLAGS_ERROR
#define NGX_FLAGS_ERROR 2
#endif
#ifndef NGX_FLAGS_ONESHAOT
#define NGX_FLAGS_ONESHAOT 4
#endif
#ifndef NGX_FLAGS_OOB
#define NGX_FLAGS_OOB 8
#endif

#ifdef _WIN32
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#define FD_SETSIZE  1024
#endif
#include <Winsock2.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#endif

#include "Event.h"
#include "Socket.h"

typedef struct select_module_s
{
	event_t * process;
	int max_handle;
	fd_set read_set_cache;
	fd_set write_set_cache;
	fd_set except_set_cache;

	fd_set read_set;
	fd_set write_set;
	fd_set except_set;

	int concurrent;
	int events_index;
	int events_count;
	event_t **events;
}select_module_t;

select_module_t * select_module_create(int concurrent);
int select_module_done(select_module_t * module);
int select_module_add(select_module_t * module,socket_t * obj,int event, int flags);
int select_module_del(select_module_t * module,socket_t * obj);
int select_module_process(select_module_t * module,int milliseconds);

#endif

#endif
