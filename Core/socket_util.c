#include "log.h"
#include "socket_util.h"

#ifdef _WIN32
int socket_init()
{
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (ret != 0) return -1;
	return 0;
}
#else
int socket_init()
{
	return 0;
}
#endif


#ifdef _WIN32
	int socket_nonblocking(SOCKET sock)
	{
		unsigned long ul=1;
		int ret=ioctlsocket(sock,FIONBIO,(unsigned long *)&ul);
		if(ret==SOCKET_ERROR)
		{
			LOGE("ioctlsocket FIONBIO failed.%d",WSAGetLastError());
		}
		return ret;
	}
	int socket_blocking(SOCKET sock)
	{
		unsigned long ul=0;
		int ret=ioctlsocket(sock,FIONBIO,(unsigned long *)&ul);
		if(ret==SOCKET_ERROR)
		{
			LOGE("ioctlsocket FIONBIO failed.%d",WSAGetLastError());
		}
		return ret;
	}
	int socket_keepalive_(SOCKET socket,int keepcnt,int keepidle,int keepintvl)
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
	int socket_keepalive(SOCKET socket)
	{
		return socket_keepalive_(socket, 5, 1, 1);
	}
#else
	int  socket_nonblocking(SOCKET sock)
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
	int socket_blocking(SOCKET sock)
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
	int socket_keepalive_(SOCKET socket,int keepcnt,int keepidle,int keepintvl)
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
	int socket_keepalive(SOCKET socket)
	{
		return socket_keepalive_(socket,3,1,1);
	}
#endif



int socket_sendbuf(SOCKET socket,int size)
{
	return setsockopt(socket,SOL_SOCKET,SO_SNDBUF,(const char*)&size,sizeof(int));
}
int socket_recvbuf(SOCKET socket,int size)
{
	return setsockopt(socket,SOL_SOCKET,SO_RCVBUF,(const char*)&size,sizeof(int));
}
int socket_linger(SOCKET socket,int onoff,int linger)
{
	struct linger    l;
	l.l_onoff = onoff;
	l.l_linger = linger;
    return setsockopt(socket, SOL_SOCKET, SO_LINGER,(const char *)&l, sizeof(struct linger));
}
int socket_sendtimeout(SOCKET socket, int timeout)
{
	return setsockopt(socket,SOL_SOCKET,SO_SNDTIMEO,(const char *)&timeout,sizeof(timeout));
}
int socket_recvtimeout(SOCKET socket, int timeout)
{
	return setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,(const char *)&timeout,sizeof(timeout));
}



 int socket_type(const char * type)
{
	if(strcmp(type,TCP) == 0)
	{
		return TYPE_TCP;
	}
	if(strcmp(type,UDP) == 0)
	{
		return TYPE_UDP;
	}
	if(strcmp(type,UNIX) == 0)
	{
		return TYPE_UNIX;
	}
	if(strcmp(type,TCP6) == 0)
	{
		return TYPE_TCP6;
	}
	if(strcmp(type,UDP6) == 0)
	{
		return TYPE_UDP6;
	}
	return TYPE_TCP;
}

 SOCKET socket_object(int Type)
{
	if(Type == TYPE_TCP)
	{
		return socket(AF_INET,SOCK_STREAM,0);
	}else if(Type == TYPE_UDP){
		return socket(AF_INET,SOCK_DGRAM,0);
	}else if(Type == TYPE_TCP6){
		return socket(AF_INET6,SOCK_STREAM,0);
	}else if(Type == TYPE_UDP6){
		return socket(AF_INET6,SOCK_DGRAM,0);
	}
#if (NGX_HAVE_SOCKET_UNIX)
	else if(Type == TYPE_UNIX){
		return socket(AF_UNIX,SOCK_STREAM,0);
	}
#endif
	else {
		return -1;
	}
}

 int socket_size(int Type)
{
	if(Type == TYPE_TCP)
	{
		return sizeof(struct sockaddr_in);
	}else if(Type == TYPE_UDP){
		return sizeof(struct sockaddr_in);
	}
#if (NGX_HAVE_SOCKET_UNIX)
	else if(Type == TYPE_UNIX){
		return sizeof(struct sockaddr_un);
	}
#endif
	else if(Type == TYPE_TCP6){
		return sizeof(struct sockaddr_in6);
	}
	else if(Type == TYPE_UDP6){
		return sizeof(struct sockaddr_in6);
	}
	else {
		return 0;
	}
}

 struct sockaddr* socket_addr(int Type,const char * addr)
{
	char * dup_addr = NULL;
	char * ip = NULL;
	int port = 0;
	if(Type != TYPE_UNIX)
	{
		dup_addr = strdup(addr);
		char * ptr = strrchr(dup_addr,':');
		if(ptr != NULL){
			*ptr = 0x00;
		}else{
			free(dup_addr);
			return NULL;
		}
		ip = dup_addr;
		if(strlen(ip) == 0)
		{
			if(Type == TYPE_TCP || Type == TYPE_UDP)
			{
				ip = "0.0.0.0";
			}else{
				ip = "::";
			}
		}else if(strcmp(ip,"localhost") == 0)
		{
			if(Type == TYPE_TCP || Type == TYPE_UDP)
			{
				ip = "127.0.0.1";
			}else {
				ip = NULL;
				LOGE("%d cann't use localhost\n",Type);
			}
		}
		if(ip == NULL){
			free(dup_addr);
			return NULL;
		}
		port = atoi(ptr + 1);
		if(port <= 0)
		{
			LOGE("port not set.\n");
			free(dup_addr);
			return NULL;
		}
		// LOGD("IP:(%s) Port:(%d)\n",ip,port);
	}

