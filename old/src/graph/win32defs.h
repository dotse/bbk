#ifndef __win32typedef_h
#define __win32typedef_h

#include <winsock2.h>

typedef unsigned _int64 uint64_t;

typedef struct in_addr in_addr_t;

#define EINPROGRESS WSAEINPROGRESS
#define usleep(x) Sleep(x/1000)
#define close(sock) closesocket(sock)

#endif



