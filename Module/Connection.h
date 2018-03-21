#ifndef CONNECTION_H
#define CONNECTION_H

#include "../Core/Core.h"
#include "../Event/EventActions.h"
#include "Cycle.h"

typedef struct connection_s{
	socket_t so;
	cycle_t * cycle;
}connection_t;

inline connection_t * createConn(cycle_t * cycle,SOCKET s)
{
	connection_t * conn = MALLOC(sizeof(connection_t));
	conn->so.handle = s;
	conn->so.read = NULL;
	conn->so.write = NULL;
	conn->so.error = NULL;
	conn->cycle = cycle;
	return conn;
}

inline void deleteConn(connection_t** conn){
	if(conn != NULL)
	{
		if(*conn != NULL){
			deleteEvent(&(*conn)->so.read);
			deleteEvent(&(*conn)->so.write);
			deleteEvent(&(*conn)->so.error);
			FREE(*conn);
		}
		*conn = NULL;
	}
}

#endif
