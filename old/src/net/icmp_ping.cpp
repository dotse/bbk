//
// PING.C -- Ping program using ICMP and RAW Sockets
//

#include <stdio.h>
#include <stdlib.h>

#ifdef UNIX
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include "util.h"
#endif

#ifdef __WIN32
#include <winsock2.h>
#define pthread_exit _endthreadex
typedef int socklen_t;
#endif

#ifdef UNIX
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr_in * LPSOCKADDR_IN;
typedef struct sockaddr * LPSOCKADDR;
typedef struct sockaddr SOCKADDR;
typedef char * LPCSTR;
typedef char * LPSTR;
typedef int SOCKET;
typedef int DWORD;
typedef struct hostent HOSTENT;
typedef struct hostent * LPHOSTENT;
#define SOCKET_ERROR (-1)
#endif

#define PACKETSIZE (100)

#include "icmp_ping.h"
#include "executor.h"
#include "tvutils.h"

// Internal Functions
int Ping(struct icmp_ping_arg_struct *as);
int WaitForEchoReply(SOCKET s, unsigned int ms_timeout);
u_short in_cksum(u_short *addr, int len);

// ICMP Echo Request/Reply functions
int SendEchoRequest(SOCKET, LPSOCKADDR_IN, ECHOREQUEST *, int);
int RecvEchoReply(SOCKET, LPSOCKADDR_IN, u_char *, struct timeval *);

void delete_icmp_ping_arg_struct(struct icmp_ping_arg_struct *p) {
  if (p->values != NULL)
    free(p->values);

  free(p);

#ifdef WIN32
  WSACleanup();
#endif

}

struct icmp_ping_arg_struct * new_icmp_ping_arg_struct() {
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

	struct icmp_ping_arg_struct * ret;
	ret = (struct icmp_ping_arg_struct *)
	malloc(sizeof(struct icmp_ping_arg_struct));
	ret->num_packets = 20;
	ret->packetsize = 64;
	ret->packet_interval_ms = 200;
	ret->packet_timeout_ms = 5000;
	ret->number_hosts = 0;
	ret->hostlist = NULL;
	ret->responding_hosts = 0;
	ret->packets_sent = 0;
	ret->packets_received = 0;
	ret->rtt_max = -1.0;
	ret->rtt_min = -1.0;
	ret->rtt_avg = -1.0;
	ret->values = NULL;
	return ret;
}


