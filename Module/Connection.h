#ifndef CONNECTION_H
#define CONNECTION_H

#include "../Event/Socket.h"
#include "cycle.h"

typedef struct connection_s{
	socket_t so;
	cycle_t * cycle;
	ngx_queue_t queue;
}connection_t;

static inline connection_t * connection_create(cycle_t * cycle,SOCKET s)
{
	connection_t * conn = MALLOC(sizeof(connection_t));
	conn->so.handle = s;
	conn->so.read = NULL;
	conn->so.write = NULL;
	conn->so.error = NULL;
	conn->cycle = cycle;
	ngx_queue_init(&conn->queue);
	return conn;
}

static inline void connection_destroy(connection_t** conn){
	if(conn != NULL)
	{
		if(*conn != NULL){
			connection_t* c = *conn;
			event_destroy(&c->so.read);
			event_destroy(&c->so.write);
			event_destroy(&c->so.error);
			FREE(c);
		}
		*conn = NULL;
	}
}

#endif
