#ifndef EVENTTIMER_H
#define EVENTTIMER_H

#include "ngx_times.h"
#include "../Event/Event.h"

#define NGX_TIMER_INFINITE  (ngx_msec_t) -1

#define NGX_TIMER_LAZY_DELAY  300

void ngx_event_del_timer(ngx_rbtree_t * timeout,event_t *ev);
void ngx_event_add_timer(ngx_rbtree_t * timeout,event_t *ev, ngx_msec_t timer);
void ngx_event_timer_init(ngx_rbtree_t * timeout);
ngx_msec_t ngx_event_find_timer(ngx_rbtree_t * timeout);
void ngx_event_expire_timers(ngx_rbtree_t * timeout);
void ngx_event_cancel_timers(ngx_rbtree_t * timeout);

#endif