void * icmp_ping_executor(void *arg) {
  SOCKET rawSocket;
  struct sockaddr_in saDest;
  struct sockaddr_in saSrc;
  struct timeval sendTime;
  struct timeval tvNow;
  struct timeval tvHostFinished;
  struct icmp_ping_arg_struct *as;
  struct thread_arg_struct *ta;
  struct timeval tvNextSend;
  ECHOREQUEST * echoReq;
  int sz_echoreq, sz_data;
  double tot;
  u_char cTTL;
  int nHost;
  int nRet;
  unsigned int hostpkts, hostpkts_received;
  int bFirstval = 1;

  ta = (struct thread_arg_struct *)arg;
  as = (struct icmp_ping_arg_struct *)ta->thread_args;

  ta->progress = 0;
  ta->completion = false;
  ta->executing = true;
  ta->started = true;

#ifdef ENGLISH
  strcpy(ta->progress_str, "Starting ICMP test");
#endif
#ifdef SWEDISH
  strcpy(ta->progress_str, "Startar ICMP test");
#endif

  if (as->packetsize < (sizeof(ECHOREQUEST) + sizeof(IPHDR) - 1))
    as->packetsize = sizeof(ECHOREQUEST) + sizeof(IPHDR) - 1;
  sz_data = as->packetsize - sizeof(ECHOREQUEST) - sizeof(IPHDR) + 1;
  sz_echoreq = as->packetsize - sizeof(IPHDR);

  /*
    printf("Datasz=%d, Allocating %d bytes for echo request packet\n",
    sz_data, sz_echoreq);
    fflush(stdout);
  */

  echoReq = (ECHOREQUEST *)malloc(sz_echoreq);

  // Fill in some data to send
  for (nRet = 0; nRet < sz_data; nRet++)
    echoReq->cData[nRet] = ' '+(nRet%30);

  // allocate space for our results
  as->no_values = as->number_hosts * as->num_packets;
  as->values = (struct valuestruct *)
    calloc(as->no_values, sizeof(struct valuestruct));

  // Create a Raw socket
#ifdef WIN32
  rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
#endif
#ifdef MACOSX
  rawSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
#endif
  if (rawSocket == SOCKET_ERROR) 
    {
      ta->error = errno;
      ta->completion = false;
      ta->executing = false;
      thread_exit();
    }

  tot = 0.0;
  for (nHost = 0; nHost < (int)as->number_hosts; nHost++) {

    int bResponded = 0;

    // Setup destination socket address
    saDest.sin_addr = as->hostlist[nHost];
    saDest.sin_family = AF_INET;
    saDest.sin_port = 0;

    // Tell the user what we're doing
    /*
      printf("\nPinging %s with %d bytes of data:\n",
      inet_ntoa(saDest.sin_addr),
      as->packetsize);
    */

#ifdef ENGLISH
<<<<<<< .mine
    sprintf(ta->progress_str, "Pinging %s with %d %d-byte packets",
	    inet_ntoa(saDest.sin_addr), as->num_packets, as->packetsize);
=======
    sprintf(ta->progress_str, "Pinging %s with %d %d-byte packets",
	    inet_ntoa(saDest.sin_addr.s_addr), as->num_packets, as->packetsize);
>>>>>>> .r1379
#endif
#ifdef SWEDISH
    sprintf(ta->progress_str, "Pingar %s med %d %d-byte datapaket",
	    inet_ntoa(saDest.sin_addr), as->num_packets, as->packetsize);
#endif


    gettimeofday(&tvNextSend, NULL);

    // Ping multiple times
    hostpkts = hostpkts_received = 0;
    tvHostFinished.tv_sec = tvHostFinished.tv_usec = 0;

    while (1) 
      {
	struct timeval tvTimeout;
	if (ta->die == true) {
	  ta->completion = false;
	  ta->executing = false;
	  thread_exit();
	}

	int valpos = nHost * as->num_packets + hostpkts_received;

	/* Check if we should hibernate */
	while (ta->execute == false) {
	  ta->executing = false;
#ifdef ENGLISH
	  strcpy(ta->progress_str, "ICMP test paused");
#endif
#ifdef SWEDISH
	  strcpy(ta->progress_str, "ICMP test vilande");
#endif
	  /* Check if we should die during hibernation */
	  if (ta->die == true) {
	    ta->completion = false;
	    ta->executing = false;
	    thread_exit();
	  }
	  usleep(100000);
	}
	ta->executing = true;

	// update progress counter
	ta->progress = (as->packets_sent * 100) /
	  (as->number_hosts * as->num_packets);

	gettimeofday(&tvNow, NULL);
	if (tvcompare(&tvNow, &tvNextSend) < 0) {
	  // if it is not yet time to send something
	  double deltasec = calc_deltasec(&tvNow, &tvNextSend);
	  tvTimeout.tv_sec = (int)(deltasec);
	  tvTimeout.tv_usec = (int)( (deltasec - (float)((int)deltasec)) * 1000000.0 );
	  nRet = WaitForEchoReply(rawSocket, (int)(deltasec * 1000.0));
	  if (nRet == SOCKET_ERROR)
	    {
	      // give up??
	      /*
		gettimeofday(&(as->values[valpos].timestamp), NULL);
		as->values[valpos].value = -1.0;
		continue;
	      */
	    }
	  else if (nRet)
	    {
	      nRet = RecvEchoReply(rawSocket, &saSrc, &cTTL, &sendTime);
	      if (nRet == as->packetsize) {
		as->packets_received++;
		hostpkts_received++;
		if (!bResponded)
		  bResponded = 1;
		// Calculate elapsed time
		gettimeofday(&(as->values[valpos].timestamp), NULL);
		as->values[valpos].value = 
		  calc_deltasec(
				&(as->values[valpos].timestamp),
				&sendTime
				);
		tot += as->values[valpos].value;
		if (bFirstval) {
		  bFirstval = 0;
		  as->rtt_max = as->rtt_min = as->values[valpos].value;
		}
		else {
		  if (as->values[valpos].value > as->rtt_max)
		    as->rtt_max = as->values[valpos].value;
		  if (as->values[valpos].value < as->rtt_min)
		    as->rtt_min = as->values[valpos].value;
		}
	      }
	    }
	  /*						
							printf("\nReply from: %s: bytes=%d time=%.4fms TTL=%d (sent:%d.%d, recvd:%d.%d)", 
							inet_ntoa(saSrc.sin_addr), 
							nRet,
							(double)as->values[valpos].value * 1000.0,
							cTTL,
							sendTime.tv_sec, sendTime.tv_usec,
							as->values[valpos].timestamp.tv_sec, as->values[valpos].timestamp.tv_usec
							);
	  */
	}

	if (hostpkts < as->num_packets) {
	  // Send ICMP echo request
	  if (SendEchoRequest(rawSocket, &saDest, echoReq, sz_echoreq) == sz_echoreq)
	    {
	      int send_valpos = nHost * as->num_packets + hostpkts;
	      gettimeofday(&(as->values[send_valpos].timestamp), NULL);
	      tvNextSend.tv_usec += (as->packet_interval_ms * 1000);
	      if (tvNextSend.tv_usec > 999999) {
		tvNextSend.tv_usec -= 1000000;
		tvNextSend.tv_sec += 1;
	      }
	      as->values[send_valpos].value = -1.0;
	      as->packets_sent++;
	      hostpkts++;
	    }
	}
	else {
	  if (hostpkts_received >= as->num_packets)
	    break;
	  gettimeofday(&tvNow, NULL);
	  if (tvHostFinished.tv_sec == 0) {
	    tvHostFinished.tv_sec = tvNow.tv_sec;
	    tvHostFinished.tv_usec = tvNow.tv_usec;
	  }
	  if ((calc_deltasec(&tvHostFinished, &tvNow) * 1000.0) > (float)as->packet_timeout_ms)
	    break;
	  tvNextSend.tv_usec += (as->packet_timeout_ms * 1000);
	  if (tvNextSend.tv_usec > 999999) {
	    tvNextSend.tv_usec -= 1000000;
	    tvNextSend.tv_sec += 1;
	  }
	}

      }
    if (bResponded)
      as->responding_hosts++;
  }

  as->rtt_avg = tot / ((double)(as->packets_received));

#ifdef __WIN32
  nRet = closesocket(rawSocket);
#else
  nRet = close(rawSocket);
#endif

#ifdef ENGLISH
  strcpy(ta->progress_str, "ICMP test finished");
#endif
#ifdef SWEDISH
  strcpy(ta->progress_str, "ICMP test fardigt");
#endif

  ta->progress = 100;
  ta->completion = true;
  ta->executing = false;
  thread_exit();

  return NULL;

}


