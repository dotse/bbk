#include "TcpWindowSize.h"

#include <stdio.h>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
typedef int SOCKLEN_T;
#endif

#ifdef UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket(s) close(s)
#define SOCKLEN_T socklen_t
#endif

char * TcpWindowSize::GetStr() {
	static char ret_str[100];
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
		sprintf(ret_str, "Failed to read TCP window size");
	else {
		int optlen = sizeof(int);
		int sendbuf = 0;
		getsockopt(sock, SOL_SOCKET, SO_SNDBUF,
			   (char*)&sendbuf, (SOCKLEN_T*)&optlen);
		sprintf(ret_str, "TCP Window size: %d\n", sendbuf);
		closesocket(sock);
	}
	return ret_str;
}
