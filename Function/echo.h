#ifndef ECHO_H
#define ECHO_H

#include "../Event/Event.h"
#include "loopqueue.h"

typedef struct echo_s {
	connection_t * c;
	loopqueue_t queue;
}echo_t;

inline echo_t *echo_create(connection_t *c)
{
	echo_t * echo = (echo_t*)MALLOC(sizeof(echo_t));
	echo->c = c;
	queue_init(&echo->queue,10);
	return echo;
}

static inline void echo_destroy(echo_t **echo_ptr)
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

int echo_read_event_handler(event_t *ev);

int echo_write_event_handler(event_t *ev);

int echo_error_event_handler(event_t * ev);

int buffer_read(connection_t * c,char *byte,size_t len);

int buffer_write(connection_t * c,char * byte,size_t size);

#endif
