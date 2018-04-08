#include "Module/module.h"
#include "Module/slave.h"
#include "Function/echo.h"
#include "Function/signal.h"
#include "Function/service.h"


void heartbeat_event_handler(event_t *ev)
{
	char byte[65535];
	int len = 10;
	connection_t *c = (connection_t*)ev->data;
	int ret = buffer_write(c,byte,len);
	if(ret != -1)
	{
		timer_add(c->cycle,c->so.write,1000);
	}else
	{
		event_add(c->cycle,c->so.read);
	}
}

void discard_read_event_handler(event_t *ev)
{
	char byte[65535];
	int len = 65535;
	connection_t *c = (connection_t*)ev->data;
	while(1)
	{
		int ret = buffer_read(c,byte,len);
		if(ret < 0)
		{
			break;
		}
		else if(ret == len)
		{
			continue;
		}else if(ret == 0)
		{
			timer_add(c->cycle,c->so.read,10);
		}
		break;
	}
}

void control_init(connection_t * c)
{
	ASSERT(c != NULL);
	c->so.write = event_create(heartbeat_event_handler,c);
	c->so.read = event_create(discard_read_event_handler,c);
	c->so.error = event_create(connection_error_handle,c);
}

#define MAX_FD_COUNT 1024*1024
char * url = "127.0.0.1:888";
int blocking = 0;
int max_connection_count = 0;

#define GET_PARAM(PARAM,I)	if(argc >= I+1) PARAM = argv[I];
#define GET_PARAM_INT(PARAM,I)	if(argc >= I+1) PARAM = atoi(argv[I]);

void cycle_handler(event_t *ev)
{
	cycle_t *cycle = (cycle_t*)ev->data;

	if(max_connection_count > 0 && cycle->connection_count >= max_connection_count)
	{
		LOGD("connection count:%d\n",cycle->connection_count);
		timer_add(cycle,ev,10000);
		return;
	}

	SOCKET fd = socket_connect("tcp",url,blocking);
	if(fd == -1){
		if(_ERRNO == _ERROR(EADDRNOTAVAIL))
		{
			return ;
		}
		timer_add(cycle,ev,1000);
		return;
	}
	if(!blocking)
	{
		socket_nonblocking(fd);
	}
	// LOGD("socket connect %d\n",fd);
	connection_t *conn = connection_create(cycle,fd);
	control_init(conn);
	int ret = 0;
	connection_cycle_queue_add(conn);
	// ret = connection_cycle_add_(conn,NGX_READ_EVENT,0);
	ASSERT(ret == 0);
	if(ret != 0)
	{
		connection_clear(conn);
	}else{
		timer_add(conn->cycle,conn->so.write,1000);
	}
	event_add(cycle,ev);
}

void print()
{
	LOGD("ngx_rbtree_node_t:%d\n",sizeof(ngx_rbtree_node_t));
	LOGD("ngx_queue_t:%d\n",sizeof(ngx_queue_t));
	LOGD("event_t:%d\n",sizeof(event_t));
	LOGD("socket_t:%d\n",sizeof(socket_t));
	LOGD("cycle_t:%d\n",sizeof(cycle_t));
	LOGD("cycle_slave_t:%d\n",sizeof(cycle_slave_t));
	LOGD("connection_t:%d\n",sizeof(connection_t));
	LOGD("loopqueue_t:%d\n",sizeof(loopqueue_t));
}

int main(int argc,char* argv[])
{
	print();

	GET_PARAM(url,1);
	GET_PARAM_INT(blocking,2);
	GET_PARAM_INT(max_connection_count,3);

	os_init();
	socket_init();
	ngx_time_init();

	cycle_t *cycle = cycle_create(MAX_FD_COUNT,NULL);
	ABORTI(cycle == NULL);
	ABORTI(cycle->core == NULL);
	signal_init(cycle);

	event_t *process = event_create(cycle_handler,cycle);
	event_add(cycle,process);
	cycle_process(cycle);
	event_destroy(&process);
	cycle_destroy(&cycle);
	return 0;
}
