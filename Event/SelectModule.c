#include "../Core/Core.h"
#include "SelectModule.h"

#if (NGX_HAVE_SELECT)

void select_module_event_handler(event_t *ev);

select_module_t * select_module_create(int concurrent)
{
	select_module_t *module = (select_module_t*)MALLOC(sizeof(select_module_t));
	module->max_handle = 0;

	FD_ZERO(&module->read_set_cache);
	FD_ZERO(&module->write_set_cache);
	FD_ZERO(&module->except_set_cache);

	FD_ZERO(&module->read_set);
	FD_ZERO(&module->write_set);
	FD_ZERO(&module->except_set);

	module->concurrent = concurrent;
	module->events_index = 0;
	module->events_count = 0;
	int size = sizeof(event_t*)*concurrent*3;
	module->events = MALLOC(size);
	MEMSET(module->events,0,size);

	module->process = (event_t*)MALLOC(sizeof(event_t));
	module->process->data = module;
	module->process->handler = (event_handler_pt)select_module_event_handler;
	return module;
}

int select_module_done(select_module_t * module)
{
	if(module->process != NULL)
	{
		FREE(module->process);
		module->process = NULL;
	}
	if(module->events != NULL)
	{
		FREE(module->events);
		module->events = NULL;
	}
	FREE(module);
	return 0;
}

int select_module_add(select_module_t * module,socket_t * so,int event, int flags)
{
	if((event & NGX_READ_EVENT) == NGX_READ_EVENT)
	{
		if(so->read->index == EVENT_INVALID_INDEX)
		{
			FD_SET(so->handle, &module->read_set_cache);
			module->events[module->events_index] = so->read;
			so->read->index = module->events_index;
			module->events_index++;
		}else{
			LOGE("select_module_add so->read->index:%d\n",so->read->index);
		}
	}
	if((event & NGX_WRITE_EVENT) == NGX_WRITE_EVENT)
	{
		if(so->write->index == EVENT_INVALID_INDEX)
		{
			FD_SET(so->handle, &module->write_set_cache);
			module->events[module->events_index] = so->write;
			so->write->index = module->events_index;
			module->events_index++;
		}else{
			LOGE("select_module_add so->write->index:%d\n",so->write->index);
		}
	}
	if((event & NGX_ERROR_EVENT) == NGX_ERROR_EVENT)
	{
		if(so->error->index == EVENT_INVALID_INDEX)
		{
			FD_SET(so->handle, &module->except_set_cache);
			module->events[module->events_index] = so->error;
			so->error->index = module->events_index;
			module->events_index++;
		}else{
			LOGE("select_module_add so->error->index:%d\n",so->error->index);
		}
	}
	if(module->max_handle <= so->handle)
	{
		module->max_handle = so->handle;
	}
	return 0;
}

int select_module_del(select_module_t * module,socket_t * so)
{
	int event = NGX_READ_EVENT | NGX_WRITE_EVENT | NGX_ERROR_EVENT;
	if((event & NGX_READ_EVENT) == NGX_READ_EVENT)
	{
		if(so->read->index != EVENT_INVALID_INDEX)
		{
			FD_CLR(so->handle, &module->read_set_cache);
			int index = so->read->index;
			so->read->index = EVENT_INVALID_INDEX;
			if(index == module->events_index-1)
			{
				module->events[index] = module->events[module->events_index-1];
				module->events[index]->index = index;
			}
			module->events_index--;
		}
	}
	if((event & NGX_WRITE_EVENT) == NGX_WRITE_EVENT)
	{
		if(so->write->index != EVENT_INVALID_INDEX)
		{
			FD_CLR(so->handle, &module->write_set_cache);
			int index = so->write->index;
			so->write->index = EVENT_INVALID_INDEX;
			if(index == module->events_index-1)
			{
				module->events[index] = module->events[module->events_index-1];
				module->events[index]->index = index;
			}
			module->events_index--;
		}
	}
	if((event & NGX_ERROR_EVENT) == NGX_ERROR_EVENT)
	{
		if(so->error->index != EVENT_INVALID_INDEX)
		{
			FD_CLR(so->handle, &module->except_set_cache);
			int index = so->error->index;
			so->error->index = EVENT_INVALID_INDEX;
			if(index == module->events_index-1)
			{
				module->events[index] = module->events[module->events_index-1];
				module->events[index]->index = index;
			}
			module->events_index--;
		}
	}
	return 0;
}

int select_module_process(select_module_t * module,int milliseconds)
{
	module->read_set = module->read_set_cache;
  	module->write_set = module->write_set_cache;
  	module->except_set = module->except_set_cache;

	struct timeval timeout_spec;
    timeout_spec.tv_sec = milliseconds / 1000;
    timeout_spec.tv_usec = (milliseconds % 1000) * 1000;
	int n = select(module->max_handle + 1, &module->read_set, &module->write_set, &module->except_set, &timeout_spec);
	if(n == 0)
	{
		return 0;
	}
	else if(n < 0)
	{
		LOGE("select wait errno:%d\n",errno);
		return -1;
	}
	module->events_count = n;
	module->process->handler(module->process);
	return module->events_count ;
}

void select_module_event_handler(event_t *ev)
{
	ASSERT(ev != NULL);
	select_module_t * module = (select_module_t *)ev->data;
	ASSERT(module != NULL);
	int events_cur_count = module->events_index;
	for(int i = 0;i < events_cur_count;i++)
	{
		event_t * event = module->events[i];
		if(event == NULL)
		{
			continue;
		}
		socket_t *so = (socket_t*)event->data;
		ASSERT(so != NULL);
		if(so->read == event)
		{
			if(FD_ISSET(so->handle,&module->read_set)){
				so->read->handler(so->read);
				module->events_count--;
			}
		}else if(so->write == event){
			if(FD_ISSET(so->handle,&module->write_set)){
				so->write->handler(so->write);
				module->events_count--;
			}
		}else if(so->error == event){
			if(FD_ISSET(so->handle,&module->except_set)){
				so->error->handler(so->error);
				module->events_count--;
			}
		}
		if(module->events_count <= 0) break;
	}
}

#endif
