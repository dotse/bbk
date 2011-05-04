/*
 * $Id: server.c,v 1.9 2005/08/17 11:12:37 rlonn Exp $
 * $Source: /cvsroot-fuse/tptest/apps/unix/server/server.c,v $
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * server.c - simple server TPTEST application
 *
 * Written by
 *  Ragnar Lönn <prl@gatorhole.com>
 *  Hans Nästén
 *
 * This file is part of the TPTEST system.
 * See the file LICENSE for copyright notice.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by       
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.       
 *
 * You should have received a copy of the GNU General Public License       
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
// #include <netdb.h>
#include <syslog.h>
// #include <varargs.h>

#include <sys/types.h>
#include <sys/time.h>
// #include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>

// #include <net/if.h>
// #include <netinet/in.h>
// #include <netinet/in_systm.h>
// #include <netinet/ip.h>
// #include <netinet/in.h>
// #include <netinet/udp.h>
//#include <arpa/inet.h>
#include <regex.h>

#include "tpengine.h"
#include "tpio.h"
#include "tpio_unix.h"
#include "server.h"
#include "tpcommon.h"

struct textstruct *AllowHosts;
struct textstruct *DenyHosts;
int SCurClients = 0;
int SMaxBitrate = 0;
int SMaxTime = 0;
int SMaxPPS = 0;
int SMaxPacketsize = 0;
int SMaxClients = 0;
int SyslogLevel = 0;
int SPortRangeStart = 0;
int SPortRangeEnd = 0;

struct TPEngine * engp;

char configfile[200];

#define CONFIGFILE	"/etc/tptest.conf"
#define DENYFILE	"/etc/tptest.deny"

char *testdesc[] = \
	{ "none", "UDP Full Duplex", "UDP Send", "UDP Receive", "TCP Send", "TCP Receive" };

#define M_NONE                  0
#define M_UDP_FDX               1
#define M_UDP_SEND              2
#define M_UDP_RECV              3
#define M_TCP_SEND              4
#define M_TCP_RECV              5


extern int ReadConfigFile(char *);
extern int Reply(int, int, char *);
extern int Rand32();


/*
 * Signal handler.
 */
static void StopServer( int Sig )
{

  syslog( LOG_INFO, "tptest terminated" );
  exit( 0 );

}

/*
 * SIG_CHLD signal handler.
 */
static void SuddenDeath( int Sig )
{
  int Status;
  signal( SIGCHLD, SuddenDeath );
  while( wait3(&Status, WNOHANG, (struct rusage *)0) > 0 )
    SCurClients--;
}


/*
 * We get signal!
 */
static void ReloadConfig( int Sig )
{
  syslog( LOG_INFO, "Reloading %s", configfile);
  ReadConfigFile(configfile);
  signal( SIGHUP, ReloadConfig );
}

/*
 * We get signal!
 */
static void ResetCurClients( int Sig )
{
  syslog( LOG_INFO, "Resetting SCurClients");
  SCurClients = 0;
  signal( SIGUSR1, ResetCurClients );
}



