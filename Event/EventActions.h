#ifndef EVENTACTIONS_H
#define EVENTACTIONS_H

#include "Socket.h"
#include "EpollModule.h"
#include "KqueueModule.h"
#include "SelectModule.h"

typedef struct core_s{
}core_t;

core_t * action_create(int concurrent);
int action_done(core_t * core);
int action_add(core_t * core,socket_t * obj,int event, int flags);
int action_del(core_t * core,socket_t * obj);
int action_process(core_t * core,int milliseconds);

#endif
