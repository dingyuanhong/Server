#include "Core/Core.h"
#include "Event/EventActions.h"
#include "Module/module.h"

#define MAX_FD_COUNT 1024*1024

int cicle_process(cycle_t * cycle)
{
	while(1){
		ngx_time_update();
		ngx_msec_t timeout = ngx_event_find_timer(&cycle->timeout);
		if(timeout == NGX_TIMER_INFINITE)
		{
			timeout = 10;
		}
		int ret = action_process(cycle->core,timeout);
		if(ret == -1)
		{
			break;
		}
		ngx_time_update();
		ngx_event_expire_timers(&cycle->timeout);
		safe_process_event(cycle);
		ngx_event_process_posted(&cycle->posted);

		if(cycle->connection_count == 0 && event_is_empty(cycle) && timer_is_empty(cycle))
		{
			break;
		}
	}
	return 0;
}

int connection_close_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	int ret = 0;
	ret = shutdown(c->so.handle,SHUT_RDWR);
	LOGD("shutodwn %d\n",ret);
	ret = close(c->so.handle);
	if(ret == 0)
	{
		LOGD("connection closed:%d\n",c->so.handle);
		deleteConn(&c);
		deleteEvent(&ev);
	}else{
		LOGD("connection closing:%d\n",c->so.handle);
		add_timer(c->cycle,ev,1);
	}
}

void connection_close(connection_t *c){
	event_t * timer = createEvent(connection_close_handler,c);
	add_timer(c->cycle,timer,1);
}

int read_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	while(1){
		char byte;
		size_t len = 1;
		LOGD("recv %d ...\n",c->so.handle);
		int ret = recv(c->so.handle,&byte,len,0);
		if(ret == len)
		{
			break;
		}
		else if(ret > 0)
		{
			break;
		}
		else if(ret == 0)
		{
			ret = del_connection(c);
			if(ret == 0){
				connection_close(c);
				LOGD("close ok.\n");
			}else{
				LOGD("recv %d errno:%d\n",ret,errno);
			}
			break;
		}else if(ret == -1)
		{
			if(EAGAIN == errno)
			{
				LOGD("recv again.\n");
				break;
			}
			ret = del_connection(c);
			if(ret == 0){
				connection_close(c);
				LOGD("close ok.\n");
			}else{
				LOGD("recv %d errno:%d\n",ret,errno);
			}
			break;
		}
	}
	return 1;
}

int error_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	socket_t * so = &c->so;
	int ret = del_connection(c);
	if(ret == 0)
	{
		connection_close(c);
	}else
	{
		LOGE("error close failed %d errno:%d\n",ret,errno);
	}
	return 0;
}

int cycle_handler(event_t *ev)
{
	cycle_t *cycle = (cycle_t*)ev->data;
	SOCKET fd = socket_connect("tcp","127.0.0.1:888",0);
	LOGD("socket:%d\n",fd);
	if(fd == -1){
		return -1;
	}

	connection_t *conn = createConn(cycle,fd);
	conn->so.read = createEvent(read_handler,conn);
	conn->so.write = NULL;
	conn->so.error = createEvent(error_handler,conn);
	add_connection(conn);
	return 0;
}

int main(int argc,char* argv[])
{
	os_init();
	socket_init();
	ngx_time_init();

	cycle_t *cycle = createCycle(MAX_FD_COUNT);
	ABORTM(cycle == NULL);
	ABORTM(cycle->core == NULL);
	event_t *process = createEvent(cycle_handler,cycle);
	add_event(cycle,process);
	cicle_process(cycle);
	deleteEvent(&process);
	deleteCycle(&cycle);
	return 0;
}
