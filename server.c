#include "Core/Core.h"
#include "Event/EventActions.h"
#include "Module/module.h"
#include "Core/thread.h"

#define MAX_FD_COUNT 1024*1024
static int max_thread_count = 0;
ngx_array_t *cycle_pool = NULL;
int cycle_pool_index = 0;
ngx_array_t *thread_pool = NULL;

static inline void *ngx_array_get(ngx_array_t *a, ngx_uint_t n)
{
	if(a->nalloc > n){
		return (u_char *) a->elts + a->size * n;
	}
	return NULL;
}

int cycle_thread_post(cycle_t *cycle,SOCKET fd);

int connection_close_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	int ret = 0;
	ret = socket_linger(c->so.handle,1,0);//直接关闭SOCKET，避免TIME_WAIT
	ABORTIF(ret == 0,"socket_linger %d\n",ret);
	// ret = shutdown(c->so.handle,SHUT_WR);
	// ABORTIF(ret == 0,"shutodwn %d\n",ret);
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
	conn->so.read = createEvent(accept_event_handler,conn);
	conn->so.write = NULL;
	conn->so.error = createEvent(error_event_handler,conn);
	ret = add_connection(conn);
	ABORTIF(ret == 0,"action_add %d errno:%d\n",ret,errno);
	return 0;
}

int connection_add_event(event_t *ev)
{
	connection_t * conn = (connection_t*)ev->data;
	deleteEvent(&ev);
	conn->so.read = createEvent(error_event_handler,conn);
	conn->so.write = NULL;
	conn->so.error = createEvent(error_event_handler,conn);
	int ret = add_connection(conn);
	ABORTIF(ret == 0,"action_add %d errno:%d\n",ret,errno);
	return ret;
}


void connection_add_event_sub(cycle_t * cycle,event_t *ev)
{
	SOCKET fd = (SOCKET)ev->data;
	deleteEvent(&ev);
	// LOGD("add_connection_event.%d\n",fd);
	connection_t * conn = createConn(cycle,fd);
	conn->so.read = createEvent(error_event_handler,conn);
	conn->so.write = NULL;
	conn->so.error = createEvent(error_event_handler,conn);
	int ret = add_connection(conn);
	ABORTIF(ret == 0,"action_add %d errno:%d\n",ret,errno);
}

void cycle_thread_cb(void* arg)
{
	cycle_t *cycle = (cycle_t*)arg;
	cicle_process_loop(cycle);
}

int cycle_thread_post(cycle_t *cycle,SOCKET fd)
{
	if(cycle_pool == NULL || thread_pool == NULL)
	{
		add_event(cycle,createEvent(connection_add_event,createConn(cycle,fd)));
	}else{
		cycle_t ** subcycle_ptr = (cycle_t**)ngx_array_get(cycle_pool,cycle_pool_index%max_thread_count);
		ABORTI(subcycle_ptr == NULL);
		if(*subcycle_ptr == NULL)
		{
			cycle_t *subcycle = createCycle(MAX_FD_COUNT);
			ABORTI(subcycle == NULL);
			ABORTI(subcycle->core == NULL);
			*subcycle_ptr = subcycle;

			//启动线程
			uv_thread_t * thread_id = (uv_thread_t*)ngx_array_get(thread_pool,cycle_pool_index%max_thread_count);
			int ret = uv_thread_create(thread_id,cycle_thread_cb,subcycle);
			ABORTI(ret != 0);
		}
		ABORTI(*subcycle_ptr == NULL);
		safe_add_event(*subcycle_ptr,createEvent(NULL,(void*)fd),connection_add_event_sub);

		cycle_pool_index++;
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

	max_thread_count = (ngx_ncpu - 1)*2;
	if(max_thread_count > 0)
	{
		cycle_pool = ngx_array_create(max_thread_count,sizeof(cycle_t*));
		thread_pool = ngx_array_create(max_thread_count,sizeof(uv_thread_t));
	}

	event_t *process = createEvent(accept_handler,cycle);
	add_event(cycle,process);
	cicle_process(cycle);
	deleteEvent(&process);
	deleteCycle(&cycle);
	return 0;
}
