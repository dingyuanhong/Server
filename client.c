#include "Module/module.h"
#include "Module/slave.h"
#include "Function/connection_close.h"
#include "Function/echo.h"
#include "Function/signal.h"

#define MAX_FD_COUNT 1024*1024

int read_event_handler(event_t *ev)
{
	char byte[65536];
	size_t len = 65536;

	connection_t *c = (connection_t*)ev->data;
	while(1){
		LOGD("recv %d ...\n",c->so.handle);
		int ret = recv(c->so.handle,&byte,len,0);
		if(ret == len)
		{
			continue;
		}
		else if(ret > 0)
		{
			break;
		}
		else if(ret == 0)
		{
			connection_remove(c);

			break;
		}else if(ret == -1)
		{
			if(EAGAIN == errno)
			{
				LOGD("recv again.\n");
				break;
			}
			connection_remove(c);
			break;
		}
	}
	return 1;
}

int error_event_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	connection_remove(c);
	return 0;
}

int cycle_handler(event_t *ev)
{
	cycle_t *cycle = (cycle_t*)ev->data;
	SOCKET fd = socket_connect("tcp","10.0.2.6:888",1);
	if(fd == -1){
		event_add(cycle,ev);
		return -1;
	}
	LOGD("socket connect %d\n",fd);
	connection_t *conn = connection_create(cycle,fd);
	conn->so.read = event_create(read_event_handler,conn);
	conn->so.write = event_create(echo_write_event_handler,conn);
	conn->so.error = event_create(error_event_handler,conn);
	int ret = connection_cycle_add_(conn,NGX_READ_EVENT|NGX_WRITE_EVENT,0);
	ASSERT(ret == 0);
	timer_add(cycle,conn->so.write,500);

	event_add(cycle,ev);
	return 0;
}

int main(int argc,char* argv[])
{
	os_init();
	socket_init();
	ngx_time_init();

	cycle_t *cycle = cycle_create(MAX_FD_COUNT);
	ABORTI(cycle == NULL);
	ABORTI(cycle->core == NULL);
	signal_init(cycle);

	event_t *process = event_create(cycle_handler,cycle);
	event_add(cycle,process);
	cicle_process_master(cycle);
	event_destroy(&process);
	cycle_destroy(&cycle);
	return 0;
}
