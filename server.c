#include "Core/Core.h"
#include "Event/EventActions.h"
#include "Module/module.h"

#define MAX_FD_COUNT 1024*1024

int connection_close_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	int ret = 0;
	ret = socket_linger(c->so.handle,1,0);//直接关闭SOCKET，避免TIME_WAIT
	LOGA(ret == 0,"socket_linger %d\n",ret);
	// ret = shutdown(c->so.handle,SHUT_WR);
	// LOGA(ret == 0,"shutodwn %d\n",ret);
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
}

void connection_close(connection_t *c){
	event_t * timer = createEvent(connection_close_handler,c);
	add_event(c->cycle,timer);
}

int accept_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	int count = 0;
	while(1){
		struct sockaddr_in addr;
		socklen_t len = sizeof(struct sockaddr_in);
		// LOGD("accept %d ...\n",c->so.handle);
		int afd = accept(c->so.handle,(struct sockaddr*)&addr,&len);
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
		connection_close(createConn(c->cycle,afd));
	}
	return count;
}

int error_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	int ret = del_connection(c);
	if(ret == 0)
	{
		connection_close(c);
		// LOGD("error close success\n");
	}else
	{
		LOGE("error close failed %d errno:%d\n",ret,errno);
	}
	return 0;
}

int cycle_handler(event_t *ev)
{
	cycle_t *cycle = (cycle_t*)ev->data;
	SOCKET fd = socket_bind("tcp","127.0.0.1:888");
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
	conn->so.read = createEvent(accept_handler,conn);
	conn->so.write = NULL;
	conn->so.error = createEvent(error_handler,conn);
	ret = add_connection(conn);
	LOGA(ret == 0,"action_add %d errno:%d\n",ret,errno);
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
	add_event(cycle,process);
	cicle_process(cycle);
	deleteEvent(&process);
	deleteCycle(&cycle);
	return 0;
}