int CheckTestParams(TPEngine * engp) {

  WSInfo * w;
  int CtrlSock;
  char str[200];
  w = (WSInfo *)(engp->ctrlRefp);
  CtrlSock = w->tcpCtrlSock;

/*
  if (sPnt->MajorVersion < MajorVersion) {
    sprintf(str, "502 You need to upgrade your client. Go to %s\r\n",
      UPGRADE_URL);
    if( write( CtrlSock, str, strlen(str) ) != strlen(str) ) { 
      syslog( LOG_ERR, "Can't send reply to client %s (%m)",
	    inet_ntoa( SourceAddress ) );
    }
    usleep( 200000 );
    close( CtrlSock );
    close( RecvSock );
    close( SendSock );
    exit( 1 );
  }
*/


  if (engp->tpMode != M_TCP_SEND && engp->tpMode != M_TCP_RECV)
  {

  /*
   * Check the session variables to see nothing exceeds our
   * allowed max values
   */

  if (engp->packetsPerSecond > SMaxPPS && SMaxPPS > 0)
  {
    syslog(LOG_INFO, "Session from %s refused. They requested %ld PPS",
	inet_ntoa(engp->hostIP), engp->packetsPerSecond);
    sprintf(str, "512 Max Packets Per Second allowed is %d\r\n", SMaxPPS);
    if( write( CtrlSock, str, strlen(str) ) != strlen(str) ) { 
      syslog( LOG_ERR, "Can't send reply to client %s (%m)",
	    inet_ntoa( engp->hostIP ) );
    }
    return 0;
  }

  if (engp->packetSize > SMaxPacketsize && SMaxPacketsize > 0)
  {
    syslog(LOG_INFO, "Session from %s refused. They requested Packetsize %ld",
	inet_ntoa(engp->hostIP), engp->packetSize);
    sprintf(str, "512 Max Packetsize allowed is %d\r\n", SMaxPacketsize);
    if( write( CtrlSock, str, strlen(str) ) != strlen(str) ) { 
      syslog( LOG_ERR, "Can't send reply to client %s (%m)",
	    inet_ntoa( engp->hostIP ) );
    }
    return 0;
  }
 
  if (engp->bitsPerSecond > SMaxBitrate && SMaxBitrate > 0)
  {
    syslog(LOG_INFO, "Session from %s refused. They requested %s bitrate",
	inet_ntoa(engp->hostIP), Int32ToString(engp->bitsPerSecond));
    sprintf(str, "512 Max Bitrate allowed is %s (%d bps)\r\n", 
	Int32ToString(SMaxBitrate), SMaxBitrate);
    if( write( CtrlSock, str, strlen(str) ) != strlen(str) ) { 
      syslog( LOG_ERR, "Can't send reply to client %s (%m)",
	    inet_ntoa( engp->hostIP ) );
    }
    return 0;
  }
  
  }

  if (engp->sessionMaxTime > SMaxTime && SMaxTime > 0)
  {
    syslog(LOG_INFO, "Session from %s refused. They requested max test time %ld",
	inet_ntoa(engp->hostIP), engp->sessionMaxTime);
    sprintf(str, "512 Max Test Time allowed is %d\r\n", SMaxTime);
    if( write( CtrlSock, str, strlen(str) ) != strlen(str) ) { 
      syslog( LOG_ERR, "Can't send reply to client %s (%m)",
	    inet_ntoa( engp->hostIP ) );
    }
    return 0;
  }

  return 1;
}




/*
 * Perform a test session.
 */
