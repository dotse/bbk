/*
 * $Id: newstuff.c,v 1.4 2005/08/17 11:12:37 rlonn Exp $
 * $Source: /cvsroot-fuse/tptest/apps/unix/server/newstuff.c,v $
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * main.c - simple client/server TPTEST application
 *
 * Written by
 *  Ragnar Lönn <prl@gatorhole.com>
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

#include "server.h"

#ifdef DEBUGFILE
FILE *dbg = NULL;
char fname[30];
#endif

extern struct textstruct *AllowHosts;
extern struct textstruct *DenyHosts;
extern int SMaxBitrate;
extern int SMaxTime;
extern int SMaxPPS;
extern int SMaxPacketsize;
extern int SMaxClients;
extern int SyslogLevel;
extern int SPortRangeStart;
extern int SPortRangeEnd;

void dumptofile(FILE * fp, void * data, int len) {
  int lineno = 0;
  int j = 0;
  char *p;
  char line[75], hex[3];
  memset(line, 32, 74);
  line[74] = '\0';
  while (j < len) {
    p = line + ((j % 16));
    if (isprint((int)(*(((char *)data)+j))))
      *p = *( ((char *)data)+j );
    else
      *p = '.';
    p = line + ((j % 16) * 3) + 25;
    sprintf(hex, "%2X", *(((unsigned char *)data)+j));
    *p++ = hex[0];
    *p = hex[1];
    j++;
    if (j % 16 == 0) {
      lineno++;
      line[73] = '\n';
      line[74] = '\0';
      fprintf(fp, "%s", line);
      memset(line, 32, 74);
    }
  }
  line[74] = '\n';
  fprintf(fp, "%s", line);
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





struct textstruct * new_textstruct()
{
  struct textstruct *p;
  p = (struct textstruct *)malloc(sizeof(struct textstruct));
  if (p == NULL)
  {
    syslog(LOG_ERR, "malloc() failed");
    exit(1);
  }
  p->text = NULL;
  p->next = NULL;
  return p;
}

int AddAllowHost(char *hname, int deny)
{
  struct textstruct *t;

  if (deny)
    t = DenyHosts;
  else
    t = AllowHosts;

  if (t == NULL) {
    if (deny)
      t = DenyHosts = new_textstruct();
    else
      t = AllowHosts = new_textstruct();
  }
  else
  {
    while (t->next != NULL)
      t = t->next;
    t->next = new_textstruct();
    t = t->next;
  }
  t->text = (char *)malloc(strlen(hname)+1);
  if (t->text == NULL)
  {
    syslog(LOG_ERR, "malloc() failed");
    exit(1);
  }
  strcpy(t->text, hname);
  return 1;
}


void stripcrlf(char *s)
{
  char *p;
  p = s + (strlen(s) - 1);
  if (*p == 13 || *p == 10)
  {
    *p = '\0';
    stripcrlf(s);
  }
}


int ReadConfigFile(char *configfile)
{
  FILE *fp;
  struct textstruct *t, *t2;
  char buf[256], varname[100], *p, *p2;
  int i, line;
  fp = fopen(configfile, "r");
  if (fp == NULL)
    return 0;
  line = 0;


  if ((t = AllowHosts) != NULL)
  {
    while (t->next != NULL)
    {
      t2 = t->next;
      if (t->text != NULL)
        free(t->text);
      free(t);
      t = t2;
    }
    if (t->text != NULL)
      free(t->text);
    free(t);
  }
  AllowHosts = NULL;

  if ((t = DenyHosts) != NULL)
  {
    while (t->next != NULL)
    {
      t2 = t->next;
      if (t->text != NULL)
        free(t->text);
      free(t);
      t = t2;
    }
    if (t->text != NULL)
      free(t->text);
    free(t);
  }
  DenyHosts = NULL;

  while (!feof(fp))
  {
    line++;
    memset(buf, 0, 256);
    fgets(buf, 255, fp);
    stripcrlf(buf);
    p = buf;

    while (isspace((int)(*p)) && *p != '\0') p++;
    /* If nothing on this line or it is a comment, ignore it */ 
    if (*p == '\0' || *p == '#')
      continue;

    /* Find and extract variable name */
    while (!isalpha((int)(*p)) && *p != '\0') p++;
    if (*p == '\0')
      continue;
    p2 = p + 1;
    while (isalpha((int)(*p2)) && *p2 != '\0') p2++;
    if (*p2 == '\0')
      continue;
    *p2 = '\0';
    strncpy(varname, p, 99);
    varname[99] = '\0';

    /* Find value */
    p = p2 + 1;
    while (isspace((int)(*p)) && *p != '\0') p++;
    if (*p == '\0')
      continue;
    p2 = p + 1;
    while (!isspace((int)(*p2)) && *p2 != '\0') p2++;
    *p2 = '\0';


    /* Set server parameters or complain */
    if (!strcasecmp(varname, "MaxBitrate"))
    {
      if ((i = atoi(p)) != 0)
        SMaxBitrate = i;
      else 
        syslog(LOG_ERR, "%s, line %d: Invalid bitrate", configfile, line);
    }
    else if (!strcasecmp(varname, "MaxTime"))
    {
      if ((i = atoi(p)) != 0)
        SMaxTime = i;
      else 
        syslog(LOG_ERR, "%s, line %d: Invalid maxtime", configfile, line);
    }
    else if (!strcasecmp(varname, "MaxPPS"))
    {
      if ((i = atoi(p)) != 0)
        SMaxPPS = i;
      else 
        syslog(LOG_ERR, "%s, line %d: Invalid maxpps", configfile, line);
    }
    else if (!strcasecmp(varname, "MaxPacketsize"))
    {
      if ((i = atoi(p)) != 0)
        SMaxPacketsize = i;
      else 
        syslog(LOG_ERR, "%s, line %d: Invalid maxpacketsize", configfile, line);
    }
    else if (!strcasecmp(varname, "MaxClients"))
    {
        SMaxClients = atoi(p);
    }
    else if (!strcasecmp(varname, "DenyHost"))
    {
      if (!AddAllowHost(p, 1))
        syslog(LOG_ERR, "%s, line %d: Failed to add denyhost \"%s\"",
          configfile, line, p);
    }
    else if (!strcasecmp(varname, "AllowHost"))
    {
      if (!AddAllowHost(p, 0))
        syslog(LOG_ERR, "%s, line %d: Failed to add allowhost \"%s\"",
          configfile, line, p);       
    }
    else if (!strcasecmp(varname, "SyslogLevel"))
    {
      if ((i = atoi(p)) != 0)
        SyslogLevel = i;
      else 
        syslog(LOG_ERR, "%s, line %d: Invalid SyslogLevel", configfile, line);
    }
    else if (!strcasecmp(varname, "PortRange"))
    {
      if ((i = atoi(p)) != 0)
      {
        char * p2 = strchr(p, '-');
        if (p2 != NULL) 
        {
          SPortRangeStart = i;
          p2++;
          if ((i = atoi(p2)) != 0)
          {
            SPortRangeEnd = i;
            continue;
          }
        }
      }
      syslog(LOG_ERR, "%s, line %d: Invalid port range supplied",
        configfile, line);
    }
    else
      syslog(LOG_ERR, "%s, line %d: Unrecognized option \"%s\"",
        configfile, line, varname);
  }
  fclose(fp);
  return 1;
}


