#ifndef NGX_TYPE_H
#define NGX_TYPE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>

#include "memory_util.h"
//typedef

typedef uint8_t u_char;
typedef uint32_t u_int;
#ifndef __linux__
typedef uint32_t u_long;
#endif

typedef uint32_t ngx_uint_t;
typedef int32_t ngx_int_t;

#define ngx_pid_t uint32_t

#ifdef _WIN32
	#define NGX_WIN32 1
	typedef int ssize_t;
	#ifndef _OFF_T_DEFINED
		#define _OFF_T_DEFINED
		typedef long _off_t;
		#if !__STDC__
			typedef _off_t off_t;
		#endif
	#endif
#else
#include <sys/resource.h>
#endif

typedef ngx_uint_t      ngx_msec_t;
typedef ngx_int_t  		ngx_msec_int_t;

//string
//
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

//func

#define ngx_min min
#define ngx_cdecl

//error

#define  NGX_OK         0
#define  NGX_ERROR      0xFFFFFFFF
#define  NGX_AGAIN EAGAIN
#define  NGX_BUSY  EBUSY

//define

#define ngx_inline inline

#if defined(WIN32)
	#if !defined(__cplusplus)
		#define inline __inline
	#endif
#endif

#endif