static void DoSession( int CtrlSock, int toomany )
{
  struct sockaddr_in LocSa, SourceAddress;
  int SaLen, state, msRecv;
  char str[256];
  WSInfo * w;
  struct timeval tv;
  float BytesPerSecondRecv;

  struct stat statbuf;
  regex_t re;
  int allowed;
  struct textstruct * t;


  /*
   * Get the source address from the control socket.
   */
  SaLen = sizeof LocSa;
  if( getpeername( CtrlSock, (struct sockaddr *)(&LocSa), &SaLen ) != 0 ) {
    syslog( LOG_ERR, "Can't get source address from control socket (%m)" );
    close( CtrlSock );
    exit( 1 );
  }
  syslog( LOG_INFO, "Connection from %s:%d",
	  inet_ntoa( LocSa.sin_addr ), ntohs( LocSa.sin_port ) );
  SourceAddress.sin_addr = LocSa.sin_addr;

//  printf( "Connection from %s:%d\n",
//	  inet_ntoa( LocSa.sin_addr ), ntohs( LocSa.sin_port ) );


  /*
   * Too many clients?
   */
  if (toomany)
  {
    sprintf(str, "410 Max # of clients reached (%d). Try again later\r\n", SMaxClients);
    syslog(LOG_INFO, "Service denied to %s, max # of clients reached (%d)",
      inet_ntoa(LocSa.sin_addr), SMaxClients);
    if( write( CtrlSock, str, strlen(str) ) != strlen(str) ) { 
      syslog( LOG_ERR, "Can't send reply to client %s (%m)",
        inet_ntoa( SourceAddress.sin_addr ) );
    }
    usleep( 200000 );
    close( CtrlSock );
    exit( 0 );
  }

  /*
   * Check to see if it's in our allowed/deny structures
   */
  allowed = 0;
  strcpy(str, inet_ntoa(LocSa.sin_addr));

  t = AllowHosts;
  while (t != NULL)
  {
    if (regcomp(&re, t->text, REG_EXTENDED | REG_NOSUB) == 0)
    {
      if (regexec(&re, str, (size_t)0, NULL, 0) == 0)
          allowed = 1;
      regfree(&re);
      if (allowed == 1)
        break;
    }
    t = t->next;
  }
  
  /* If not explicitly allowed, check deny hosts */
  if (!allowed)
  {
    t = DenyHosts;
    while (t != NULL)
    {
      if (regcomp(&re, t->text, REG_EXTENDED | REG_NOSUB) == 0)
      {
        if (regexec(&re, str, (size_t)0, NULL, 0) == 0)
          break;
        regfree(&re);
      }
      t = t->next;
    }
    if (t != NULL)
    {
      syslog(LOG_INFO, "Connection from %s:%d refused: matches deny rule \"%s\"",
        inet_ntoa(LocSa.sin_addr), ntohs(LocSa.sin_port), t->text);
      Reply(CtrlSock, 500, "You are not allowed to use this server, sorry");
      usleep( 200000 );
      close( CtrlSock );
      exit( 0 );
    }
  }

  /*
   * Check if DENYFILE exists and if so, refuse connection
   */
  if (stat(DENYFILE, &statbuf) == 0)
  {
    syslog( LOG_INFO, "Connection from %s:%d refused: DENYFILE (%s) exists",
      inet_ntoa( LocSa.sin_addr ), ntohs( LocSa.sin_port ), DENYFILE);
    Reply(CtrlSock, 410, "Service temporarily disabled. Please try later");
    usleep( 200000 );
    close( CtrlSock );
    exit( 1 );
  }


	/* Manual StartServerContext() and other initializations */
	engp->failCode = 0;
	engp->timeLimit = 0;
	engp->active = 1;
	engp->curSend = 0;
	engp->callMeAgain = 50000;
	engp->stats.ooCount = 0;
	engp->prevPacket = 0;
	engp->local_tcp_pr_start = engp->local_udp_pr_start = SPortRangeStart;
	engp->local_tcp_pr_end = engp->local_udp_pr_end = SPortRangeEnd;
	
	engp->hostIP.s_addr = LocSa.sin_addr.s_addr;
	engp->hostCtrlPort = ntohs(LocSa.sin_port);
		
	time(&engp->startTime);
	engp->stats.PktsSent = engp->stats.PktsUnSent = engp->stats.PktsRecvd = 0;
	engp->stats.BytesSent = engp->stats.BytesRecvd = 0;
	engp->stats.MaxRoundtrip = 0;
	engp->stats.MinRoundtrip = 999999999;

	SetSMState(engp, SSM_WAITTEST);
	SetTimeLimit(engp, 10);
	engp->iAmServer = 1;
	engp->tcpCtrlConnectComplete = 1;

	srandom((unsigned int)(tv.tv_usec));
	while (engp->sessCookie == 0)
		engp->sessCookie = Rand32();
	w = (WSInfo *)(engp->ctrlRefp);
	w->tcpCtrlSock = CtrlSock;
	w->ctrlConnOK = 1;
	w->xferPos = w->replyPos = w->xferCnt = w->replyCnt = 0;



  /*
   * Send the client our hello message
   */

	ClearCtrlReply(engp);
	sprintf(str, "200 vmajor=%u;vminor=%u;cookie=%lu\r\n",
		MAJORVERSION, MINORVERSION, engp->sessCookie);
	QueueCtrlData(engp, str);
  
  /*
   *  Hand over to the test engine at SSM_WAITTEST
   *
   */     

  state = engp->state;
  while (engp->state != SSM_FAILED && engp->state != SSM_COMPLETE) {
	RunServerContext(engp);
	if (engp->state == SSM_POSTTEST) {
		// Check test parameters for validity
		if (CheckTestParams(engp))
			RunServerContext(engp);
		else 
			FailSession(engp, TPER_BADTEST);
	}
/*
	if (engp->callMeAgain != 0) 
		usleep(engp->callMeAgain);
*/
  }
  
  /*
   *  Calculate test results
   *
   */
  msRecv = ( engp->stats.StopRecv.tv_sec - engp->stats.StartRecv.tv_sec ) * 1000;
  msRecv += ( engp->stats.StopRecv.tv_usec - engp->stats.StartRecv.tv_usec ) / 1000;
  if( msRecv != 0 )
    BytesPerSecondRecv = ( (double)(engp->stats.BytesRecvd) * 1000.0 )
                              / (double)(msRecv);
  else
    BytesPerSecondRecv = 0.0;

  /*
   *  Log results to syslog, if configured to do so
   *
   */
  if (SyslogLevel >= 2) {
    int port = 0;
    switch (engp->tpMode) {
      case M_TCP_SEND:
      case M_TCP_RECV:
        port = engp->hostTCPDataPort;
        break;
      case M_UDP_SEND:
      case M_UDP_FDX:
        port = engp->hostUDPSendPort;
        break;
      case M_UDP_RECV:
        port = engp->hostUDPRecvPort;
        break;
    }
    syslog(LOG_INFO, "Client %s:%d (data port %d) finished %s test in %d.%02d seconds. Result = %s",
           inet_ntoa(engp->hostIP), engp->hostCtrlPort, port,
           testdesc[engp->tpMode], msRecv / 1000, msRecv % 1000,
           Int32ToString((int)(BytesPerSecondRecv * 8.0)));
  }
}



