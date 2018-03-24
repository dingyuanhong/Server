#include "Module/module.h"
#include "Module/slave.h"
#include "Function/echo.h"
#include <signal.h>

#define MAX_FD_COUNT 1024*1024
static cycle_t * g_signal_master = NULL;
#ifdef _WIN32
void signal_init()
{}
#else
static void signal_handle_term(int sig)
{
	LOGI("signal exit:%d",sig);
	cycle_t *cycle = g_signal_master;
	if(cycle != NULL)
	{
		if(cycle->data != NULL)
		{
			cycle_slave_t * slave = (cycle_slave_t*)cycle->data;
			slave_stop(slave);
		}
		cycle->stop = 1;
	}
}

void signal_init(){
	signal(SIGTERM , signal_handle_term);
	signal(SIGINT , signal_handle_term);
	signal(SIGQUIT , signal_handle_term);
}
#endif



int cycle_thread_post(cycle_t *cycle,SOCKET fd);

int error_event_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	int ret = connection_cycle_del(c);
	if(ret == 0)
	{
		connection_close(c);
		// LOGD("error close.\n");
	}else
	{
		LOGE("error close failed %d errno:%d\n",ret,errno);
	}
	return 0;
}

int accept_event_handler(event_t *ev)
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
				LOGE("accept errno:%d\n",errno);
			}
			break;
		}
		count++;

		// socket_nonblocking(afd);
		// connection_close(createConn(c->cycle,afd));
		cycle_thread_post(c->cycle,afd);
	}
	return count;
}

int accept_handler(event_t *ev)
{
	cycle_t *cycle = (cycle_t*)ev->data;
	SOCKET fd = socket_bind("tcp","0.0.0.0:888");
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

	socket_nonblocking(fd);
	connection_t *conn = connection_create(cycle,fd);
	conn->so.read = event_create(accept_event_handler,conn);
	conn->so.write = NULL;
	conn->so.error = event_create(error_event_handler,conn);
	ret = connection_cycle_add(conn);
	ASSERTIF(ret == 0,"action_add %d errno:%d\n",ret,errno);
	return 0;
}

void accept_connection(connection_t *conn)
{
	ASSERT(conn != NULL);
	conn->so.read = event_create(echo_read_event_handler,conn);
	conn->so.write = NULL;
	conn->so.error = event_create(error_event_handler,conn);
	int ret = connection_cycle_add(conn);
	ASSERTIF(ret == 0,"action_add %d errno:%d\n",ret,errno);
}

int connection_add_event(event_t *ev)
{
	connection_t * conn = (connection_t*)ev->data;
	accept_connection(conn);
	event_destroy(&ev);
	return 0;
}

void connection_add_event_sub(cycle_t * cycle,event_t *ev)
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
		cycle_t * cycle_sub = slave_next_cycle(slave);
		ASSERT(cycle_sub != NULL);
		safe_add_event(cycle_sub,event_create(NULL,(void*)fd),connection_add_event_sub);
	}
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

	int max_thread_count = (ngx_ncpu - 1)*2;
	if(max_thread_count > 0)
	{
		cycle->data = slave_create(MAX_FD_COUNT,max_thread_count);
	}

	g_signal_master = cycle;
	signal_init();

	event_t *process = event_create(accept_handler,cycle);
	event_add(cycle,process);
	cicle_process_master(cycle);
	event_destroy(&process);
	if(cycle->data != NULL)
	{
		cycle_slave_t * slave = (cycle_slave_t*)cycle->data;
		slave_destroy(&slave);
		cycle->data = NULL;
	}
	cycle_destroy(&cycle);
	return 0;
}
