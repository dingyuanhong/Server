#ifndef ECHO_H
#define ECHO_H

#include "../Event/Event.h"
#include "loopqueue.h"

int buffer_read(connection_t * c,char *byte,size_t len);

int buffer_write(connection_t * c,char * byte,size_t size);

void echo_init(connection_t * c);

#endif
