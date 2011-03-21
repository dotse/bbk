
#ifdef UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "tcp_ping.h"
#include "executor.h"
#include "valuestruct.h"
#include "tvutils.h"

void socket_close(int s) {
#ifdef UNIX
	close(s);
#endif
#ifdef WIN32
	closesocket(s);
#endif
}

void * tcp_ping_executor(void *arg) {
  struct tcp_ping_arg_struct *a;
  struct thread_arg_struct *t;
  struct sockaddr_in sa;
  struct timeval tv, tv2, timeout_tv;
  fd_set fds;
  int s = -1, i, j, res, noBlock;
  double tmp;

  t = (struct thread_arg_struct *)arg;
  a = (struct tcp_ping_arg_struct *)t->thread_args;


  t->progress = 0;
  t->completion = false;
  t->executing = true;
  t->started = true;

#ifdef SWEDISH
  strcpy(t->progress_str, "Startar TCP ping test");
#endif
#ifdef ENGLISH
  strcpy(t->progress_str, "Starting TCP ping test");
#endif

  a->values = (struct valuestruct *)
    calloc(sizeof(struct valuestruct), a->number_hosts);  
  a->no_values = a->number_hosts;

  for (i = 0; i < (int)a->number_hosts; i++) {

#ifdef SWEDISH
    sprintf(t->progress_str, "Ansluter mot %s", inet_ntoa(a->hostlist[i]));
#endif
#ifdef ENGLISH
    sprintf(t->progress_str, "Connecting to %s", inet_ntoa(a->hostlist[i]));
#endif

    /* Check if we should hibernate */
	while (t->execute == false) {
      t->executing = false;

#ifdef SWEDISH
      sprintf(t->progress_str, "TCP ping vilande");
#endif
#ifdef ENGLISH
      sprintf(t->progress_str, "TCP ping paused");
#endif

      /* Check if we should die during hibernation */
      if (t->die == true) {
        t->completion = false;
        t->executing = false;
	thread_exit();
      }
      usleep(100000);
    }
    t->executing = true;

/*    printf("Executing host %d\n", i); */

    /* create TCP socket */
    s = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
      t->error = errno;
      t->completion = false;
      t->executing = false;
	  thread_exit();
	}

    /* set non-blocking mode */
#ifdef UNIX
    noBlock = fcntl(s, F_GETFL);
    noBlock |= O_NONBLOCK;
    fcntl(s, F_SETFL, noBlock);
    if (!(fcntl(s, F_GETFL) & O_NONBLOCK)) {
	  t->error = errno;
#endif
#ifdef WIN32
    noBlock = 0;
	if (ioctlsocket( s, FIONBIO, (u_long*)&noBlock ) != 0) {
//		MessageBox(CurDlg, "Can't set non-blocking mode for send socket", TPERROR, MB_OK);
		t->error = WSAGetLastError();
#endif
      t->completion = false;
      t->executing = false;
	  socket_close(s);
      thread_exit();
	}

    /* initiate connect() to our destination */

    sa.sin_addr = a->hostlist[i];

    sa.sin_port = htons(a->portlist[i]);
    sa.sin_family = AF_INET;
    gettimeofday(&tv, NULL);
    res = connect(s, (struct sockaddr *)&sa, sizeof(sa));
    if (res == -1) {
      if (errno != EINPROGRESS) {
        continue;
      }
    }

    /* loop until connection succeeds, or */
    /* until we get an error or timeout   */

    while (1) {

      /* dying is important so check that here too */
      if (t->die == true) {
        socket_close(s);
        t->completion = false;
        t->executing = false;
		thread_exit();
	  }

      /* block 100ms waiting for connect() to complete */
      FD_ZERO(&fds);
      FD_SET(s, &fds);
      timeout_tv.tv_sec = 0;
      timeout_tv.tv_usec = 100000;
      res = select(s+1, NULL, &fds, NULL, &timeout_tv);
      gettimeofday(&tv2, NULL);

      tmp = calc_deltasec(&tv, &tv2);

      /* connect() succeeded */
      if (res == 1) {
        memcpy(&(a->values[i].timestamp), &tv2, sizeof(struct timeval));
        a->values[i].value = tmp;
        a->connects++;
        if (a->connects == 1) {
          a->max_connect_time = 
            a->min_connect_time = 
            a->avg_connect_time = tmp;
        }
        else {
          if (a->values[i].value > a->max_connect_time)
            a->max_connect_time = a->values[i].value;
          if (a->values[i].value < a->min_connect_time)
            a->min_connect_time = a->values[i].value;
          tmp = 0.0f;
          for (j = 0; j <= i; j++) {
            tmp += a->values[j].value;
          }
          a->avg_connect_time = tmp / a->connects;
        }
        break;
      }

      /* select() failed, and so did our connect() */
      if (res == -1) {
        break;
      }
      /* timeout */
      if (tmp >= ((double)(a->timeout_ms) / 1000.0))
        break;
    }
    t->progress = ((i+1) * 100) / a->number_hosts;

  }

#ifdef ENGLISH
  strcpy(t->progress_str, "TCP test finished");
#endif
#ifdef SWEDISH
  strcpy(t->progress_str, "TCP test fardigt");
#endif
  if( s >= 0 )
	socket_close(s);
  t->progress = 100;
  t->completion = true;
  t->executing = false;
  thread_exit();
  return 0;

}

void delete_tcp_ping_arg_struct(struct tcp_ping_arg_struct *p) {
  if (p->values != NULL)
    free(p->values);
  free(p);
#ifdef WIN32
  WSACleanup();
#endif
}

struct tcp_ping_arg_struct * new_tcp_ping_arg_struct() {

#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD( 1, 1 );
	err = WSAStartup( wVersionRequested, &wsaData );

	if ( err != 0 ) {
		return NULL;
	}

	if( Init_gettimeofday() == 0 )
	{
		return NULL;
	}
#endif
	
	struct tcp_ping_arg_struct *ret;
	ret = (struct tcp_ping_arg_struct *)
	calloc(sizeof(struct tcp_ping_arg_struct), 1);
	ret->timeout_ms = 10000;
	ret->number_hosts = 0;
	ret->hostlist = NULL;
	ret->portlist = NULL;
	ret->connects = 0;
	ret->no_values = 0;
	ret->values = NULL;
	return ret;
}

