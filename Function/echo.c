#include "../Module/module.h"
#include "echo.h"

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
	}else
	{
		if(_ERRNO == _ERROR(EWOULDBLOCK))
		{
			return 0;
		}
		LOGE("recv error:%d errno:%d\n",ret,_ERRNO);
		connection_remove(c);
		return -1;
	}
}

int buffer_write(connection_t * c,char * byte,size_t len)
{
	int ret = send(c->so.handle,byte,len,0);
	if(ret == len)
	{
		return ret;
	}else if(ret > 0)
	{
		return ret;
	}else if(ret == -1)
	{
		if(_ERRNO == _ERROR(EWOULDBLOCK))
		{
			return 0;
		}
		LOGE("send error:%d errno:%d\n",ret,_ERRNO);
		connection_remove(c);
		return -1;
	}
	return 0;
}


typedef struct echo_s {
	connection_t * c;
	loopqueue_t queue;
}echo_t;

echo_t *echo_create(connection_t *c)
{
	echo_t * echo = (echo_t*)MALLOC(sizeof(echo_t));
	echo->c = c;
	queue_init(&echo->queue,12);
	return echo;
}

void echo_destroy(echo_t **echo_ptr)
{
	if(echo_ptr != NULL)
	{
		echo_t * echo = *echo_ptr;
		if(echo != NULL)
		{
			queue_delete(&echo->queue);
			FREE(echo);
			*echo_ptr = NULL;
		}
	}
}

void echo_read_event_handler(event_t *ev)
{

	echo_t * echo = (echo_t*)ev->data;
	connection_t *c = (connection_t*)echo->c;

	event_del(c->cycle,c->so.read);
	timer_del(c->cycle,c->so.read);

	// while(1)
	{
		void * buffer = queue_w(&echo->queue);
		int size = queue_wsize(&echo->queue);
		if(buffer == NULL || size <= 0){

			return;
		}
		int ret = buffer_read(c,buffer,size);
		if(ret < 0)
		{
			timer_add(c->cycle,c->so.read,1000);
			return;
		}
		if(ret == 0)
		{
			timer_add(c->cycle,c->so.read,1000);
			return;
		}
		queue_wpush(&echo->queue,ret);
		if(ret == size)
		{
			// 获取接受缓冲区数据大小
			if(socket_recvbuf_size(c->so.handle) > 0)
			{
				// 获取
				size = queue_wsize(&echo->queue);
				if(size > 0)
				{
					if(!event_is_add(c->cycle,c->so.read))
						event_add(c->cycle,c->so.read);
				}else
				{
					event_add(c->cycle,c->so.read);
				}
			}
		}
		if(!event_is_add(c->cycle,c->so.write))
			event_add(c->cycle,c->so.write);
	}
	timer_add(c->cycle,c->so.read,1000);
}

void echo_write_event_handler(event_t *ev)
{
	echo_t * echo = (echo_t*)ev->data;
	connection_t *c = (connection_t*)echo->c;
	event_del(c->cycle,c->so.write);
	timer_del(c->cycle,c->so.write);
	// while(1)
	{
		void * buffer = queue_r(&echo->queue);
		int size = queue_rsize(&echo->queue);
		if(buffer == NULL || size <= 0)
		{
			return;
		}
		int ret = buffer_write(c,buffer,size);
		if(ret < 0)
		{
			return;
		}
		if(ret == 0)
		{
			timer_add(c->cycle,c->so.write,100);
			return;
		}
		queue_rpush(&echo->queue,ret);
		size = queue_rsize(&echo->queue);
		if(size > 0)
		{
			if(!event_is_add(c->cycle,c->so.write))
				event_add(c->cycle,c->so.write);
		}
	}
}

void echo_error_event_handler(event_t * ev)
{
	echo_t * echo = (echo_t*)ev->data;
	connection_t *c = (connection_t*)echo->c;
	if(connection_del(c) == 0)
	{
		echo_destroy(&echo);
	}
}

void echo_init(connection_t * c)
{
	ASSERT(c != NULL);
	echo_t * echo = echo_create(c);
	c->so.read = event_create(echo_read_event_handler,echo);
	c->so.write = event_create(echo_write_event_handler,echo);
	c->so.error = event_create(echo_error_event_handler,echo);
	timer_add(c->cycle,c->so.read,1000);
}
