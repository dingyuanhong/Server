#include "service.h"
#include "echo.h"

void service_init(connection_t * c)
{
	ASSERT(c != NULL);
	echo_t * echo = echo_create(c);
	c->so.read = event_create(echo_read_event_handler,echo);
	c->so.read = event_create(echo_write_event_handler,echo);
	c->so.error = event_create(echo_error_event_handler,echo);
}
