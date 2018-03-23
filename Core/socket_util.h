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
#include <ws2tcpip.h>
#include <errno.h>

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

int socket_init();
int socket_nonblocking(SOCKET sock);
int socket_blocking(SOCKET sock);
int socket_keepalive_(SOCKET socket,int keepcnt,int keepidle,int keepintvl);
int socket_keepalive(SOCKET socket);
int socket_sendbuf(SOCKET socket,int size);
int socket_recvbuf(SOCKET socket,int size);
int socket_linger(SOCKET socket,int onoff,int linger);
int socket_sendtimeout(SOCKET socket, int timeout);
int socket_recvtimeout(SOCKET socket, int timeout);



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
