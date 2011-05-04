/*
 * $Id: tptestmaster.c,v 1.2 2002/10/03 12:17:24 rlonn Exp $
 * $Source: /cvsroot-fuse/tptest/apps/unix/masterserver/tptestmaster.c,v $
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tptestmaster.c - master server program
 *
 * Written by
 *  Ragnar Lönn <prl@gatorhole.com>
 *
 * Based on earlier work by
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
#include <netdb.h>
#include <syslog.h>
#include <varargs.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>

#include "tpengine.h"
#include "tpcommon.h"

#define DENYFILE	"/tmp/tptestmaster.deny"
#define UPGRADE_URL	"http://tptest.iis.se/"
#define DATAFILE	"/etc/tptest.servers"
#define TCPPORT 	1640
#define MASTERUDPPORT	3240

char *Info[] = { \
"TPTEST - the Bandwidth tester",\
"Email: bandbreddstest@iis.se",\
"WWW: http://tptest.iis.se/",\
"" };

/* globals */
char FileName[128];
char statbuf[512];
char logbuf[600];
struct in_addr SourceAddress;
int MajorVersion, MinorVersion;
int PortNo = TCPPORT;

/* prototypes */
int ReadCTRLSock(int, int);
char * GetLine();
char * ReadLineTimeout(int, int);
int Reply(int, int, char *);
void stripcrlf(char *);


/*
 * Signal handler.
 */
static void StopServer( int Sig )
{

  syslog( LOG_INFO, "tptestmaster terminated" );
  exit( 0 );

}

/*
 * SIG_CHLD signal handler.
 */
static void SuddenDeath( int Sig )
{
  int Status;

  while( wait3(&Status, WNOHANG, (struct rusage *)0) > 0 )
    ;

}

void SendServerList( int Sock )
{
  FILE *fp;
  char buf[250], writebuf[256];
  int len;
  fp = fopen(FileName, "r");
  if (fp == NULL) {
    Reply(Sock, 410, "No service right now. Please try again later");
    return;
  }
  while (!feof(fp)) {
    memset(buf, 0, 250);
    fgets(buf, 249, fp);
    stripcrlf(buf);
    if (buf[0] == '#')
      continue;
    if (strlen(buf) < 10)
      continue;
    sprintf(writebuf, "210-%s\r\n", buf);
    len = strlen(writebuf);
    if (len < 3)
      continue;
    if (write( Sock, writebuf, len ) != len)
    {
      syslog(LOG_ERR, "write() failed");
      return;
    }
  }
  Reply(Sock, 210, " ");
  sleep(1);
}


/*
 * Perform a test session.
 */
static void DoSession( int CtrlSock )
{
  struct sockaddr_in LocSa;
  struct stat statbuf;
  int SaLen, Vmajor, Vminor;
  char str[256], *line, *p;

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
  SourceAddress = LocSa.sin_addr;

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

  /*
   * Get the HELO/INFO request from a client
   */
  if ((line = ReadLineTimeout(CtrlSock, 10)) == NULL)
  {
	close( CtrlSock );
	exit( 1 );
  }

  p = line;

  if (strncmp(p, "HELO ", 5) != 0)
  {
    if (strncmp(p, "INFO", 4) != 0)
    {
      Reply(CtrlSock, 510, "Syntax error");
      close( CtrlSock );
      exit( 1 );
    }
    else {
    /* Send INFO */
      char str[100];
      int i = 0;
      while (strlen(Info[i])) {
        sprintf(str, "250-%s\r\n", Info[i]);
        if (write(CtrlSock, str, strlen(str)) != strlen(str)) {
          syslog( LOG_ERR, "Error writing to socket (%m)");
          close( CtrlSock );
          exit( 1 );
        }
        i++;
      }
      Reply(CtrlSock, 250, "");
    }
  }
  else {
    char tBuf[50];
    memset(tBuf, 0, 50);
    p += 5;
    if (CopyTagField(tBuf, 49, p, "vmajor"))
	    Vmajor = atoi(tBuf);
    else
	    Vmajor = 0;
    if (CopyTagField(tBuf, 49, p, "vminor"))
	    Vminor = atoi(tBuf);
    else
	    Vminor = 0;


    if (Vmajor < MajorVersion)
    {
      sprintf(str, "You need to upgrade, Go to %s", UPGRADE_URL);
      Reply(CtrlSock, 501, str);
      usleep( 200000 );
      close( CtrlSock );
      exit( 1 );
    }

    SendServerList( CtrlSock );
  }

  usleep( 200000 );
  close( CtrlSock );
  exit( 0 );

}


/*
 * Fork a new child to deal with this session.
 */
static void ForkSession( int CtrlSock, int NewSock )
{

  if( !fork() ) {
    close( CtrlSock );
    setsid();
    DoSession( NewSock );
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
int main(int ac, char **av)
{
  struct sockaddr_in CtrlSa;
  int CtrlSock, NewSock, SaLen, ret;
  int One = 1;
  FILE *fp;
  struct timeval tv;
  fd_set fds;
#ifndef SOLARIS
  int i;
#endif


  if (ac < 2)
    strcpy(FileName, DATAFILE);
  else
    strcpy(FileName, av[1]);

  if ((fp = fopen(FileName, "r")) == NULL)
  {
    printf("Failed to open data file %s, exiting.\n", FileName);
    exit( 1 );
  }
  fclose(fp);

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
  if( ioctl( CtrlSock, FIONBIO, &One ) != 0 ) {
    printf( "Can't set non-blocking option, errno : %d\n", errno );
    close( CtrlSock );
    exit( 1 );
  }   

  memset( &CtrlSa, 0, sizeof CtrlSa );
#ifdef HAS_SINLEN
  CtrlSa.sin_len = sizeof( struct sockaddr_in );
#endif
  CtrlSa.sin_family = AF_INET;
  CtrlSa.sin_addr.s_addr = INADDR_ANY;
  CtrlSa.sin_port =  htons( PortNo );
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
  signal( SIGPIPE, SIG_IGN );
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
  openlog( "tptestmaster", LOG_NDELAY | LOG_PID, LOG_LOCAL0 );
  syslog( LOG_INFO, "starting tptestmaster" );
  /*
   * Eternal loop. Process received test sessions.
   */
  while( 1 ) {
    SaLen = sizeof CtrlSa;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    FD_ZERO(&fds);
    FD_SET(CtrlSock, &fds);
    ret = select(FD_SETSIZE, &fds, NULL, NULL, &tv);
    if (ret > 0) {
        if (FD_ISSET(CtrlSock, &fds)) {
          NewSock = 
		accept( CtrlSock, (struct sockaddr *)(&CtrlSa), &SaLen);
          if( NewSock >= 0 )
            ForkSession( CtrlSock, NewSock );
        }
    }
  }
}
