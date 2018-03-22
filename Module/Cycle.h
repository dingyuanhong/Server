#ifndef CYCLE_H
#define CYCLE_H

#include "../Core/Core.h"
#include "../Event/EventActions.h"
#include "ngx_event_timer.h"

typedef struct cycle_s{
	core_t * core;
	uint32_t connection_count;

	ngx_rbtree_t timeout;
	ngx_queue_t posted;

	ngx_queue_t accept_posted;
	ngx_atomic_t accept_posted_lock;
	ngx_atomic_t accept_posted_index;
}cycle_t;

inline cycle_t * createCycle(int concurrent)
{
	cycle_t * cycle = MALLOC(sizeof(cycle_t));
	cycle->core = action_create(concurrent);
	ngx_event_timer_init(&cycle->timeout);
	ngx_queue_init(&cycle->posted);
	ngx_queue_init(&cycle->accept_posted);
	cycle->accept_posted_lock = 0;
	cycle->accept_posted_index = 0;
	cycle->connection_count = 0;
	return cycle;
}

inline void deleteCycle(cycle_t ** cycle)
{
	if(cycle != NULL)
	{
		if(*cycle != NULL)
		{
			action_done((*cycle)->core);
			FREE(*cycle);
		}
		*cycle = NULL;
	}
}



#endif
