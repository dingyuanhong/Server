#ifndef LOOPQUEUE_H
#define LOOPQUEUE_H

#include "../Core/core.h"

typedef struct loopqueue_s
{
	uint8_t *data;
	int r_index;
	int w_index;
	int size;
	int full;
}loopqueue_t;

inline void queue_init(loopqueue_t * c,int size)
{
	c->data = MALLOC(size);
	c->size = size;
	c->full = 0;
	c->r_index = 0;
	c->w_index = 0;
}

inline void queue_delete(loopqueue_t * c)
{
	if(c != NULL) FREE(c->data);
	c->data  =NULL;
}

inline void * queue_w(loopqueue_t *c)
{
	if(c->full == 1)
	{
		return NULL;
	}
	return c->data + c->w_index;
}

inline int queue_wsize(loopqueue_t *c)
{
	if(c->full == 0)
	{
		if(c->r_index == c->w_index)
		{
			return c->size - c->w_index;
		}
		else if(c->r_index < c->w_index){
			return c->size - c->w_index;
		}else{
			return c->r_index - c->w_index;
		}
	}else{
		return 0;
	}
}

inline void queue_wpush(loopqueue_t *c,int count)
{
	if(c->full == 0)
	{
		if(c->w_index >= c->r_index)
		{
			if(c->size - c->w_index < count)
			{
				c->w_index = c->size;
				LOGE("w_push too much size:r:%d w:%d c:%d\n",c->r_index,c->w_index,count);
			}else{
				c->w_index += count;
			}
			if(c->w_index == c->size)
			{
				c->w_index = 0;
			}
			if(c->w_index == c->r_index)
			{
				c->full = 1;
			}
		}
		else if(c->w_index < c->r_index)
		{
			if((c->r_index - c->w_index) < count)
			{
				c->w_index = c->r_index;
				LOGE("w_push too much size:r:%d w:%d c:%d\n",c->r_index,c->w_index,count);
			}else{
				c->w_index += count;
			}
			if(c->w_index == c->r_index)
			{
				c->full = 1;
			}
		}
	}else{
		LOGE("w_push full cicle:r:%d w:%d c:%d\n",c->r_index,c->w_index,count);
	}
}

inline void* queue_r(loopqueue_t *c)
{
	if(c->full == 0)
	{
		if(c->r_index == c->w_index)
		{
			return NULL;
		}
	}
	return c->data + c->r_index;
}

inline int queue_rsize(loopqueue_t *c)
{
	if(c->full == 0)
	{
		if(c->r_index == c->w_index)
		{
			return 0;
		}
		else if(c->r_index < c->w_index){
			return c->w_index - c->r_index;
		}else{
			return c->size - c->r_index;
		}
	}else{
		return c->size - c->r_index;
	}
}

inline void queue_rpush(loopqueue_t *c,int count)
{
	if(c->full == 0)
	{
		if(c->w_index == c->r_index)
		{
			LOGE("r_push too much size:r:%d w:%d c:%d\n",c->r_index,c->w_index,count);
		}
		else
		if(c->w_index > c->r_index)
		{
			if(c->w_index - c->r_index < count)
			{
				c->r_index = c->w_index;
				LOGE("r_push too much size:r:%d w:%d c:%d\n",c->r_index,c->w_index,count);
			}else{
				c->r_index += count;
			}
		}
		else if(c->w_index < c->r_index)
		{
			if((c->size - c->r_index) < count)
			{
				c->r_index = c->size;
				LOGE("r_push too much size:r:%d w:%d c:%d\n",c->r_index,c->w_index,count);
			}else{
				c->r_index += count;
			}
			if(c->r_index == c->size)
			{
				c->r_index = 0;
			}
		}
	}else{
		if(c->size - c->r_index < count)
		{
			c->r_index = c->size;
			LOGE("r_push too much size:r:%d w:%d c:%d\n",c->r_index,c->w_index,count);
		}else{
			c->r_index += count;
		}
		if(c->r_index == c->size)
		{
			c->r_index = 0;
		}
		c->full = 0;
	}
}

#endif
