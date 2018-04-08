#ifndef MODULE_H
#define MODULE_H

#include "cycle.h"
#include "connection.h"
#include "ngx_times.h"
#include "ngx_event_timer.h"
#include "ngx_event_posted.h"

#define connection_cycle_queue_add(conn)                                    \
    if (ngx_queue_empty(&conn->queue)) {                                    \
		conn->cycle->connection_count++;									\
        ngx_queue_insert_tail(&conn->cycle->connection_queue, &conn->queue);\
    } else  {                                                               \
		LOGE("ngx_post_event already posted.\n");							\
    }

#define connection_cycle_queue_del(conn)                                     \
    {ngx_queue_remove(&conn->queue);											 \
	ngx_queue_init(&conn->queue);											 \
	conn->cycle->connection_count--;}

inline int connection_cycle_add_(connection_t *conn,int event,int flags)
{
	int ret =  action_add(conn->cycle->core,&conn->so,event,flags);
	if(ret == 0)
	{
		connection_cycle_queue_add(conn);
	}
	return ret;
}

inline int connection_cycle_add(connection_t *conn)
{
	return connection_cycle_add_(conn,NGX_READ_EVENT,0);
}

inline int connection_cycle_del(connection_t *conn)
{
	int ret = action_del(conn->cycle->core,&conn->so);
	if(ret == 0)
	{
		connection_cycle_queue_del(conn);
	}
	return ret;
}

#define event_is_add(cycle,ev) ((ev)->posted == 1)
#define event_add(cycle,ev) {ASSERT(ev != NULL);ngx_post_event(ev,&cycle->posted);}
#define event_del(cycle,ev) if(ev != NULL){ngx_delete_posted_event(ev);}
#define connection_event_del(conn) {event_del(conn->cycle,conn->so.read); \
							event_del(conn->cycle,conn->so.write); \
							event_del(conn->cycle,conn->so.error);}

#define timer_add(cycle,ev,time) {ASSERT(ev!=NULL);ngx_event_add_timer(&cycle->timeout,ev,time);}
#define timer_del(cycle,ev) if(ev != NULL){ngx_event_del_timer(&cycle->timeout,ev);}
#define connection_timer_del(conn) {timer_del(conn->cycle,conn->so.read); \
							timer_del(conn->cycle,conn->so.write); \
							timer_del(conn->cycle,conn->so.error);}



#define timer_is_empty(cycle) (cycle->timeout.root == cycle->timeout.sentinel)
#define event_is_empty(cycle) ngx_queue_empty(&cycle->posted)

//slave safe

typedef void (*safe_event_handle_pt)(cycle_t * cycle,event_t *ev);

typedef struct safe_event_s{
	event_t self;
	cycle_t *cycle;
	event_t *event;
	safe_event_handle_pt handler;
}safe_event_t;

static inline void safe_event_handler(event_t *ev)
{
	safe_event_t * sev = (safe_event_t*)ev->data;
	sev->handler(sev->cycle,sev->event);
	FREE(sev);
}

static inline void safe_add_event(cycle_t *cycle,event_t * ev,safe_event_handle_pt handler)
{
	safe_event_t * sev = (safe_event_t*)MALLOC(sizeof(safe_event_t));
	sev->cycle = cycle;
	sev->event = ev;
	sev->handler = handler;
	event_init(&sev->self,safe_event_handler,sev);
	ngx_spinlock(&cycle->async_posted_lock,1,0);
	ngx_post_event(&sev->self,&cycle->async_posted);
	cycle->async_posted_count += 1;
	ngx_unlock(&cycle->async_posted_lock);
}

static inline void safe_process_event(cycle_t *cycle)
{
	if(cycle->async_posted_count > 0)
	{
		ngx_spinlock(&cycle->async_posted_lock,1,0);
		// if(ngx_trylock(&cycle->accept_lock))
		{
			ngx_queue_add(&cycle->posted,&cycle->async_posted);
			ngx_queue_init(&cycle->async_posted);

			cycle->async_posted_count = 0;
			ngx_unlock(&cycle->async_posted_lock);
		}
	}
}


