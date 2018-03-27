#include "../Module/module.h"
#include "echo.h"
#include "connection_close.h"

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#endif

int buffer_read(connection_t * c,char *byte,size_t len)
{
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
		return -1;
	}else if(ret == -1)
	{
		if(EAGAIN == errno)
		{
			return 0;
		}
		LOGE("recv error:%d errno:%d\n",ret,errno);
		connection_remove(c);
		return -1;
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
			size += ret;
			break;
		}else if(ret > 0)
		{
			byte += ret;
			len -= ret;
			size += ret;
			continue;
		}else if(ret == -1){
			if(errno == EAGAIN)
			{
				break;
			}
			LOGE("send error:%d errno:%d\n",ret,errno);
			connection_remove(c);
			return -1;
		}else{
			break;
		}
	}
	return size;
}

int echo_read_event_handler(event_t *ev)
{

	echo_t * echo = (echo_t*)ev->data;
	connection_t *c = (connection_t*)echo->c;

	// while(1)
	{
		void * buffer = queue_w(&echo->queue);
		int size = queue_wsize(&echo->queue);
		if(buffer == NULL || size <= 0) return 1;
		int ret = buffer_read(c,buffer,size);
		if(ret <= 0)
		{
			return 1;
		}
		queue_wpush(&echo->queue,ret);
	}
	return 1;
}

int echo_write_event_handler(event_t *ev)
{
	echo_t * echo = (echo_t*)ev->data;
	connection_t *c = (connection_t*)echo->c;

	// while(1)
	{
		void * buffer = queue_r(&echo->queue);
		int size = queue_rsize(&echo->queue);
		if(buffer == NULL || size <= 0) return 1;
		int ret = buffer_write(c,buffer,size);
		if(ret <= 0)
		{
			return 1;
		}
		queue_rpush(&echo->queue,ret);
	}
	return 1;
}

static int error_event_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	int ret = connection_remove(c);
	if(ret != 0)
	{
		LOGD("connection remove %d errno:%d\n",ret,errno);
	}
	return 0;
}

int echo_error_event_handler(event_t * ev)
{
	echo_t * echo = (echo_t*)ev->data;
	connection_t *c = (connection_t*)echo->c;
	echo_destroy(&echo);
	c->so.error->handler = error_event_handler;
	c->so.error->data = c;
	event_add(c->cycle,c->so.error);
	return 0;
}
