#include "EventActions.h"

typedef struct EventActionmodule{
	void * (*create)(int concurrent);
	int (*done)(void * module);
	int (*add)(void * module,void * so,int event, int flags);
	int (*del)(void * module,void * so);
	int (*process)(void * module,int milliseconds);
}EventActionmodule;

#if  (NGX_HAVE_EPOLL)
static const EventActionmodule epoll_action = {
	epoll_module_create,
	epoll_module_done,
	epoll_module_add,
	epoll_module_del,
	epoll_module_process
};
#endif
#if  (NGX_HAVE_KQUEUE)
static const EventActionmodule kqueue_action = {
	kqueue_module_create,
	kqueue_module_done,
	kqueue_module_add,
	kqueue_module_del,
	kqueue_module_process
};
#endif
#if  (NGX_HAVE_SELECT)
static const EventActionmodule select_action = {
	select_module_create,
	select_module_done,
	select_module_add,
	select_module_del,
	select_module_process
};
#endif

#if  (NGX_HAVE_EPOLL)
#define DEFAULT_ACTION_module epoll_action
#elif (NGX_HAVE_KQUEUE)
#define DEFAULT_ACTION_module kqueue_action
#elif (NGX_HAVE_SELECT)
#define DEFAULT_ACTION_module select_action
#endif

core_t * action_create(int concurrent)
{
	return DEFAULT_ACTION_module.create(concurrent);
}

int action_done(core_t * module)
{
	return DEFAULT_ACTION_module.done(module);
}

int action_add(core_t * module,socket_t * so,int event, int flags)
{
	return DEFAULT_ACTION_module.add(module,so,event,flags);
}

int action_del(core_t * module,socket_t * so)
{
	return DEFAULT_ACTION_module.del(module,so);
}

int action_process(core_t * module,int milliseconds)
{
	return DEFAULT_ACTION_module.process(module,milliseconds);
}
