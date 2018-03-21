#ifndef M_SOCKET_UTIL_H
#define M_SOCKET_UTIL_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <mstcpip.h>

#define close closesocket
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

typedef int SOCKET;

#endif

#include "LogUtil.h"


#ifdef _WIN32
inline static int socket_init()
{
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (ret != 0) return -1;
	return 0;
}
#else
inline static int socket_init()
{
	return 0;
}
#endif



#ifdef _WIN32
inline static  int socket_nonblocking(SOCKET sock)
{
	unsigned long ul=1;
	int ret=ioctlsocket(sock,FIONBIO,(unsigned long *)&ul);
	if(ret==SOCKET_ERROR)
	{
		LOGE("ioctlsocket FIONBIO failed.%d",WSAGetLastError());
	}
	return ret;
}
inline static  int socket_blocking(SOCKET sock)
{
	unsigned long ul=0;
	int ret=ioctlsocket(sock,FIONBIO,(unsigned long *)&ul);
	if(ret==SOCKET_ERROR)
	{
		LOGE("ioctlsocket FIONBIO failed.%d",WSAGetLastError());
	}
	return ret;
}

inline static int socket_keepalive(SOCKET socket,int keepcnt,int keepidle,int keepintvl)
{
	int keep_alive = 1;
    int ret = setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&keep_alive, sizeof(keep_alive));
    if (ret == SOCKET_ERROR)
    {
        LOGE("setsockopt SO_KEEPALIVE failed:%d\n", WSAGetLastError());
        return -1;
    }

    struct tcp_keepalive in_keep_alive = {0};
    unsigned long ul_in_len = sizeof(struct tcp_keepalive);
    struct tcp_keepalive out_keep_alive = {0};
    unsigned long ul_out_len = sizeof(struct tcp_keepalive);
    unsigned long ul_bytes_return = 0;

    in_keep_alive.onoff = 1;
    in_keep_alive.keepaliveinterval = keepintvl*1000;
    in_keep_alive.keepalivetime = keepidle*1000;

    ret = WSAIoctl(socket, SIO_KEEPALIVE_VALS, (LPVOID)&in_keep_alive, ul_in_len,
                          (LPVOID)&out_keep_alive, ul_out_len, &ul_bytes_return, NULL, NULL);
    if (ret == SOCKET_ERROR)
    {
        LOGE("WSAIoctl SIO_KEEPALIVE_VALS failed:%d\n", WSAGetLastError());
        return -1;
    }
    return 0;
}
inline static int socket_keepalive(SOCKET socket)
{
	return socket_keepalive(socket,5,1,1);
}
#else
inline static int  socket_nonblocking(SOCKET sock)
{
	int  opts = fcntl(sock,F_GETFL);
	if (opts < 0 )
	{
		LOGE( "fcntl(sock,GETFL) failed.%d\n",opts );
		return -1;
	}
	opts  =  opts | O_NONBLOCK;
	if (fcntl(sock,F_SETFL,opts) < 0 )
	{
		LOGE( "fcntl(sock,SETFL,opts)\n" );
		return -1;
	}
	return 0;
}
inline static  int socket_blocking(SOCKET sock)
{
	int  opts = fcntl(sock,F_GETFL);
	if (opts < 0 )
	{
		LOGE( "fcntl(sock,GETFL) failed.%d\n",opts );
		return -1;
	}
	opts  =  opts & ~O_NONBLOCK;
	if (fcntl(sock,F_SETFL,opts) < 0 )
	{
		LOGE( "fcntl(sock,SETFL,opts)\n" );
		return -1;
	}
	return 0;
}
inline static int socket_keepalive_(SOCKET socket,int keepcnt,int keepidle,int keepintvl)
{
    socklen_t optlen = sizeof(int);
    int optval = 1;
    int  ret = setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
	if(ret != 0)
	{
		LOGE("setsockopt SO_KEEPALIVE failed:%d\n",errno);
		return -1;
	}
#ifdef SOL_TCP
	//重复检查次数
    optval = keepcnt;
    ret = setsockopt(socket, SOL_TCP, TCP_KEEPCNT, &optval, optlen);
	if(ret != 0)
	{
		LOGE("setsockopt SO_KEEPALIVE failed:%d\n",errno);
		return -1;
	}
	//检查空闲
    optval = keepidle;
    ret = setsockopt(socket, SOL_TCP, TCP_KEEPIDLE, &optval, optlen);
	if(ret != 0)
	{
		LOGE("setsockopt SO_KEEPALIVE failed:%d\n",errno);
		return -1;
	}
	//检查间隔
    optval = keepintvl;
    ret = setsockopt(socket, SOL_TCP, TCP_KEEPINTVL, &optval, optlen);
	if(ret != 0)
	{
		LOGE("setsockopt SO_KEEPALIVE failed:%d\n",errno);
		return -1;
	}
#endif
	return 0;
}
inline static int socket_keepalive(SOCKET socket)
{
	return socket_keepalive_(socket,3,1,1);
}
#endif



inline static  int socket_sendbuf(SOCKET socket,int size){
	return setsockopt(socket,SOL_SOCKET,SO_SNDBUF,(const char*)&size,sizeof(int));
}
inline static int socket_recvbuf(SOCKET socket,int size){
	return setsockopt(socket,SOL_SOCKET,SO_RCVBUF,(const char*)&size,sizeof(int));
}
inline static int socket_linger(SOCKET socket,int onoff,int linger){
	struct linger    l;
	l.l_onoff = onoff;
	l.l_linger = linger;
    return setsockopt(socket, SOL_SOCKET, SO_LINGER,(const char *)&l, sizeof(struct linger));
}
inline static int socket_sendtimeout(SOCKET socket, int timeout){
	return setsockopt(socket,SOL_SOCKET,SO_SNDTIMEO,(const char *)&timeout,sizeof(timeout));
}
inline static int socket_recvtimeout(SOCKET socket, int timeout){
	return setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,(const char *)&timeout,sizeof(timeout));
}

#ifndef _WIN32
#define NGX_HAVE_SOCKET_UNIX 1
#endif

#if (NGX_HAVE_SOCKET_UNIX)
#include <sys/un.h>
#endif

#define TCP "tcp"
#define UDP  "udp"
#define UNIX "unix"
#define TCP6 "tcp6"
#define UDP6 "udp6"

#define TYPE_TCP 0
#define TYPE_UDP 1
#define TYPE_UNIX 2
#define TYPE_TCP6 4
#define TYPE_UDP6 8

int socket_type(const char * type);
SOCKET socket_object(int Type);
int socket_size(int Type);
struct sockaddr* socket_addr(int Type,const char * addr);
SOCKET socket_bind(const char * type,const char * addr);
SOCKET socket_connect(const char * type,const char * addr,int nonblocking);

#endif
