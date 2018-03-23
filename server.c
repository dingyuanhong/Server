#include "Core/Core.h"
#include "Event/EventActions.h"
#include "Module/module.h"
#include "Module/slave.h"
#include "Function/echo.h"
#include <signal.h>

#define MAX_FD_COUNT 1024*1024
static cycle_t * g_signal_master = NULL;
#ifdef _WIN32
void processSignal()
{}
#else
static void handle_signal_term(int sig)
{
	LOGI("signal exit:%d",sig);
	cycle_t *cycle = g_signal_master;
	if(cycle != NULL)
	{
		if(cycle->data != NULL)
		{
			cycle_slave_t * slave = (cycle_slave_t*)cycle->data;
			stop_slave(slave);
		}
		cycle->stop = 1;
	}
}

void processSignal(){
	signal(SIGTERM , handle_signal_term);
	signal(SIGINT , handle_signal_term);
	signal(SIGQUIT , handle_signal_term);
}
#endif

int cycle_thread_post(cycle_t *cycle,SOCKET fd);

int connection_close_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	int ret = 0;
	ret = socket_linger(c->so.handle,1,0);//直接关闭SOCKET，避免TIME_WAIT
	ABORTIF(ret != 0,"socket_linger %d\n",ret);
	// ret = shutdown(c->so.handle,SHUT_WR);
	// ABORTIF(ret != 0,"shutodwn %d\n",ret);
	ret = close(c->so.handle);
	if(ret == 0)
	{
		LOGD("connection closed:%d\n",c->so.handle);
		deleteConn(&c);
		deleteEvent(&ev);
	}else{
		LOGD("connection closing:%d\n",c->so.handle);
		add_event(c->cycle,ev);
	}
	return 0;
}

void connection_close(connection_t *c)
{
	event_t * timer = createEvent(connection_close_handler,c);
	add_event(c->cycle,timer);
}

int error_event_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	int ret = del_connection(c);
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
	connection_t *conn = createConn(cycle,fd);
	conn->so.read = createEvent(accept_event_handler,conn);
	conn->so.write = NULL;
	conn->so.error = createEvent(error_event_handler,conn);
	ret = add_connection(conn);
	ASSERTIF(ret == 0,"action_add %d errno:%d\n",ret,errno);
	return 0;
}

void accept_connection(connection_t *conn)
{
	ASSERT(conn != NULL);
	conn->so.read = createEvent(echo_read_event_handler,conn);
	conn->so.write = NULL;
	conn->so.error = createEvent(error_event_handler,conn);
	int ret = add_connection(conn);
	ASSERTIF(ret == 0,"action_add %d errno:%d\n",ret,errno);
}

int connection_add_event(event_t *ev)
{
	connection_t * conn = (connection_t*)ev->data;
	accept_connection(conn);
	deleteEvent(&ev);
	return 0;
}

void connection_add_event_sub(cycle_t * cycle,event_t *ev)
{
	SOCKET fd = (SOCKET)ev->data;
	connection_t * conn = createConn(cycle,fd);
	accept_connection(conn);
	deleteEvent(&ev);
}

int cycle_thread_post(cycle_t *cycle,SOCKET fd)
{
	if(cycle->data == NULL)
	{
		add_event(cycle,createEvent(connection_add_event,createConn(cycle,fd)));
	}else{
		cycle_slave_t * slave = cycle->data;
		cycle_t * cycle_sub = cycle_slave_next(slave);
		ASSERT(cycle_sub != NULL);
		safe_add_event(cycle_sub,createEvent(NULL,(void*)fd),connection_add_event_sub);
	}
	return 0;
}

int main(int argc,char* argv[])
{
	os_init();
	socket_init();
	ngx_time_init();

	cycle_t *cycle = createCycle(MAX_FD_COUNT);
	ABORTI(cycle == NULL);
	ABORTI(cycle->core == NULL);

	int max_thread_count = (ngx_ncpu - 1)*2;
	if(max_thread_count > 0)
	{
		cycle->data = create_slave(MAX_FD_COUNT,max_thread_count);
	}

	g_signal_master = cycle;
	processSignal();

	event_t *process = createEvent(accept_handler,cycle);
	add_event(cycle,process);
	cicle_process_master(cycle);
	deleteEvent(&process);
	if(cycle->data != NULL)
	{
		cycle_slave_t * slave = (cycle_slave_t*)cycle->data;
		delete_slave(&slave);
		cycle->data = NULL;
	}
	deleteCycle(&cycle);
	return 0;
}
