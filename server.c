#include "Core/Core.h"
#include "Event/EventActions.h"
#include "Module/Cycle.h"
#include "Module/Connection.h"
#include "Module/ngx_event_timer.h"
#include "Module/ngx_event_posted.h"

#define MAX_FD_COUNT 1024*1024

int accept_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	socket_t * so = &c->so;
	int count = 0;
	while(1){
		struct sockaddr_in addr;
		socklen_t len = sizeof(struct sockaddr_in);
		LOGD("accept %d ...\n",so->handle);
		int afd = accept(so->handle,(struct sockaddr*)&addr,&len);
		if(afd == -1)
		{
			if(count == 0)
			{
				LOGE("accept errno:%d\n",errno);
			}
			break;
		}
		count++;
		LOGD("accept client:%d\n",afd);
		close(afd);
	}
	return count;
}

int error_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	socket_t * so = &c->so;
	int ret = action_del(c->cycle->core,so);
	LOGD("error close\n");
	ASSERTM(ret == 0);
	return 0;
}

int cycle_handler(event_t *ev)
{
	cycle_t *cycle = (cycle_t*)ev->data;
	SOCKET fd = socket_bind("tcp","127.0.0.1:888");
	LOGD("socket:%d\n",fd);
	if(fd == -1){
		return -1;
	}
	int ret = listen(fd,MAX_FD_COUNT);
	if(ret == -1)
	{
		LOGE("listen errno:%d\n",errno);
		close(fd);
		return -1;
	}

	connection_t *conn = createConn(cycle,fd);
	conn->so.read = createEvent(accept_handler,conn);
	conn->so.write = NULL;
	conn->so.error = createEvent(error_handler,conn);
	ret = action_add(conn->cycle->core,&conn->so,NGX_READ_EVENT,0);
	if(ret == -1)
	{
		LOGE("action_add errno:%d\n",errno);
	}
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
