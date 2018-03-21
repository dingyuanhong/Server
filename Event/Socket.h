#ifndef OBJECT_CORE_H
#define OBJECT_CORE_H

#include "../Core/Core.h"
#include "Event.h"

typedef struct socket_s{
	SOCKET handle;
	event_t *read;
	event_t *write;
	event_t *error;
}socket_t;

#endif
