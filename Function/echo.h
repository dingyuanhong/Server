#ifndef ECHO_H
#define ECHO_H

#include "../Event/Event.h"

int echo_read_event_handler(event_t *ev);

int echo_write_event_handler(event_t *ev);

int buffer_read(connection_t * c,char *byte,size_t len);

int buffer_write(connection_t * c,char * byte,size_t size);

#endif
