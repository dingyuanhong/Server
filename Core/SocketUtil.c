#include "SocketUtil.h"

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
	}else if(Type == TYPE_UNIX){
		return sizeof(struct sockaddr_un);
	}else if(Type == TYPE_TCP6){
		return sizeof(struct sockaddr_in6);
	}
#if (NGX_HAVE_SOCKET_UNIX)
	else if(Type == TYPE_UDP6){
		return sizeof(struct sockaddr_in6);
	}
#endif
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
		LOGE("bind (%s) errno:%d\n",addr,errno);
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
		if(EINPROGRESS != errno)
		{
			LOGE("connect (%s) errno:%d\n",addr,errno);
			close(s);
			s = -1;
		}
	}
	return s;
}