/*
 * Fork a new child to deal with this session.
 */
static void ForkSession( int CtrlSock, int NewSock, int toomany )
{
  if( !fork() ) {
    close( CtrlSock );
    setsid();
    DoSession( NewSock, toomany );
    close( NewSock );
    exit( 1 );
  }
  else {
    close( NewSock );
  }
}


/*
 * Server mode main loop.
 */
void ServerMode(int tcpport)
{
  struct sockaddr_in CtrlSa;
  struct linger sl;
  int CtrlSock, NewSock, SaLen;
  int One = 1;
  int i;

#ifdef PIDFILE
  FILE *fp;
#endif

  sl.l_onoff = 0;
  sl.l_linger = 0;


  /*
   * Close stdin, stdout, stderr etc
   */
  for (i = 0; i < 256; i++)
    close(i);

  /*
   * Create the control socket.
   */
  CtrlSock = socket( AF_INET, SOCK_STREAM, 0 );
  if( CtrlSock < 0 ) {
    printf( "Can't create control socket\n" );
    exit( 1 );
  }
  if( setsockopt( CtrlSock, SOL_SOCKET, SO_REUSEADDR, (char *)(&One),
                        sizeof( One ) ) != 0 ) {
    printf( "Can't set socket option, errno : %d\n", errno );
    close( CtrlSock );
    exit( 1 );
  }

  if ( setsockopt( CtrlSock, SOL_SOCKET, SO_LINGER, (char *)(&sl),
	sizeof(sl) ) != 0) {
    printf( "Can't set socket option, errno : %d\n", errno );
    close( CtrlSock );
    exit( 1 );
  }


  memset( &CtrlSa, 0, sizeof CtrlSa );
#ifdef HAS_SINLEN
  CtrlSa.sin_len = sizeof( struct sockaddr_in );
#endif
  CtrlSa.sin_family = AF_INET;
  CtrlSa.sin_addr.s_addr = INADDR_ANY;
  CtrlSa.sin_port =  htons( tcpport );
  if( bind( CtrlSock, (struct sockaddr *)(&CtrlSa), sizeof CtrlSa ) < 0 ) {
    printf( "Can't bind control socket, errno : %d\n", errno );
    close( CtrlSock );
    exit( 1 );
  }
  listen( CtrlSock, 4 );
  signal( SIGTERM, StopServer );
  signal( SIGINT, StopServer );
  signal( SIGQUIT, StopServer );
  signal( SIGCHLD, SuddenDeath );
  signal( SIGUSR1, ResetCurClients );
  signal( SIGPIPE, SIG_IGN );
  signal( SIGHUP, ReloadConfig );
  /*
   * Disconnect from controlling tty.
   */
  if( fork() ) {
    exit( 0 );
  }
#ifdef SOLARIS
  setsid();
#else
  i = open( "/dev/tty", O_RDWR );
  if( i >= 0 ) {
    ioctl( i, TIOCNOTTY, NULL );
    close( i );
  }
#endif

  if (SyslogLevel >= 1) {
    openlog( "tptest", LOG_NDELAY | LOG_PID, LOG_LOCAL0 );
    syslog( LOG_INFO, "starting tptest" );
  }

#ifdef PIDFILE
  if ((fp = fopen(PIDFILE, "w")) == NULL)
  {
    syslog(LOG_ERR, "Failed to write PIDFILE %s", PIDFILE);
    exit(1);
  }
  fprintf(fp, "%d", (int)getpid());
  fclose(fp);
#endif


  /*
   * Eternal loop. Process received test sessions.
   */
  while( 1 ) {
    SaLen = sizeof CtrlSa;
    NewSock = accept( CtrlSock, (struct sockaddr *)(&CtrlSa), &SaLen );
    if( NewSock >= 0 )
    {
      /*
       * Too many clients?
       */
      SCurClients++;
      if (SCurClients > SMaxClients && SMaxClients > 0)
        ForkSession( CtrlSock, NewSock, 1 );
      else 
        ForkSession( CtrlSock, NewSock, 0 );
    }
    else
      syslog( LOG_ERR, "Error accepting new connection (%m)" );
  }

}

