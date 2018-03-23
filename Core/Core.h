#ifndef CORE_UTIL_H
#define CORE_UTIL_H

#include <stdint.h>

#ifdef __linux__
#include<sys/time.h>
#include<sys/resource.h>
#include<unistd.h>
#endif



#include "log.h"
#include "MemoryUtil.h"

#include "os_util.h"
#include "time_util.h"
#include "socket_util.h"
#include "Lock/atomic.h"

#define  NGX_OK         0
#define  NGX_ERROR      0xFFFFFFFF

#include "Queue/ngx_array.h"
#include "Queue/ngx_list.h"
#include "Queue/ngx_queue.h"
#include "Queue/ngx_radix_tree.h"
#include "Queue/ngx_rbtree.h"

typedef ngx_rbtree_key_t      ngx_msec_t;
typedef ngx_rbtree_key_int_t  ngx_msec_int_t;

#define NGX_INT32_LEN   (sizeof("-2147483648") - 1)
#define NGX_INT64_LEN   (sizeof("-9223372036854775808") - 1)

#if (NGX_PTR_SIZE == 4)
#define NGX_INT_T_LEN   NGX_INT32_LEN
#define NGX_MAX_INT_T_VALUE  2147483647

#else
#define NGX_INT_T_LEN   NGX_INT64_LEN
#define NGX_MAX_INT_T_VALUE  9223372036854775807
#endif

#define NGX_MAX_SIZE_T_VALUE 0xFFFFFFFF
#define NGX_MAX_OFF_T_VALUE	0xFFFFFFFF
#define NGX_MAX_TIME_T_VALUE 0xFFFFFFFF
#define NGX_MAX_UINT32_VALUE 0xFFFFFFFF

#define LF     (u_char) '\n'
#define CR     (u_char) '\r'
#define CRLF   "\r\n"

#define ngx_pid_t uint32_t
typedef uint32_t u_int;
// typedef uint32_t u_long;

#define ngx_min min
#define ngx_cdecl

#define  NGX_AGAIN EAGAIN
#define  NGX_BUSY  EBUSY

#include "ngx_string.h"

#endif
