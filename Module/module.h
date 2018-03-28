#ifndef MODULE_H
#define MODULE_H

#include "cycle.h"
#include "connection.h"
#include "ngx_times.h"
#include "ngx_event_timer.h"
#include "ngx_event_posted.h"

#define connection_cycle_queue_add(conn)                                     		\
    if (ngx_queue_empty(&conn->queue)) {                                    \
        ngx_queue_insert_tail(&conn->cycle->connection_queue, &conn->queue);\
    } else  {                                                               \
		LOGE("ngx_post_event already posted.\n");							\
    }

#define connection_cycle_queue_del(conn)                                           \
    ngx_queue_remove(&conn->queue);											 \
	ngx_queue_init(&conn->queue);

inline int connection_cycle_add_(connection_t *conn,int event,int flags)
{
	int ret =  action_add(conn->cycle->core,&conn->so,event,flags);
	if(ret == 0)
	{
		conn->cycle->connection_count++;
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
		conn->cycle->connection_count--;
		connection_cycle_queue_del(conn);
	}
	return ret;
}


#define event_add(cycle,ev) ASSERT(ev != NULL);ngx_post_event(ev,&cycle->posted)
#define event_del(cycle,ev) if(ev != NULL){ngx_delete_posted_event(ev);}
#define connection_event_del(conn) event_del(conn->cycle,conn->so.read); \
							event_del(conn->cycle,conn->so.write); \
							event_del(conn->cycle,conn->so.error);

#define timer_add(cycle,ev,time) ASSERT(ev!=NULL);ngx_event_add_timer(&cycle->timeout,ev,time)
#define timer_del(cycle,ev) if(ev != NULL){ngx_event_del_timer(&cycle->timeout,ev);}
#define connection_timer_del(conn) timer_del(conn->cycle,conn->so.read); \
							timer_del(conn->cycle,conn->so.write); \
							timer_del(conn->cycle,conn->so.error);



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
	ngx_spinlock(&cycle->accept_posted_lock,1,0);
	ngx_post_event(&sev->self,&cycle->accept_posted);
	cycle->accept_posted_index += 1;
	ngx_unlock(&cycle->accept_posted_lock);
}

inline void safe_process_event(cycle_t *cycle)
{
	if(cycle->accept_posted_index > 0)
	{
		ngx_spinlock(&cycle->accept_posted_lock,1,0);
		// if(ngx_trylock(&cycle->accept_lock))
		{
			ngx_queue_add(&cycle->posted,&cycle->accept_posted);
			ngx_queue_init(&cycle->accept_posted);

			cycle->accept_posted_index = 0;
			ngx_unlock(&cycle->accept_posted_lock);
		}
	}
}


//close connection

inline int connection_close_object(connection_t *c)
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
		return 0;
	}else{
		LOGD("connection closing:%d errorno:%d\n",so,errno);
		return -1;
	}
}

static inline void connection_close_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	int ret = connection_close_object(c);
	if(ret == -1)
	{
		event_add(c->cycle,ev);
	}
}

static inline void connection_close_self(connection_t *c){
	ASSERT(c != NULL);
	connection_cycle_queue_del(c);
	connection_event_del(c);
	connection_timer_del(c);

	ASSERT(c->so.error != NULL);
	c->so.error->handler = connection_close_handler;
	c->so.error->data = c;
	event_add(c->cycle,c->so.error);
}

static inline int connection_close(connection_t * c)
{
	int ret = connection_cycle_del(c);
	if(ret == 0){
		connection_close_self(c);
	}else{
		LOGD("connection close %d errno:%d\n",ret,errno);
	}
	return ret;
}

static inline void connection_close_event_handler(event_t *ev)
{
	connection_t *c = (connection_t*)ev->data;
	connection_close(c);
}

//remove connection

typedef int (*connection_remove_pt)(connection_t * c);

inline void cycle_remove_connections(cycle_t * cycle,connection_remove_pt remove)
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

inline int connection_remove_default(connection_t * c)
{
	ASSERT(c != NULL);
	ASSERT(c->so.error != NULL);
	event_add(c->cycle,c->so.error);
	return 0;
}

static inline int cycle_process_master(cycle_t * cycle)
{
	LOGD("cycle_process_master begin.\n");
	while(!cycle->stop){
		ngx_time_update();
		ngx_msec_t timeout = ngx_event_find_timer(&cycle->timeout);
		if(!event_is_empty(cycle))
		{
			timeout = 0;
		}else
		if(timeout == NGX_TIMER_INFINITE)
		{
			timeout = 10;
		}
		int ret = action_process(cycle->core,timeout);
		if(ret == -1)
		{
			break;
		}else if(ret > 0)
		{
			// LOGD("action_process :%d\n",ret);
		}
		ngx_time_update();
		ngx_event_expire_timers(&cycle->timeout);
		safe_process_event(cycle);
		ngx_event_process_posted(&cycle->posted);

		if(cycle->connection_count == 0 && event_is_empty(cycle) && timer_is_empty(cycle))
		{
			break;
		}
	}
	cycle_remove_connections(cycle,connection_remove_default);
	while(1){
		ngx_event_process_posted(&cycle->posted);
		if(event_is_empty(cycle))
		{
			break;
		}
	}
	LOGD("cycle_process_master end.\n");
	return 0;
}

static inline int cycle_process_slave(cycle_t * cycle)
{
	LOGD("cycle_process_slave begin %d.\n",cycle->index);
	if(cycle->index != -1)
	{
		thread_affinity_cpu(cycle->index);
	}
	while(!cycle->stop){
		ngx_msec_t timeout = ngx_event_find_timer(&cycle->timeout);
		if(!event_is_empty(cycle))
		{
			timeout = 0;
		}else
		if(timeout == NGX_TIMER_INFINITE)
		{
			timeout = 10;
		}
		int ret = action_process(cycle->core,timeout);
		if(ret == -1)
		{
			break;
		}else if(ret > 0)
		{
			// LOGD("action_process :%d\n",ret);
		}
		ngx_event_expire_timers(&cycle->timeout);
		safe_process_event(cycle);
		ngx_event_process_posted(&cycle->posted);
	}
	cycle_remove_connections(cycle,connection_remove_default);
	while(1){
		ngx_event_process_posted(&cycle->posted);
		if(event_is_empty(cycle))
		{
			break;
		}
	}
	LOGD("cycle_process_slave end %d.\n",cycle->index);
	return 0;
}

#endif
