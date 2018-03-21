#ifndef CYCLE_H
#define CYCLE_H

#include "../Core/Core.h"
#include "../Event/EventActions.h"
#include "ngx_event_timer.h"

typedef struct cycle_s{
	core_t * core;
	ngx_rbtree_t timeout;
	ngx_queue_t posted;
}cycle_t;

inline cycle_t * createCycle(int concurrent)
{
	cycle_t * cycle = MALLOC(sizeof(cycle_t));
	cycle->core = action_create(concurrent);
	ngx_event_timer_init(&cycle->timeout);
	ngx_queue_init(&cycle->posted);
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
