
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include "ngx_array.h"

ngx_array_t *
ngx_array_create(ngx_uint_t n, size_t size)
{
    ngx_array_t *a;

    a = MALLOC(sizeof(ngx_array_t));
    if (a == NULL) {
        return NULL;
    }

    if (ngx_array_init(a, n, size) != NGX_OK) {
        return NULL;
    }

    return a;
}


void
ngx_array_destroy(ngx_array_t *a)
{
	if(a->elts != NULL){
		FREE(a->elts);
		a->elts = NULL;
	}
	FREE(a);
}


void *
ngx_array_push(ngx_array_t *a)
{
    void        *elt, *new;
    size_t       size;

    if (a->nelts == a->nalloc) {

        /* the array is full */

        size = a->size * a->nalloc;

        {
            /* allocate a new array */

            new = MALLOC(2 * size);
            if (new == NULL) {
                return NULL;
            }

            memcpy(new, a->elts, size);
            a->elts = new;
            a->nalloc *= 2;
        }
    }

    elt = (u_char *) a->elts + a->size * a->nelts;
    a->nelts++;

    return elt;
}


void *
ngx_array_push_n(ngx_array_t *a, ngx_uint_t n)
{
    void        *elt, *new;
    ngx_uint_t   nalloc;

    if (a->nelts + n > a->nalloc) {

        /* the array is full */

        {
            /* allocate a new array */

            nalloc = 2 * ((n >= a->nalloc) ? n : a->nalloc);

            new = MALLOC(nalloc * a->size);
            if (new == NULL) {
                return NULL;
            }

            memcpy(new, a->elts, a->nelts * a->size);
            a->elts = new;
            a->nalloc = nalloc;
        }
    }

    elt = (u_char *) a->elts + a->size * a->nelts;
    a->nelts += n;

    return elt;
}
