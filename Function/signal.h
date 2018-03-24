#include <signal.h>
#include "../Module/slave.h"

extern void * g_signal_master = NULL;

#ifdef _WIN32
void signal_init(void * data)
{}
#else
static void signal_handle_term(int sig)
{
	LOGI("signal exit:%d\n",sig);
	cycle_t *cycle = (cycle_t*)g_signal_master;
	if(cycle != NULL)
	{
		if(cycle->data != NULL)
		{
			cycle_slave_t * slave = (cycle_slave_t*)cycle->data;
			slave_wait_stop(slave);
		}
		cycle->stop = 1;
	}
}

void signal_init(void * data){
	g_signal_master = data;

	signal(SIGTERM , signal_handle_term);
	signal(SIGINT , signal_handle_term);
	signal(SIGQUIT , signal_handle_term);
}
#endif
