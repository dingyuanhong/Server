#ifndef CONNECTION_H
#define CONNECTION_H

#include "../Event/Socket.h"
#include "cycle.h"

typedef struct connection_s{
	socket_t so;
	cycle_t * cycle;
}connection_t;

inline connection_t * connection_create(cycle_t * cycle,SOCKET s)
{
	connection_t * conn = MALLOC(sizeof(connection_t));
	conn->so.handle = s;
	conn->so.read = NULL;
	conn->so.write = NULL;
	conn->so.error = NULL;
	conn->cycle = cycle;
	return conn;
}

inline void connection_destroy(connection_t** conn){
	if(conn != NULL)
	{
		if(*conn != NULL){
			event_destroy(&(*conn)->so.read);
			event_destroy(&(*conn)->so.write);
			event_destroy(&(*conn)->so.error);
			FREE(*conn);
		}
		*conn = NULL;
	}
}

#endif
