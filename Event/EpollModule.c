#include "EpollModule.h"

#if (NGX_HAVE_EPOLL)

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

void epoll_module_event_handler(event_t *ev);

epoll_module_t * epoll_module_create(int concurrent)
{
	epoll_module_t *module = (epoll_module_t*)MALLOC(sizeof(epoll_module_t));
	module->handle = epoll_create(concurrent);
	ABORTI(module->handle == -1);
	// LOGD("epoll module %x\n",module);
	module->process = (event_t*)MALLOC(sizeof(event_t));
	module->process->data = module;
	module->process->handler = (event_handler_pt)epoll_module_event_handler;

	module->max_events_count = concurrent;
	module->events_count = 0;
	module->events = NULL;
	return module;
}

int epoll_module_done(epoll_module_t * module)
{
	if(module->events != NULL)
	{
		FREE(module->events);
		module->events = NULL;
	}
	if(module->process != NULL)
	{
		FREE(module->process);
		module->process = NULL;
	}
	close(module->handle);
	FREE(module);
	return 0;
}

int epoll_module_add(epoll_module_t * module,socket_t * so,int event, int flags)
{
	struct epoll_event eevent = {0};
	eevent.events  = event;
	eevent.events |= flags;
	eevent.data.ptr = (void*)so;
	// LOGD("epoll module:%x :%d socket:%d\n",module,module->handle,so->handle);
	return epoll_ctl(module->handle,EPOLL_CTL_ADD,so->handle,&eevent);
}

int epoll_module_del(epoll_module_t * module,socket_t * so)
{
	return epoll_ctl(module->handle,EPOLL_CTL_DEL,so->handle,NULL);
}

int epoll_module_process(epoll_module_t * module,int milliseconds)
{
	int events_count = module->max_events_count;
	struct epoll_event *events_ptr = (struct epoll_event *)module->events;
	if(events_ptr == NULL)
	{
		events_count = max(1,events_count);
		events_ptr = (struct epoll_event*)MALLOC(sizeof(struct epoll_event)*events_count);
		if(events_ptr == NULL)
		{
			ABORTI("memory not enough.\n");
			return -1;
		}
		module->events = events_ptr;
	}

	int n = epoll_wait(module->handle,events_ptr,events_count,milliseconds);
	if(n == 0)
	{
		return 0;
	}
	else if(n < 0)
	{
		LOGE("epoll wait errno:%d\n",errno);
		return -1;
	}
	module->events_count = min(n,events_count);
	module->process->handler(module->process);
	return module->events_count ;
}

void epoll_module_event_handler(event_t *ev)
{
	epoll_module_t * module = (epoll_module_t *)ev->data;
	ASSERT(module != NULL);
	struct epoll_event *events_ptr = (struct epoll_event *)module->events;
	ASSERT(events_ptr != NULL);
	for(int i = 0 ; i < module->events_count;i++)
	{
		struct epoll_event *event = &events_ptr[i];
		ASSERT(event != NULL);
		int events = event->events;
		socket_t *so = (socket_t *)event->data.ptr;

#ifdef EPOLLRDHUP
		if(events & EPOLLRDHUP)
		{
			LOGD("EPOLLRDHUP trigger.\n");
			so->read->handler(so->read);
		}
		else
#endif
		if(events & EPOLLHUP)
		{
			LOGD("EPOLLHUP trigger.\n");
			so->error->handler(so->error);
		}
		else
		if(events & EPOLLERR)
		{
			LOGD("EPOLLERR trigger.\n");
			so->error->handler(so->error);
		}
		else
		if(events & EPOLLPRI)
		{
			LOGD("EPOLLPRI trigger.\n");
			//带外数据
			so->error->handler(so->error);
		}
		else {
			int flags = 0;
			if(events & EPOLLET)
			{
				flags |= NGX_FLAGS_ET;
			}
			if(events & EPOLLONESHOT)
			{
				flags |= NGX_FLAGS_ONESHAOT;
			}
			if(events & EPOLLIN)
			{
				so->read->flags = flags;
				so->read->handler(so->read);
			}
			if(events & EPOLLOUT)
			{
				so->write->flags = flags;
				so->write->handler(so->write);
			}
		}
	}
}

#endif
