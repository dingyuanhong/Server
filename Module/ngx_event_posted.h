
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_EVENT_POSTED_H_INCLUDED_
#define _NGX_EVENT_POSTED_H_INCLUDED_

#include "../Core/Core.h"
#include "../Event/Event.h"

#define ngx_post_event(ev, q)                                                 \
                                                                              \
    if (!(ev)->posted) {                                                      \
        (ev)->posted = 1;                                                     \
        ngx_queue_insert_tail(q, &(ev)->queue);                               \
    } else  {                                                                 \
		LOGE("ngx_post_event already posted.\n");							  \
    }


#define ngx_delete_posted_event(ev)                                           \
    (ev)->posted = 0;                                                         \
    ngx_queue_remove(&(ev)->queue);



static inline void ngx_event_process_posted(ngx_queue_t *posted)
{
	ngx_queue_t *q;
	event_t  *ev;

	ngx_queue_t tmp;
	ngx_queue_t *queue_ev = &tmp;
	ngx_queue_init(queue_ev);
	ngx_queue_add(queue_ev,posted);
	ngx_queue_init(posted);

	// ngx_queue_t *queue_ev = posted;

	while (!ngx_queue_empty(queue_ev)) {
		q = ngx_queue_head(queue_ev);
		ev = ngx_queue_data(q, event_t, queue);
		ngx_delete_posted_event(ev);
		ev->handler(ev);
	}
}

#endif /* _NGX_EVENT_POSTED_H_INCLUDED_ */