int SendEchoRequest(SOCKET s,LPSOCKADDR_IN lpstToAddr, ECHOREQUEST * echoReq, int sz_echoreq)
{
  static u_short nId = 1;
  static u_short nSeq = 1;
  int nRet;

  // Fill in echo request
  echoReq->icmpHdr.Type	= ICMP_ECHOREQ;
  echoReq->icmpHdr.Code	= 0;
  echoReq->icmpHdr.Checksum = 0;
  echoReq->icmpHdr.ID = nId++;
  echoReq->icmpHdr.Seq = nSeq++;

  // Save tick count when sent
  gettimeofday(&(echoReq->sendTime), NULL);

  // Put data in packet and compute checksum
  echoReq->icmpHdr.Checksum = in_cksum((u_short *)echoReq, sz_echoreq);

  // Send the echo request
  nRet = sendto(s,			/* socket */
                (LPSTR)echoReq,		/* buffer */
                sz_echoreq,
                0,			/* flags */
                (LPSOCKADDR)lpstToAddr, /* destination */
                sizeof(SOCKADDR_IN));   /* address length */

  if (nRet == SOCKET_ERROR) 
    return -1;
  return (nRet);
}


// RecvEchoReply()
// Receive incoming data
// and parse out fields
int RecvEchoReply(SOCKET s, LPSOCKADDR_IN lpsaFrom, u_char *pTTL, struct timeval *sendTime) 
{
  ECHOREPLY echoReply;
  int nRet;
  int nAddrLen = sizeof(struct sockaddr_in);

  // Receive the echo reply	
  nRet = recvfrom(s,							// socket
                  (LPSTR)&echoReply,			// buffer
                  sizeof(ECHOREPLY),			// size of buffer
                  0,							// flags
                  (sockaddr*)lpsaFrom,			// From address
                  (socklen_t*)&nAddrLen);		// pointer to address len

  // Check return value
  if (nRet == SOCKET_ERROR) {
    return 0;
  }

  // return time sent and IP TTL
  *pTTL = echoReply.ipHdr.TTL;
  memcpy(sendTime, &(echoReply.echoRequest.sendTime), sizeof(struct timeval));
  return nRet;
}


// WaitForEchoReply()
// Use select() to determine when
// data is waiting to be read
int WaitForEchoReply(SOCKET s, unsigned int ms_timeout)
{
  struct timeval Timeout;
  fd_set readfds;

  FD_ZERO(&readfds);
  FD_SET(s, &readfds);

  Timeout.tv_sec = ms_timeout / 1000;
  ms_timeout -= (Timeout.tv_sec * 1000);
  Timeout.tv_usec = ms_timeout * 1000;  

  return(select(((int)s)+1, &readfds, NULL, NULL, &Timeout));
}


//
// Mike Muuss' in_cksum() function
// and his comments from the original
// ping program
//
// * Author -
// *	Mike Muuss
// *	U. S. Army Ballistic Research Laboratory
// *	December, 1983

/*
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
u_short in_cksum(u_short *addr, int len)
{
  register int nleft = len;
  register u_short *w = addr;
  register u_short answer;
  register int sum = 0;

  /*
   *  Our algorithm is simple, using a 32 bit accumulator (sum),
   *  we add sequential 16 bit words to it, and at the end, fold
   *  back all the carry bits from the top 16 bits into the lower
   *  16 bits.
   */
  while( nleft > 1 )  {
    sum += *w++;
    nleft -= 2;
  }

  /* mop up an odd byte, if necessary */
  if( nleft == 1 ) {
    u_short u = 0;

    *(u_char *)(&u) = *(u_char *)w ;
    sum += u;
  }

  /*
   * add back carry outs from top 16 bits to low 16 bits
   */
  sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
  sum += (sum >> 16);			/* add carry */
  answer = ~sum;				/* truncate to 16 bits */
  return (answer);
}




