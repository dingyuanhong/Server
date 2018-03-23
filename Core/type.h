#ifndef TYPE_UTIL_H
#define TYPE_UTIL_H

#include <stdint.h>

typedef uint8_t u_char;
typedef uint32_t ngx_uint_t;
typedef int32_t ngx_int_t;

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
#ifndef __linux__
typedef uint32_t u_long;
#endif

#ifdef _WIN32
typedef int ssize_t;
#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
typedef long _off_t;
#if !__STDC__
typedef _off_t off_t;
#endif
#endif
#define NGX_WIN32 1
#endif

#define ngx_min min
#define ngx_cdecl

#define  NGX_OK         0
#define  NGX_ERROR      0xFFFFFFFF
#define  NGX_AGAIN EAGAIN
#define  NGX_BUSY  EBUSY

#define ngx_inline inline

#if defined(WIN32)
#if !defined(__cplusplus)
#define inline __inline
#endif
#endif

#endif