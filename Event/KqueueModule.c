#include "../Core/Core.h"
#include "KqueueModule.h"

#if (NGX_HAVE_KQUEUE)

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <sys/time.h>

void kqueue_module_event_handler(event_t *ev);

kqueue_module_t * kqueue_module_create(int concurrent)
{
	kqueue_module_t *module =  (kqueue_module_t*)MALLOC(sizeof(kqueue_module_t));
	module->handle = kqueue();

	module->process = (event_t*)MALLOC(sizeof(event_t));
	module->process->data = module;
	module->process->handler = (event_handler_pt)kqueue_module_event_handler;

	module->max_events_count = concurrent;
	module->events_count = 0;
	module->events = NULL;
	return module;
}

int kqueue_module_done(kqueue_module_t * module)
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

int kqueue_module_add(kqueue_module_t * module,socket_t * so,int event, int flags)
{
	return kqueue_module_set(module,so,EVFILT_READ | event,EV_ADD|EV_ENABLE|flags);
}

int kqueue_module_del(kqueue_module_t * module,socket_t * so)
{
	return kqueue_module_set(module,so,EVFILT_READ,EV_DELETE|EV_DISABLE);
}

int kqueue_module_set(kqueue_module_t * module,socket_t * so,int event, int flags)
{
	struct kevent ev;
    EV_SET(&ev, so->handle, event, flags, 0, 0, (void*)(intptr_t)so);
	int r = kevent(module->handle, &ev, 1, NULL, 0, NULL);
	return r;
}

int kqueue_module_process(kqueue_module_t * module,int milliseconds)
{
	int events_count = module->max_events_count;
	struct kevent *events_ptr = (struct kevent *)module->events;

	if(events_ptr == NULL)
	{
		events_count = max(1,events_count);
		events_ptr = (struct kevent*)MALLOC(sizeof(struct kevent)*events_count);
		if(events_ptr == NULL)
		{
			ABORTL("memory not enough.");
			return -1;
		}

		module->events = events_ptr;
	}

	struct timespec timeout_spec;
    timeout_spec.tv_sec = milliseconds / 1000;
    timeout_spec.tv_nsec = (milliseconds % 1000) * 1000 * 1000;
	int n = kevent(module->handle, NULL, 0, events_ptr, events_count, &timeout_spec);
	if(n == 0)
	{
		return 0;
	}
	else if(n < 0)
	{
		LOGE("kevent wait errno:%d\n",errno);
		return -1;
	}
	module->events_count = min(n,events_count);
	module->process->handler(module->process);
	return module->events_count;
}

void kqueue_module_event_handler(event_t *ev)
{
	kqueue_module_t * module = (kqueue_module_t *)ev->data;
	ASSERT(module != NULL);
	struct kevent *events_ptr = (struct kevent *)module->events;
	ASSERT(events_ptr != NULL);
	for(int i = 0 ; i < module->events_count;i++)
	{
		struct kevent *event_ptr = (events_ptr + i);
		ASSERT(event_ptr != NULL);

		socket_t *so = (socket_t*)event_ptr->udata;
		ASSERT(so != NULL);

		int events = event_ptr->filter;
		int flags = event_ptr->flags;

		if(flags & EV_ERROR)
		{
			if(so->error != NULL) so->error->handler(so->error);
		}
		else if(events & EVFILT_READ)
		{
			if(so->read != NULL) so->read->handler(so->read);
		}
		else if(events & EVFILT_WRITE)
		{
			if(so->write != NULL) so->write->handler(so->write);
		}
		else{
			if(so->error != NULL) so->error->handler(so->error);
		}
	}
}

#endif
