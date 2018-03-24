#include "../Module/module.h"
#include "echo.h"
#include "connection_close.h"

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#endif

int buffer_read(connection_t * c,char *byte,size_t len)
{
	while(1){
		int ret = recv(c->so.handle,byte,len,0);
		if(ret == len)
		{
			return ret;
		}
		else if(ret > 0)
		{
			return ret;
		}
		else if(ret == 0)
		{
			connection_remove(c);
			return 0;
		}else if(ret == -1)
		{
			if(EAGAIN == errno)
			{
				continue;
			}
			LOGE("recv error:%d errno:%d\n",ret,errno);
			connection_remove(c);
			return -1;
		}
	}
	return 0;
}

int buffer_write(connection_t * c,char * byte,size_t len)
{
	int size = 0;
	while(1){
		int ret = send(c->so.handle,byte,len,0);
		if(ret == len)
		{
			return ret;
		}else if(ret > 0)
		{
			byte += ret;
			len -= ret;
			size += ret;
			continue;
		}else if(ret == -1){
			if(errno == EAGAIN)
			{
				continue;
			}
			LOGE("send error:%d errno:%d\n",ret,errno);
			return -1;
		}else{
			break;
		}
	}
	return size;
}

int echo_read_event_handler(event_t *ev)
{
	char byte[65536];
	size_t len = 65536;

	connection_t *c = (connection_t*)ev->data;
	while(1){
		int ret = buffer_read(c,byte,len);
		if(ret <= 0)
		{
			break;
		}
		int r = buffer_write(c,byte,ret);
		if(r == -1)
		{
			connection_remove(c);
			break;
		}
		if(ret == len)
		{
			continue;
		}
		break;
	}
	return 1;
}

int echo_write_event_handler(event_t *ev)
{
	char buf[1024];
	int len = 1024;
	connection_t *c = (connection_t*)ev->data;
	int ret = buffer_write(c,buf,len);
	if(ret == -1)
	{
		connection_remove(c);
	}
	return ret;
}
