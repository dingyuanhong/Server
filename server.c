#include "Module/module.h"
#include "Module/slave.h"
#include "Function/echo.h"
#include "Function/signal.h"
#include "Function/service.h"

#define MAX_FD_COUNT 1024*1024

int cycle_thread_post(cycle_t *cycle,SOCKET fd);

void accept_event_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	int count = 0;
	while(1){
		struct sockaddr_in addr;
		socklen_t len = sizeof(struct sockaddr_in);
		// LOGD("accept %d ...\n",c->so.handle);
		SOCKET afd = accept(c->so.handle,(struct sockaddr*)&addr,&len);
		if(afd == -1)
		{
			if(count == 0)
			{
				LOGE("accept errno:%d\n",_ERRNO);
			}
			break;
		}
		// socket_nonblocking(afd);
		cycle_thread_post(c->cycle,afd);
		count++;
		if(count >= 1000)
		{
			break;
		}
	}
}

void accept_handler(event_t *ev)
{
	cycle_t *cycle = (cycle_t*)ev->data;
	event_destroy(&ev);
	SOCKET fd = socket_bind("tcp","0.0.0.0:888");
	if(fd == -1){
		return ;
	}
	int ret = listen(fd,MAX_FD_COUNT);
	if(ret == -1)
	{
		LOGE("listen errno:%d\n",_ERRNO);
		close(fd);
		return ;
	}

	socket_nonblocking(fd);
	connection_t *conn = connection_create(cycle,fd);
	conn->so.read = event_create(accept_event_handler,conn);
	conn->so.write = NULL;
	conn->so.error = event_create(connection_error_handle,conn);
	ret = connection_cycle_add(conn);
	ASSERTIF(ret == 0,"action_add %d errno:%d\n",ret,errno);
}

void accept_connection(connection_t *conn)
{
	ASSERT(conn != NULL);
	service_init(conn);
	// if(conn->so.read == NULL) conn->so.read = event_create(connection_error_handle,conn);
	// if(conn->so.error == NULL) conn->so.error = event_create(connection_error_handle,conn);
#ifdef NGX_FLAGS_ET
	int ret = connection_cycle_add_(conn,NGX_READ_EVENT,NGX_FLAGS_ET);
#else
	int ret = connection_cycle_add(conn);
#endif
	ASSERTIF(ret == 0,"action_add %d errno:%d\n",ret,errno);
}

void connection_add_event(event_t *ev)
{
	connection_t * conn = (connection_t*)ev->data;
	accept_connection(conn);
	event_destroy(&ev);
}

void slave_connection_add_event(cycle_t * cycle,event_t *ev)
{
	SOCKET fd = (SOCKET)ev->data;
	connection_t * conn = connection_create(cycle,fd);
	accept_connection(conn);
	event_destroy(&ev);
}

int cycle_thread_post(cycle_t *cycle,SOCKET fd)
{
	if(cycle->data == NULL)
	{
		event_add(cycle,event_create(connection_add_event,connection_create(cycle,fd)));
	}else{
		cycle_slave_t * slave = cycle->data;
		cycle_t * slave_cycle = slave_next_cycle(slave);
		ASSERT(slave_cycle != NULL);
		safe_add_event(slave_cycle,event_create(NULL,(void*)fd),slave_connection_add_event);
	}
	return 0;
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

	os_init();
	socket_init();
	ngx_time_init();

	cycle_t *cycle = cycle_create(MAX_FD_COUNT);
	ABORTI(cycle == NULL);
	ABORTI(cycle->core == NULL);
	cycle->index = 0;
	int max_thread_count = (ngx_ncpu - 1)*2;
	if(max_thread_count > 0)
	{
		cycle->data = slave_create(MAX_FD_COUNT,max_thread_count);
	}
	signal_init(cycle);

	event_t *process = event_create(accept_handler,cycle);
	event_add(cycle,process);
	cycle_process(cycle);

	if(cycle->data != NULL)
	{
		cycle_slave_t * slave = (cycle_slave_t*)cycle->data;
		slave_destroy(&slave);
		cycle->data = NULL;
	}
	cycle_destroy(&cycle);
	return 0;
}
