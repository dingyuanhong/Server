#ifndef CORE_UTIL_H
#define CORE_UTIL_H

#include <stdint.h>
#include <errno.h>

#ifdef __linux__
#include<sys/time.h>
#include<sys/resource.h>
#include<unistd.h>
#endif

#if defined(WIN32)
#include <winsock2.h>
#include <Windows.h>
#endif

#include "log.h"
#include "MemoryUtil.h"
#include "type.h"

#include "os_util.h"
#include "time_util.h"
#include "socket_util.h"
#include "Lock/atomic.h"

#include "Queue/ngx_array.h"
#include "Queue/ngx_list.h"
#include "Queue/ngx_queue.h"
#include "Queue/ngx_radix_tree.h"
#include "Queue/ngx_rbtree.h"

typedef ngx_rbtree_key_t      ngx_msec_t;
typedef ngx_rbtree_key_int_t  ngx_msec_int_t;

#include "ngx_string.h"

#endif
