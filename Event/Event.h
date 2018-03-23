#ifndef EVENT_CORE_H
#define EVENT_CORE_H

#include "../Core/Core.h"

typedef struct event_s event_t;

typedef int (*event_handler_pt)(event_t *ev);

typedef struct event_s{
	event_handler_pt  handler;
	void * data;
	int  index;
	int flags;

	unsigned		 posted:1;
	unsigned         timedout:1;
    unsigned         timer_set:1;
	unsigned		 cancelable:1;

	ngx_rbtree_node_t   timer;
    ngx_queue_t      queue;
}event_t;

#define EVENT_INVALID_INDEX  0xFFFFFFFF


#include "../Core/Core.h"

inline void initEvent(event_t *event,event_handler_pt handler,void* data)
{
	memset(event,0,sizeof(event_t));
	event->data = data;
	event->handler = handler;
	event->index = EVENT_INVALID_INDEX;
	ngx_queue_init(&event->queue);

	// event->flags = 0;
	//
	// event->posted = 0;
	// event->timedout = 0;
	// event->timer_set = 0;
	// event->cancelable = 0;
}

inline event_t * createEvent(event_handler_pt handler,void* data)
{
	event_t * event = MALLOC(sizeof(event_t));
	initEvent(event,handler,data);
	return event;
}

inline void deleteEvent(event_t** event){
	if(event != NULL)
	{
		if(*event != NULL)
		{
			FREE(*event);
		}
		*event = NULL;
	}
}

#endif
