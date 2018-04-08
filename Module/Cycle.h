#ifndef CYCLE_H
#define CYCLE_H

#include "../Event/EventActions.h"
#include "ngx_event_timer.h"

struct cycle_s;

typedef void (*cycle_func)(struct cycle_s* cycle);

typedef struct cycle_ptr{
	cycle_func init;
	cycle_func step;
	cycle_func over;
	cycle_func closing;
	cycle_func end;
}cycle_ptr;

typedef struct cycle_s{
	core_t * core;
	int stop;
	int32_t  index;
	int32_t  master;

	ngx_queue_t connection_queue;
	uint32_t connection_count;
	ngx_queue_t internal_posted;

	ngx_rbtree_t timeout;
	ngx_queue_t posted;

	ngx_queue_t async_posted;
	ngx_atomic_t async_posted_lock;
	ngx_atomic_t async_posted_count;

	void * data;
	cycle_ptr * ptr;
}cycle_t;

static inline cycle_t * cycle_create(int concurrent,cycle_ptr * ptr)
{
	cycle_t * cycle = MALLOC(sizeof(cycle_t));
	cycle->stop = 0;
	cycle->index = -1;
	cycle->master = 1;

	cycle->core = action_create(concurrent);
	ngx_queue_init(&cycle->connection_queue);
	cycle->connection_count = 0;

	ngx_queue_init(&cycle->internal_posted);

	ngx_event_timer_init(&cycle->timeout);
	ngx_queue_init(&cycle->posted);
	ngx_queue_init(&cycle->async_posted);
	cycle->async_posted_lock = 0;
	cycle->async_posted_count = 0;
	
	cycle->data = NULL;
	cycle->ptr = ptr;
	return cycle;
}

static inline void cycle_destroy(cycle_t ** cycle_ptr)
{
	if(cycle_ptr != NULL)
	{
		if(*cycle_ptr != NULL)
		{
			cycle_t * cycle = *cycle_ptr;
			cycle->stop = 1;
			action_done(cycle->core);
			FREE(cycle);
			*cycle_ptr = NULL;
		}
	}
}

static inline void cycle_process_init(cycle_t * cycle)
{
	if(cycle != NULL && cycle->ptr != NULL && cycle->ptr->init != NULL)
	{
		cycle->ptr->init(cycle);
	}
}

static inline void cycle_process_step(cycle_t * cycle)
{
	if(cycle != NULL && cycle->ptr != NULL && cycle->ptr->step != NULL)
	{
		cycle->ptr->step(cycle);
	}
}

static inline void cycle_process_over(cycle_t * cycle)
{
	if(cycle != NULL && cycle->ptr != NULL && cycle->ptr->over != NULL)
	{
		cycle->ptr->over(cycle);
	}
}

static inline void cycle_process_closing(cycle_t * cycle)
{
	if(cycle != NULL && cycle->ptr != NULL && cycle->ptr->closing != NULL)
	{
		cycle->ptr->closing(cycle);
	}
}

static inline void cycle_process_end(cycle_t * cycle)
{
	if(cycle != NULL && cycle->ptr != NULL && cycle->ptr->end != NULL)
	{
		cycle->ptr->end(cycle);
	}
}
#endif
