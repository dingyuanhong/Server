#include "service.h"
#include "echo.h"

void service_init(connection_t * c)
{
	ASSERT(c != NULL);
	c->so.read = event_create(echo_read_event_handler,c);
	c->so.error = NULL;
}
