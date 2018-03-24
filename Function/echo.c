#include "../Module/module.h"
#include "echo.h"
#include "connection_close.h"

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#endif


int echo_buffer(connection_t * c,char * byte,size_t size){
	while(1){
		int ret = send(c->so.handle,byte,size,0);
		if(ret == size)
		{
			return ret;
		}else if(ret > 0)
		{
			byte += ret;
			size -= ret;
			continue;
		}else{
			if(errno == EAGAIN)
			{
				continue;
			}
			return -1;
		}
	}
	return 0;
}

int echo_read_event_handler(event_t *ev)
{
	char byte[65536];
	size_t len = 65536;

	connection_t *c = (connection_t*)ev->data;
	while(1){
		LOGD("recv %d ...\n",c->so.handle);
		int ret = recv(c->so.handle,&byte,len,0);
		if(ret > 0)
		{
			int r = echo_buffer(c,byte,ret);
			if(r == -1)
			{
				connection_remove(c);
				break;
			}
		}
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

int echo_write_event_handler(event_t *ev)
{
	char buf[1024];
	int len = 1024;
	connection_t *c = (connection_t*)ev->data;
	int ret = echo_buffer(c,buf,len);
	if(ret == -1)
	{
		connection_remove(c);
	}
	return ret;
}