//close connection
//释放
static inline int connection_destroy_object(connection_t *c)
{
	ASSERT(c != NULL);
	SOCKET so = c->so.handle;
	int ret = socket_linger(so,1,0);//直接关闭SOCKET，避免TIME_WAIT
	ABORTIF(ret != 0,"socket_linger %d\n",ret);
	// ret = shutdown(so,SHUT_WR);
	// ABORTIF(ret != 0,"shutodwn %d\n",ret);
	ret = close(so);
	if(ret == 0)
	{
		connection_destroy(&c);
		LOGD("connection close:%d\n",so);
		return 0;
	}else{
		LOGD("connection closing:%d errno:%d\n",so,_ERRNO);
		return -1;
	}
}

static inline void connection_destroy_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	int ret = connection_destroy_object(c);
	if(ret == -1)
	{
		// event_add(c->cycle,ev);
		ngx_post_event(ev,&c->cycle->internal_posted);
	}
}

//清理事件
static inline void connection_clear(connection_t *c){
	ASSERT(c != NULL);
	// connection_cycle_queue_del(c);
	connection_event_del(c);
	connection_timer_del(c);

	ASSERT(c->so.error != NULL);
	c->so.error->handler = connection_destroy_handler;
	c->so.error->data = c;
	// event_add(c->cycle,c->so.error);
	ngx_post_event(c->so.error,&c->cycle->internal_posted);
}

static inline void connection_clear_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	connection_clear(c);
}

static inline int connection_del(connection_t *c){
	ASSERT(c != NULL);
	int ret = connection_cycle_del(c);
	if(ret == 0)
	{
		c->so.error->handler = connection_clear_handler;
		c->so.error->data = c;
	}
	// event_add(c->cycle,c->so.error);
	ngx_post_event(c->so.error,&c->cycle->internal_posted);
	return ret;
}

static inline void connection_del_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	connection_del(c);
}

#define connection_error_handle connection_del_handler

static inline void connection_remove(connection_t * c)
{
	ASSERT(c != NULL);
	ASSERT(c->so.error != NULL);
	// event_add(c->cycle,c->so.error);
	if(!event_is_add(c->sycle,c->so.error)){
		ngx_post_event(c->so.error,&c->cycle->internal_posted);
	}
}

//remove connection

typedef void (*connection_remove_pt)(connection_t * c);

static inline void cycle_remove_connections(cycle_t * cycle,connection_remove_pt remove)
{
	ngx_queue_t *q;
	connection_t  *conn;
	ngx_queue_t *queue_ev = &cycle->connection_queue;
	while (!ngx_queue_empty(queue_ev)) {
		q = ngx_queue_head(queue_ev);
		conn = ngx_queue_data(q, connection_t, queue);
		connection_cycle_queue_del(conn);
		remove(conn);
	}
}

static inline int cycle_process(cycle_t * cycle)
{
	LOGD("cycle_process begin(%d).\n",cycle->index);
	if(cycle->index != -1)
	{
		thread_affinity_cpu(cycle->index);
	}
	cycle_process_init(cycle);
	while(!cycle->stop){
		if(cycle->master)
		{
			ngx_time_update();
		}
		
		ngx_msec_t timeout = ngx_event_find_timer(&cycle->timeout);
		if(!event_is_empty(cycle))
		{
			timeout = 0;
		}else
		if(timeout == NGX_TIMER_INFINITE)
		{
			timeout = 10;
		}
		if(cycle->connection_count > 0)
		{
			int ret = action_process(cycle->core,timeout);
			if(ret == -1)
			{
				break;
			}else if(ret > 0)
			{
				// LOGD("action_process :%d\n",ret);
			}
		}
		
		if(cycle->master)
		{
			ngx_time_update();
		}
		ngx_event_expire_timers(&cycle->timeout);
		safe_process_event(cycle);
		ngx_event_process_posted(&cycle->posted);

		ngx_event_process_posted(&cycle->internal_posted);

		cycle_process_step(cycle);

		if(cycle->master && 
			cycle->connection_count == 0 && 
			event_is_empty(cycle) && timer_is_empty(cycle))
		{
			break;
		}
	}
	cycle_process_over(cycle);
	cycle_remove_connections(cycle,connection_remove);
	cycle_process_closing(cycle);
	while(1){
		ngx_event_process_posted(&cycle->internal_posted);
		if(ngx_queue_empty(&cycle->internal_posted))
		// if(event_is_empty(cycle))
		{
			break;
		}
	}
	cycle_process_end(cycle);
	LOGD("cycle_process end(%d).\n",cycle->index);
	return 0;
}

#endif
