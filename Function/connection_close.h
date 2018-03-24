#ifndef CONNECTION_CLOSE_H
#define CONNECTION_CLOSE_H

//connection close

static inline int connection_close_handler(event_t *ev)
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
		connection_destroy(&c);
	}else{
		LOGD("connection closing:%d\n",c->so.handle);
		event_add(c->cycle,ev);
	}
	return 0;
}

inline void connection_close(connection_t *c){
	ASSERT(c != NULL);
	ASSERT(c->so.error != NULL);
	connection_event_del(c);
	connection_timer_del(c);
	c->so.error->handler = connection_close_handler;
	event_add(c->cycle,c->so.error);
}

inline int connection_remove(connection_t * c)
{
	int ret = connection_cycle_del(c);
	if(ret == 0){
		connection_close(c);
	}else{
		LOGD("connection remove %d errno:%d\n",ret,errno);
	}
	return ret;
}

#endif
