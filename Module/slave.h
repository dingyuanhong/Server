#ifndef SLAVE_H
#define SLAVE_H

#include "../Core/thread.h"
#include "../Event/EventActions.h"
#include "module.h"


typedef struct cycle_slave_s{
	int concurrent;
	int max_cycle_count;
	int cycle_pool_index;
	ngx_array_t *cycle_pool;
	ngx_array_t *thread_pool;
}cycle_slave_t;

inline void *ngx_array_get(ngx_array_t *a, ngx_uint_t n)
{
	if(a->nalloc > n){
		return (u_char *) a->elts + a->size * n;
	}
	return NULL;
}

inline cycle_slave_t *slave_create(int concurrent,int thread_count){
	cycle_slave_t * slave = (cycle_slave_t*)MALLOC(sizeof(cycle_slave_t));
	slave->concurrent = concurrent;
	slave->max_cycle_count = thread_count;
	slave->cycle_pool_index = 0;
	slave->cycle_pool = ngx_array_create(thread_count,sizeof(cycle_t*));
	slave->thread_pool = ngx_array_create(thread_count,sizeof(uv_thread_t));
	return slave;
}

inline void slave_destroy(cycle_slave_t**slave_ptr){
	ASSERT(slave_ptr != NULL);
	cycle_slave_t * slave = *slave_ptr;
	ASSERT(slave != NULL);
	for(int i = 0 ; i < slave->max_cycle_count;i++)
	{
		cycle_t ** cycle_ptr = ngx_array_get(slave->cycle_pool,i);
		if(cycle_ptr != NULL && *cycle_ptr != NULL)
		{
			(*cycle_ptr)->stop = 1;
		}
		uv_thread_t * thread_id = (uv_thread_t*)ngx_array_get(slave->thread_pool,i);
		if(thread_id != NULL && *thread_id != 0)
		{
			uv_thread_join(thread_id);
		}
		cycle_destroy(cycle_ptr);
	}
	ngx_array_destroy(slave->cycle_pool);
	slave->cycle_pool = NULL;
	ngx_array_destroy(slave->thread_pool);
	slave->thread_pool = NULL;
	FREE(slave);
	*slave_ptr = NULL;
}

void slave_stop(cycle_slave_t*slave)
{
	ASSERT(slave != NULL);
	for(int i = 0 ; i < slave->max_cycle_count;i++)
	{
		cycle_t ** cycle_ptr = ngx_array_get(slave->cycle_pool,i);
		if(cycle_ptr != NULL && *cycle_ptr != NULL)
		{
			(*cycle_ptr)->stop = 1;
		}
	}
}

void slave_wait_stop(cycle_slave_t*slave)
{
	ASSERT(slave != NULL);
	for(int i = 0 ; i < slave->max_cycle_count;i++)
	{
		cycle_t ** cycle_ptr = ngx_array_get(slave->cycle_pool,i);
		if(cycle_ptr != NULL && *cycle_ptr != NULL)
		{
			(*cycle_ptr)->stop = 1;
		}
		uv_thread_t * thread_id = (uv_thread_t*)ngx_array_get(slave->thread_pool,i);
		if(thread_id != NULL && *thread_id != 0)
		{
			uv_thread_join(thread_id);
		}
	}
}

static inline void slave_thread_cb(void* arg)
{
	cycle_t *cycle = (cycle_t*)arg;
	cycle_process_slave(cycle);
}

inline cycle_t * slave_next_cycle(cycle_slave_t *slave)
{
	int index = slave->cycle_pool_index%slave->max_cycle_count;
	cycle_t ** cycle_ptr = (cycle_t**)ngx_array_get(slave->cycle_pool,index);
	ABORTI(cycle_ptr == NULL);
	if(*cycle_ptr == NULL)
	{
		cycle_t *cycle = cycle_create(slave->concurrent);
		ABORTI(cycle == NULL);
		ABORTI(cycle->core == NULL);
		cycle->index = index/2 + 1;
		*cycle_ptr = cycle;

		//启动线程
		uv_thread_t * thread_id = (uv_thread_t*)ngx_array_get(slave->thread_pool,index);
		int ret = uv_thread_create(thread_id,slave_thread_cb,cycle);
		ABORTI(ret != 0);
	}
	ABORTI(*cycle_ptr == NULL);
	slave->cycle_pool_index++;
	return *cycle_ptr;
}


#endif
