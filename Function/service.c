#include "service.h"
#include "echo.h"

void service_init(connection_t * c)
{
	echo_init(c);
}
