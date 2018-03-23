#include "Core/Core.h"
#include "Event/EventActions.h"
#include "Module/module.h"

#define MAX_FD_COUNT 1024*1024

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

void connection_close(connection_t *c){
	event_t * timer = createEvent(connection_close_handler,c);
	add_event(c->cycle,timer);
}

int read_event_handler(event_t *ev)
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
		add_timer(cycle,ev,1);
		return -1;
	}
	LOGD("socket connect %d\n",fd);
	connection_t *conn = createConn(cycle,fd);
	conn->so.read = createEvent(error_event_handler,conn);
	conn->so.write = createEvent(error_event_handler,conn);
	conn->so.error = createEvent(error_event_handler,conn);
	int ret = add_connection_event(conn,NGX_READ_EVENT|NGX_WRITE_EVENT,0);
	ASSERT(ret == 0);

	// add_timer(cycle,ev,1);
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
	event_t *process = createEvent(cycle_handler,cycle);
	add_event(cycle,process);
	cicle_process(cycle);
	deleteEvent(&process);
	deleteCycle(&cycle);
	return 0;
}
