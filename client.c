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
	}else{
		LOGD("connection closing:%d\n",c->so.handle);
		add_event(c->cycle,ev);
	}
	return 0;
}

void connection_close(connection_t *c){
	ASSERT(c->so.error != NULL);
	del_event_conn(c);
	del_timer_conn(c);
	c->so.error->handler = connection_close_handler;
	add_event(c->cycle,c->so.error);
}

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
			ret = del_connection(c);
			if(ret == 0){
				connection_close(c);
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
			}else{
				LOGD("recv %d errno:%d\n",ret,errno);
			}
			break;
		}
	}
	return 1;
}

int error_event_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	int ret = del_connection(c);
	if(ret == 0)
	{
		connection_close(c);
		LOGD("error close.\n");
	}else
	{
		LOGE("error close failed %d errno:%d\n",ret,errno);
	}
	return 0;
}

int cycle_handler(event_t *ev)
{
	cycle_t *cycle = (cycle_t*)ev->data;
	SOCKET fd = socket_connect("tcp","10.0.2.6:888",1);
	if(fd == -1){
		add_event(cycle,ev);
		return -1;
	}
	LOGD("socket connect %d\n",fd);
	connection_t *conn = createConn(cycle,fd);
	conn->so.read = createEvent(read_event_handler,conn);
	conn->so.write = createEvent(echo_write_event_handler,conn);
	conn->so.error = createEvent(error_event_handler,conn);
	int ret = add_connection_event(conn,NGX_READ_EVENT|NGX_WRITE_EVENT,0);
	ASSERT(ret == 0);
	add_timer(cycle,conn->so.write,500);

	add_event(cycle,ev);
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

	g_signal_master = cycle;
	processSignal();

	event_t *process = createEvent(cycle_handler,cycle);
	add_event(cycle,process);
	cicle_process_master(cycle);
	deleteEvent(&process);
	deleteCycle(&cycle);
	return 0;
}
