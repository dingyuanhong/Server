#include "Module/module.h"
#include "Module/slave.h"
#include "Function/connection_close.h"
#include "Function/echo.h"
#include "Function/signal.h"
#include "Function/service.h"


int heartbeat_event_handler(event_t *ev)
{
	char byte[65535];
	int len = 65535;
	connection_t *c = (connection_t*)ev->data;
	int ret = buffer_write(c,byte,len);
	if(ret != -1)
	{
		timer_add(c->cycle,c->so.write,1000);
	}
	return 0;
}

int discard_read_event_handler(event_t *ev)
{
	char byte[65535];
	int len = 65535;
	connection_t *c = (connection_t*)ev->data;
	while(1)
	{
		int ret = buffer_read(c,byte,len);
		if(ret <= 0)
		{
			break;
		}
		if(ret == len)
		{
			continue;
		}
		break;
	}
	return 0;
}

void control_init(connection_t * c)
{
	ASSERT(c != NULL);
	c->so.write = event_create(heartbeat_event_handler,c);
	c->so.read = event_create(discard_read_event_handler,c);
}

#define MAX_FD_COUNT 1024*1024

static int error_event_handler(event_t *ev)
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
	control_init(conn);
	if(conn->so.read == NULL) conn->so.read = event_create(error_event_handler,conn);
	if(conn->so.error == NULL) conn->so.error = event_create(error_event_handler,conn);
	int ret = connection_cycle_add_(conn,NGX_READ_EVENT|NGX_WRITE_EVENT,0);
	ASSERT(ret == 0);
	timer_add(conn->cycle,conn->so.write,1000);
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