	struct sockaddr* addr_ptr = NULL;

	if(Type == TYPE_TCP || Type == TYPE_UDP){
		struct sockaddr_in *addr_in = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
		addr_in->sin_family = AF_INET;
		addr_in->sin_port = htons(port);
		addr_in->sin_addr.s_addr = inet_addr(ip);
		char * rIP = inet_ntoa(addr_in->sin_addr);
		int rPort = ntohs(addr_in->sin_port);
		if(rIP == NULL || rPort != port){
			free(addr_in);
		}else{
			addr_ptr = (struct sockaddr*)addr_in;
		}
	}
	else if(Type == TYPE_TCP6 || Type == TYPE_UDP6){
		struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6*)malloc(sizeof(struct sockaddr_in6));
		addr_in6->sin6_family = AF_INET6;
		addr_in6->sin6_port = htons(port);
		inet_pton(AF_INET6, ip, &addr_in6->sin6_addr);
		addr_ptr = (struct sockaddr*)addr_in6;
	}
#if (NGX_HAVE_SOCKET_UNIX)
	else if(Type == TYPE_UNIX){
		struct sockaddr_un *addr_un = (struct sockaddr_un*)malloc(sizeof(struct sockaddr_un));
		addr_un->sun_family = AF_UNIX;
		strcpy(addr_un->sun_path, addr);
		addr_ptr = (struct sockaddr*)addr_un;
	}
#endif
	if(dup_addr != NULL){
		free(dup_addr);
	}
	return addr_ptr;
}

 SOCKET socket_bind(const char * type,const char * addr)
{
	int Type = socket_type(type);
	SOCKET s = socket_object(Type);
	if(s == -1) return s;
	struct sockaddr* addr_ptr = socket_addr(Type,addr);
	if(addr_ptr == NULL){
		close(s);
		return -1;
	}
	int ret = bind(s,addr_ptr,socket_size(Type));
	if(addr_ptr != NULL){
		free(addr_ptr);
	}
	if(ret != 0) {
		LOGE("bind (%s) errno:%d\n",addr,SOCKET_ERRNO);
		close(s);
		s = -1;
	}
	return s;
}

 SOCKET socket_connect(const char * type,const char * addr,int nonblocking)
{
	int Type = socket_type(type);
	SOCKET s = socket_object(Type);
	if(s == -1) return s;
	struct sockaddr* addr_ptr = socket_addr(Type,addr);
	if(addr_ptr == NULL){
		close(s);
		return -1;
	}
	if(nonblocking){
		socket_nonblocking(s);
	}
	int ret = connect(s,addr_ptr,socket_size(Type));
	if(addr_ptr != NULL){
		free(addr_ptr);
	}
	if(ret != 0){
		if(!(SEINPROGRESS == SOCKET_ERRNO || SEWOULDBLOCK == SOCKET_ERRNO))
		{
			LOGE("connect (%s) errno:%d\n",addr,SOCKET_ERRNO);
			close(s);
			s = -1;
		}
	}
	return s;
}