void Usage(char ** av) {
	printf("Usage: %s [-p port]               tcp control port\n", 
		av[0]);
	printf("          [-f configfile]         path to tptest.conf\n");
	printf("          [-V]                    version info\n");
	printf("          [-h -?]                 this help text\n");
	printf("\n");
}

int main(int argc, char **argv) {
	int tcpport = DEFAULT_CONTROL_PORT;
	int OptErrors = 0;
	int chr;

	while( (chr = getopt( argc, argv, "p:f:Vh?" )) >= 0 ) {    
		switch( chr ) {
		case 'p':
			if (sscanf(optarg, "%d", &tcpport) != 1) {
				Usage(argv);
				exit( 1 );
			}
			break;
		case 'f':
			if (ReadConfigFile(optarg) != 1) {
				Usage(argv);
				exit(1);
			}
			strncpy(configfile, optarg, 199);
			configfile[199] = '\0';
			break;
		case 'V':
			printf( "Version : %d.%02d\n", 
				MAJORVERSION, MINORVERSION );	
			exit( 0 );
		case 'h':
		case '?':
			Usage(argv);
			exit( 0 );
		default:
			OptErrors++;
			break;    
		}
	}

	if( OptErrors ) {      
		Usage(argv);
		exit( 1 );
	}

	engp = CreateContext();
	if (engp == NULL) {
//		printf("Error creating engine context\n");
		exit(1);
	}

	ServerMode(tcpport);
	return 0;
}
