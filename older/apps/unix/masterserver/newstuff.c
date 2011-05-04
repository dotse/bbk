/*
 * $Id: newstuff.c,v 1.3 2002/10/03 12:17:24 rlonn Exp $
 * $Source: /cvsroot-fuse/tptest/apps/unix/masterserver/newstuff.c,v $
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * newstuff.c - master server program support functions
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>

char linebuf[1024];
int line_state = 0;
int line_len = 0;
FILE *dbg = NULL;
char fname[30];

void stripcrlf(char *s) {
  char *p;
  p = s + (strlen(s) - 1);
  if (*p == 13 || *p == 10) {
    *p = '\0';
    return stripcrlf(s);
  }
}

int Reply(int CtrlSock, int Code, char *str)
{
  char writestr[256];
  sprintf(writestr, "%d %s\r\n", Code, str);
  if (write(CtrlSock, writestr, strlen(writestr)) != strlen(writestr))
  {
    syslog(LOG_ERR, "write() failed");
    return 0;
  }
  return 1;
}


/*
 * Read one more byte into linebuf until we see CR+LF
 * in which case we don't read anymore until the line is
 * fetched by GetLine(). Returns -1 if the line is too long,
 * -2 in case select() fails and -3 in case read() does.
 */

int ReadCTRLSock(int TCPSock, int mstimeout)
{
  fd_set Fds;
  int i;
  struct timeval tv;
  if (line_state == 2)
    return 2;

  if (line_len >= 1023) {
    syslog( LOG_ERR, "Control command too long (>1023 bytes)" );
    line_state = line_len = 0;
    return -1;
  }

  FD_ZERO( &Fds );
  FD_SET( TCPSock, &Fds );
  tv.tv_sec = 0;
  tv.tv_usec = mstimeout * 1000;
  
  if( ( i = select( FD_SETSIZE, &Fds, NULL, NULL, &tv ) ) < 0 ) {
    syslog( LOG_ERR, "select failed (%m)" );
    return -2;
  }

  if( FD_ISSET( TCPSock, &Fds ) ) {
      i = read( TCPSock, &linebuf[ line_len ], 1);
      if (i <= 0) 
        return -3;

#ifdef DEBUGFILE
      if (dbg == NULL)
        dbg = fopen(fname, "a");

      if (dbg != NULL) {
        if (isprint(linebuf[line_len]))
          fprintf(dbg, "[%d]%c ", linebuf[line_len], linebuf[line_len]);
        else
          fprintf(dbg, "[%d]. ", linebuf[line_len]);
        fclose(dbg);
        dbg = NULL;
      }
#endif

      line_len += i;
      switch (line_state)
      {
        case 0:
          /* CR */
          if (linebuf[line_len - 1] == 13)
            line_state = 1;
          break;
        case 1:
          /* LF */
          if (linebuf[line_len - 1] == 10)
            line_state = 2;
          else
            line_state = 0;
      }
  }
  else
    return 0;

  if (line_state == 2)
    return 2;

  return 1;

}
    

/*
 * Return the whole line if it is finished, and reset line_state
 * and line_len, otherwise return NULL
 */

char * GetLine()
{
  if (line_state == 2)
  {
    line_state = line_len = 0;
    return linebuf;
  }
  return NULL;
}


char * ReadLineTimeout(int Sock, int timeout)
{
  char *line = NULL;
  int i;
  for (i = 0; i < timeout; i++)
  {
    while (ReadCTRLSock(Sock, 999) == 1);
    if ((line = GetLine()) != NULL)
      break;
  }
  return line;
}
