#include "Core/Core.h"
#include "Event/EventActions.h"
#include "Module/Cycle.h"
#include "Module/Connection.h"
#include "Module/ngx_event_timer.h"
#include "Module/ngx_event_posted.h"

#define MAX_FD_COUNT 1024*1024

int read_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	socket_t * so = &c->so;
	while(1){
		char byte;
		size_t len = 1;
		LOGD("recv %d ...\n",so->handle);
		int ret = recv(so->handle,&byte,len,0);
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
			ret = action_del(c->cycle->core,so);
			if(ret == 0){
				// shutdown(so->handle,SHUT_RD);
				close(so->handle);
				deleteConn(&c);
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
			ret = action_del(c->cycle->core,so);
			if(ret == 0){
				// shutdown(so->handle,SHUT_RDWR);
				close(so->handle);
				deleteConn(&c);
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
	int ret = action_del(c->cycle->core,so);
	if(ret == 0)
	{
		close(so->handle);
		deleteConn(&c);
		LOGD("error close success\n");
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

	action_add(conn->cycle->core,&conn->so,NGX_READ_EVENT,0);
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
	ngx_post_event(process,&cycle->posted);

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
		ngx_event_process_posted(&cycle->posted);
	}
	deleteEvent(&process);
	deleteCycle(&cycle);
	return 0;
}
