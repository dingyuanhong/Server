#ifndef kqueue_module_t_H
#define kqueue_module_t_H

#ifdef __MAC__
#define NGX_HAVE_KQUEUE 1
#endif

#if (NGX_HAVE_KQUEUE)

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#define NGX_READ_EVENT EVFILT_READ
#define NGX_WRITE_EVENT EVFILT_WRITE

#define NGX_FLAGS_ERROR EV_ERROR
#define NGX_FLAGS_ONESHAOT EV_ONESHOT
#define NGX_FLAGS_OOB EV_OOBAND

#define NGX_FLAGS_EOF EV_EOF
#define NGX_FLAGS_CLEAR EV_CLEAR


#include "Socket.h"
#include "Event.h"

typedef struct kqueue_module_s
{
	int handle;
	event_t * process;

	int max_events_count;
	int events_count;
	void * events;
}kqueue_module_t;

kqueue_module_t * kqueue_module_create(int concurrent);
int kqueue_module_done(kqueue_module_t * module);
int kqueue_module_add(kqueue_module_t * module,socket_t * so,int event, int flags);
int kqueue_module_del(kqueue_module_t * module,socket_t * so);
int kqueue_module_set(kqueue_module_t * module,socket_t * so,int event, int flags);
int kqueue_module_process(kqueue_module_t * module,int milliseconds);

#endif

#endif
