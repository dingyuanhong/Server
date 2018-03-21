#ifndef EPOLL_module_H
#define EPOLL_module_H

#include "Socket.h"
#include "Event.h"

typedef struct epoll_module_s
{
	int handle;
	event_t * process;

	int max_events_count;
	int events_count;
	void * events;
}epoll_module_t;

#ifdef __linux__
#define NGX_HAVE_EPOLL 1
#endif

#if (NGX_HAVE_EPOLL)

#include <sys/epoll.h>

#define NGX_READ_EVENT EPOLLIN
#define NGX_WRITE_EVENT EPOLLOUT

#define NGX_FLAGS_ERROR ( EPOLLERR | EPOLLHUP )
#define NGX_FLAGS_ONESHAOT EPOLLONESHOT
#define NGX_FLAGS_OOB EPOLLPRI

#define NGX_FLAGS_ET EPOLLET

epoll_module_t * epoll_module_create(int concurrent);
int epoll_module_done(epoll_module_t * module);
int epoll_module_add(epoll_module_t * module,socket_t * so,int event, int flags);
int epoll_module_del(epoll_module_t * module,socket_t * so);
int epoll_module_process(epoll_module_t * module,int milliseconds);

#endif

#endif
