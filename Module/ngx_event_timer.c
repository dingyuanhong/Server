#include "ngx_event_timer.h"
#ifndef ngx_abs
#include <stdlib.h>
#define ngx_abs abs
#endif

//time events
void ngx_event_del_timer(ngx_rbtree_t * timeout,event_t *ev)
{
	if(ev->timer_set == 0)
	{
		return;
	}
	
    ngx_rbtree_delete(timeout, &ev->timer);

#if (NGX_DEBUG)
    ev->timer.left = NULL;
    ev->timer.right = NULL;
    ev->timer.parent = NULL;
#endif

    ev->timer_set = 0;
}


void ngx_event_add_timer(ngx_rbtree_t * timeout,event_t *ev, ngx_msec_t timer)
{
    ngx_msec_t      key;
    ngx_msec_int_t  diff;

    key = ngx_current_msec + timer;

    if (ev->timer_set) {
        /*
         * Use a previous timer value if difference between it and a new
         * value is less than NGX_TIMER_LAZY_DELAY milliseconds: this allows
         * to minimize the rbtree operations for fast connections.
         */
        diff = (ngx_msec_int_t) (key - ev->timer.key);

        if (ngx_abs(diff) < NGX_TIMER_LAZY_DELAY) {
            return;
        }

        ngx_event_del_timer(timeout,ev);
    }
    ev->timer.key = key;
    ngx_rbtree_insert(timeout, &ev->timer);

    ev->timer_set = 1;
}

void ngx_event_timer_init(ngx_rbtree_t * timeout)
{
	static ngx_rbtree_node_t  ngx_event_timer_sentinel;
	ngx_rbtree_init(timeout, &ngx_event_timer_sentinel,
                    ngx_rbtree_insert_timer_value);
}

ngx_msec_t ngx_event_find_timer(ngx_rbtree_t * timeout)
{
	ngx_msec_int_t      timer;
	ngx_rbtree_node_t  *node, *root, *sentinel;

	root = timeout->root;
	sentinel = timeout->sentinel;

	if (root == sentinel) {
		return NGX_TIMER_INFINITE;
	}

	node = ngx_rbtree_min(root, sentinel);

	timer = (ngx_msec_int_t) (node->key - ngx_current_msec);

	return (ngx_msec_t) (timer > 0 ? timer : 0);
}

void ngx_event_expire_timers(ngx_rbtree_t * timeout)
{
	event_t        *ev;
	ngx_rbtree_node_t  *node, *root, *sentinel;

	sentinel = timeout->sentinel;

	for ( ;; ) {
		root = timeout->root;
		if (root == sentinel) {
			return;
		}
		node = ngx_rbtree_min(root, sentinel);
		/* node->key > ngx_current_time */
		if ((ngx_msec_int_t) (node->key - ngx_current_msec) > 0) {
			return;
		}
		ev = (event_t *) ((char *) node - offsetof(event_t, timer));
		ngx_rbtree_delete(timeout, &ev->timer);

	#if (NGX_DEBUG)
		ev->timer.left = NULL;
		ev->timer.right = NULL;
		ev->timer.parent = NULL;
	#endif
		ev->timer_set = 0;
		ev->timedout = 1;
		ev->handler(ev);
	}
}

void ngx_event_cancel_timers(ngx_rbtree_t * timeout)
{
	event_t        *ev;
    ngx_rbtree_node_t  *node, *root, *sentinel;

    sentinel = timeout->sentinel;

    for ( ;; ) {
        root = timeout->root;

        if (root == sentinel) {
            return;
        }

        node = ngx_rbtree_min(root, sentinel);

        ev = (event_t *) ((char *) node - offsetof(event_t, timer));

        if (!ev->cancelable) {
            return;
        }

        ngx_rbtree_delete(timeout, &ev->timer);

#if (NGX_DEBUG)
        ev->timer.left = NULL;
        ev->timer.right = NULL;
        ev->timer.parent = NULL;
#endif

        ev->timer_set = 0;

        ev->handler(ev);
    }
}
