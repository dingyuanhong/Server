#ifndef OBJECT_H
#define OBJECT_H

#include "../Core/core.h"

typedef struct object_s;

typedef void(*object_close_pt)(struct object_s *ev);

typedef struct object_s{
	void* data;
	object_close_pt close;
}object_t;

static inline object_t * object_create(void *data, object_close_pt handle)
{
	object_t * obj = MALLOC(sizeof(object_t));
	obj->data = data;
	obj->close = handle;
	return obj;
}

static inline void object_destroy(object_t ** obj_ptr){
	if(obj_ptr == NULL) return;
	object_t* obj = &obj_ptr;
	if (obj == NULL)return;
	if (obj->close != NULL)
	{
		obj->close(obj);
	}
	FREE(obj);
	*obj_ptr = NULL;
}

#endif
