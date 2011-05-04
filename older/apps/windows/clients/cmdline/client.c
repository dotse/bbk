/*
 * $Id: client.c,v 1.1 2004/04/07 13:23:20 rlonn Exp $
 * $Source: /cvsroot-fuse/tptest/apps/windows/clients/cmdline/client.c,v $
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * client.c - TPTEST 3.0 client
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
#ifdef UNIX
#include <syslog.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include "tpengine.h"
#include "tpcommon.h"
#include "tpclient.h"
#include "tpio.h"

#ifdef __WIN32
#define strcasecmp  strcmpi
#endif

/* globals */
int verbosity = 0;
#ifdef UNIX
int syslog_verbosity = 0;
#endif
int no_output = 0;
int is_v4 = 0;
int use_delay = 0;
int repeat = 1;
char output_text[400];

/* prototypes */
void usage(char *);
void log_error(char *);
void log_text(int);
LONG_LONG timediff(struct timeval *, struct timeval *);
void ReportResults(int, TPEngine *, time_t *, time_t *);
#ifndef UNIX
int getopt(int, char *const *, const char *);
#endif

void usage(char *s) {
  printf("Usage: tptestclient [options] <-m mode> <parameters> <address> <port>\n");
  printf("\n");
  printf("options: <> = required argument, [] = optional argument\n");
  printf("  -b <local address>        Bind to local address/interface\n");
  printf("  -n <repetitions>          Repeat test n number of times (0 = infinite)\n");
  printf("  -v <verbosity>            Set verbosity level\n");
#ifdef UNIX
  printf("  -s <verbosity>            Set syslog verbosity level\n");
#endif
  printf("  -e <email>                Set email for TPTEST/Statistik\n");
  printf("  -p <password>             Set password for TPTEST/Statistik\n");
  printf("  -d <delaytime>            Set delaytime between repeated tests\n");
  printf("  -S <sendbuf size>         Set size of socket send buffer\n");
  printf("  -R <recvbuf size>         Set size of socket receive buffer\n");
  printf("  -t                        No text output\n");
  printf("\n");
  printf("test modes & parameters:\n");
  printf("  udp-send | us             UDP send to server\n");
  printf("    parameters: <testtime> <bitrate>\n");
  printf("    parameters: <testtime> <packetsize> <packets/sec>\n");
  printf("  udp-receive | ur          UDP receive from server\n");
  printf("    parameters: <testtime> <bitrate>\n");
  printf("    parameters: <testtime> <packetsize> <packets/sec>\n");
  printf("  udp-fdx | uf              UDP full duplex\n");
  printf("    parameters: <testtime> <bitrate>\n");
  printf("    parameters: <testtime> <packetsize> <packets/sec>\n");
  printf("  tcp-send | ts             TCP send to server\n");
  printf("    parameters: <max testtime> <bytes to send>\n");
  printf("  tcp-receive | tr          TCP receive from server\n");
  printf("    parameters: <max testtime> <bytes to receive>\n");
  printf("  tcp-send-auto | tsa       TCP auto send to server\n");
  printf("  tcp-receive-auto | tra    TCP auto receive from server\n");
  printf("  tcp-auto | ta             TCP auto (auto send + auto receive)\n");
  printf("\n");
  if (strlen(s)) {
    printf("%s\n", s);
  }
  printf("\n");
  exit(1);
}


int main(int argc, char **argv) {

  TPEngine *engp;
  int selectedMode = CLM_NONE;
  int succeeds = 0;
  int fails = 0;
  int not_checked = 1;
  int delay = 30;
  int ch;
  double tmp;
#ifdef __WIN32
  DWORD sleeptime;
#else
  struct timespec sleeptime;
#endif
  time_t starttime, stoptime;
  char email[100], password[100];

  extern char *optarg;
  extern int optind;

  /* 0.5 sec wait between automatic tests */
#ifdef __WIN32
  sleeptime = 500;
#else
  sleeptime.tv_sec = 0;
  sleeptime.tv_nsec = 500000000;
#endif

  /* create engine context */
  engp = CreateContext();

  /* check command line arguments */
  while ((ch = getopt(argc, argv, "m:b:v:s:n:e:p:d:S:R:t")) != -1) {
    switch (ch) {
      case 'm':
        if (strcasecmp(optarg, "udp-send")==0 ||  strcasecmp(optarg, "us")==0)
          selectedMode = CLM_UDP_SEND;
        else if (strcasecmp(optarg, "udp-receive")==0 || strcasecmp(optarg, "ur")==0)
          selectedMode = CLM_UDP_RECV;
        else if (strcasecmp(optarg, "udp-full-duplex")==0 || strcasecmp(optarg, "uf")==0)
          selectedMode = CLM_UDP_FDX;
        else if (strcasecmp(optarg, "tcp-send")==0 || strcasecmp(optarg, "ts")==0)
          selectedMode = CLM_TCP_SEND;
        else if (strcasecmp(optarg, "tcp-receive")==0 || strcasecmp(optarg, "tr")==0)
          selectedMode = CLM_TCP_RECV;
        else if (strcasecmp(optarg, "tcp-send-auto")==0 || strcasecmp(optarg, "tsa")==0)
          selectedMode = CLM_AUTO_TCP_SEND;
        else if (strcasecmp(optarg, "tcp-receive-auto")==0 || strcasecmp(optarg, "tra")==0)
          selectedMode = CLM_AUTO_TCP_RECV;
        else if (strcasecmp(optarg, "tcp-auto")==0 || strcasecmp(optarg, "ta")==0)
          selectedMode = CLM_AUTO_TCP;
	    else {
          /* error, no mode supplied */
          usage("Error: no test mode supplied");
        }
        break;
      case 'b':
        if (inet_addr(optarg) != INADDR_NONE)
          engp->myLocalAddress.s_addr = inet_addr(optarg);
        else {
          /* error - invalid IP address */
          usage("Error: invalid IP address argument for -b option");
        }
        break;
      case 'S':
        engp->socket_sndbuf = atoi(optarg);
        if (engp->socket_sndbuf == 0) {
          usage("Error: invalid socket send buffer size\n");
        }
        break;
      case 'R':
        engp->socket_rcvbuf = atoi(optarg);
        if (engp->socket_rcvbuf == 0) {
          usage("Error: invalid socket receive buffer size\n");
        }
        break;
      case 't':
        no_output = 1;
        break;
      case 'e':
        strncpy(email, optarg, 99);
        email[99] = '\0';
        is_v4 = 1;
        break;
      case 'd':
        delay = atoi(optarg);
        use_delay = 1;
        break;
      case 'p':
        strncpy(password, optarg, 99);
        password[99] = '\0';
        is_v4 = 1;
        break;
      case 'n':
        repeat = atoi(optarg);
        if (repeat == 0 && optarg[0] != '0') {
          /* error. non-number argument */
          usage("Error: invalid argument to -n option");
        }
        break;
      case 'v':
        verbosity = atoi(optarg);
        if (verbosity == 0 && optarg[0] != '0') {
          /* error - missing argument */
          usage("Error: invalid argument to -v option");
        }
        break;
      case 's':
#ifdef UNIX
        syslog_verbosity = atoi(optarg);
        if (syslog_verbosity == 0 && optarg[0] != '0') {
          /* error - missing argument */
          usage("Error: invalid argument to -s option");
        }
#else
          usage("Error: option -s only available on Unix systems");
#endif
        break;
      case '?':
      default:
        usage("Error: command line syntax error");
    }
  }
  argc -= optind;
  argv += optind;

  /* check test params for individual tests */
  switch (selectedMode) {
    case CLM_UDP_SEND:
    case CLM_UDP_RECV:
    case CLM_UDP_FDX:
      /* determine test params */
      if (argc == 4) {
        engp->sessionTime = atoi(argv[0]);
        engp->bitsPerSecond = atoi(argv[1]);
	strncpy(engp->hostName, argv[2], TP_HOST_NAME_SIZE);
        engp->hostCtrlPort = atoi(argv[3]);
        RecalculatePPSSZ(engp);
      }
      else if (argc == 5) {
        engp->sessionTime = atoi(argv[0]);
        engp->packetSize = atoi(argv[1]);
        engp->packetsPerSecond = atoi(argv[2]);
	strncpy(engp->hostName, argv[3], TP_HOST_NAME_SIZE);
        engp->hostCtrlPort = atoi(argv[4]);
      }
      /* check that we have necessary values */
      if (engp->sessionTime == 0) 
        usage("Error: no test session time set");
      if (engp->bitsPerSecond == 0) {
        if (engp->packetsPerSecond == 0 || engp->packetSize == 0)
          usage("Error: no bitrate (or packet size + packet rate) set");
      }
      break;
    case CLM_TCP_SEND:
    case CLM_TCP_RECV:
      if (argc == 4) {
        engp->sessionMaxTime = atoi(argv[0]);
        engp->tcpBytes = atoi(argv[1]);
	strncpy(engp->hostName, argv[2], TP_HOST_NAME_SIZE);
        engp->hostCtrlPort = atoi(argv[3]);
      }
      if (engp->sessionMaxTime == 0)
        usage("Error: no max time set for test session");
      if (engp->tcpBytes == 0)
        usage("Error: number of TCP bytes to transfer not set");
      break;
    case CLM_AUTO_TCP_SEND:
    case CLM_AUTO_TCP_RECV:
    case CLM_AUTO_TCP:
      if (argc == 2) {
	strncpy(engp->hostName, argv[0], TP_HOST_NAME_SIZE);
        engp->hostCtrlPort = atoi(argv[1]);
      }
      break;
    default:
      /* shouldn't happen */
      usage("Error: unknown test mode");
  }

  if (argc < 2) {
    /* error - need server and server port as commandline args */
    usage("Error: need server address and control port");
  }

  if (engp->hostCtrlPort == 0) {
    /* error - invalid server port argument */
    usage("Error: invalid server control port argument");
  }

  /* check server address argument */
  if (inet_addr(engp->hostName) == INADDR_NONE) {
    struct hostent * hent;
    hent = gethostbyname(engp->hostName);
    if (hent == NULL) {
      log_error("Error: hostname lookup failed");
      exit(1);
    }
    engp->hostIP.s_addr = ((struct in_addr *)(hent->h_addr))->s_addr;
  }
  else 
    engp->hostIP.s_addr = inet_addr(engp->hostName);

  engp->tpMode = CLM_NONE;

#ifdef UNIX
  /* init syslog, if we want that facility */
  if (syslog_verbosity) {
    openlog("tptestclient", LOG_CONS | LOG_PID, LOG_USER);
  }
#endif

  if (is_v4) {
    delay = 30;
    use_delay = 1;
  }

  /* ********************************* */
  /* Main loop. May run multiple tests */
  /* ********************************* */

  while (1) {

    time(&starttime);

    /* Inner main loop. This loop runs individual tests or auto-tests */

    while (1) {

      /* use AdvanceTest() to set test params and new test mode */
      engp->tpMode = AdvanceTest(engp, selectedMode, engp->tpMode, 0);
      if (engp->tpMode == CLM_NONE)
        break;

      /* initiate new test */
      if (StartClientContext(engp) != 0) {
        log_error("Error: StartClientContext() failed");
        exit(1);
      }

      not_checked = 1;

      if (engp->tpMode == CLM_TCP_SEND || engp->tpMode == CLM_TCP_RECV) {
        sprintf(output_text, "Server: %s:%u  Test:%d  Time:%u  Maxtime:%u  Bytes: %u\n",
          inet_ntoa(engp->hostIP), engp->hostCtrlPort, 
          (int)engp->tpMode, (unsigned int)engp->sessionTime, 
          (unsigned int)engp->sessionMaxTime, (unsigned int)engp->tcpBytes);
      }
      else {
        sprintf(output_text, "Server: %s:%u  Test:%d  Time:%u  Maxtime:%u  Bitrate: %s\n",
          inet_ntoa(engp->hostIP), engp->hostCtrlPort, 
          (int)engp->tpMode, (unsigned int)engp->sessionTime, 
          (unsigned int)engp->sessionMaxTime, Int32ToString(engp->bitsPerSecond));
      }
      log_text(2);

      /* run test until finished or an error occurs */
      while (1) {
        if (engp->state == CLSM_FAILED) {
          /* Backoff algorithm to avoid overloading the servers. */
          /* If we fail more than 2 consecutive times, we increase */
          /* the delay between tests. If we succeed more than two */
          /* consecutive times, we decrease the delay between tests */
          /* (down to a minimum of 30 seconds) */
          if (is_v4) {
            succeeds = 0;
            if (++fails > 2) {
              delay += 30;
              fails = 0;
            }
          }
          sprintf(output_text, "Test failed. Failcode:%d  Ioerror:%d\n", 
            (int)engp->failCode, (int)engp->ioError);
          log_text(0);
          break;
        }
        else if (engp->state == CLSM_COMPLETE) {
          /* more backoff stuff */
          if (is_v4) {
            fails = 0;
            if (++succeeds > 2) {
              delay -= 30;
              if (delay < 30)
                delay = 30;
              succeeds = 0;
            }
          }
          break;
        }
        else if (engp->state == CLSM_TESTLOOP && not_checked) {
          not_checked = 0;
          if (engp->socket_sndbuf != 0) {
            sprintf(output_text, "Wanted SO_SNDBUF: %d   Actual SO_SNDBUF: %d\n",
              engp->socket_sndbuf, engp->cur_socket_sndbuf); log_text(2);
          }
          if (engp->socket_rcvbuf != 0) {
            sprintf(output_text, "Wanted SO_RCVBUF: %d   Actual SO_RCVBUF: %d\n",
              engp->socket_rcvbuf, engp->cur_socket_rcvbuf); log_text(2);
          }
        }
        RunClientContext(engp);
      }

      if (engp->state == CLSM_COMPLETE && 
         (selectedMode != CLM_UDP_SEND && 
          selectedMode != CLM_UDP_RECV && 
          selectedMode != CLM_UDP_FDX) ) {
        tmp = (engp->stats.BytesRecvd * 8.0) / 
               timediff(&engp->stats.StartRecv, &engp->stats.StopRecv);
        sprintf(output_text, "Received %u/%u bytes in %0.2f seconds.\n",
          (unsigned int)engp->stats.BytesRecvd, (unsigned int)engp->tcpBytes, 
	  (double)timediff(&engp->stats.StartRecv, &engp->stats.StopRecv) / 1000000.0);
        log_text(2);
      }

      /* sleep 0.5 seconds before starting next test, if any */
#ifdef __WIN32
      Sleep(sleeptime);
#else
      nanosleep(&sleeptime, NULL);
#endif

    }

    /* note when this test stopped */

    time(&stoptime);

    /* Update starting values for TCP tests so future tests will find */
    /* optimal value for tcpBytes quicker */

    if (engp->bestTCPRecvRate > 0.0)
      engp->start_tcprecv_bytes = (UINT32)(engp->bestTCPRecvRate * 20);
    if (engp->bestTCPSendRate > 0.0)
      engp->start_tcpsend_bytes = (UINT32)(engp->bestTCPSendRate * 20);

    /* report results */
    ReportResults(selectedMode, engp, &starttime, &stoptime);

    /* perform more tests or quit? */
    if (repeat != 0) {
      if (--repeat <= 0)
        break;
    }

    /* perform more tests */
    engp->tpMode = CLM_NONE;
    engp->bestTCPRecvRate = 0.0f;
    engp->bestTCPSendRate = 0.0f;
    engp->bestUDPRecvRate = 0.0f;
    engp->bestUDPSendRate = 0.0f;

    if (use_delay) {
      sprintf(output_text, "Sleeping %d seconds until next test...\n", delay); log_text(2);
#ifdef UNIX
      sleep(delay);
#else
      Sleep(delay * 1000);
#endif
    }

  }

  return 0;

}


void log_text(int level) {
  if (no_output) return;
#ifdef UNIX
  if (syslog_verbosity >= level)
    syslog(LOG_NOTICE, output_text);
#endif
  if (verbosity >= level)
    printf(output_text);
}


void ReportResults(int selectedMode, TPEngine *engp, time_t * starttime, time_t * stoptime) {
  int throughput;
  LONG_LONG recvtime;
  struct tm *tmPnt;

  sprintf(output_text, "Test results:\n");  log_text(1);
  sprintf(output_text, "-------------\n");  log_text(1);
  sprintf(output_text, "Server: %s:%d\n", inet_ntoa(engp->hostIP), engp->hostCtrlPort);  log_text(1);
  sprintf(output_text, "Test:   %d\n", selectedMode);  log_text(1);

  if (selectedMode == CLM_UDP_SEND || selectedMode == CLM_UDP_RECV ||
      selectedMode == CLM_TCP_SEND || selectedMode == CLM_TCP_RECV ||
      selectedMode == CLM_UDP_FDX) {
    sprintf(output_text, "Time:	%lu            Timelimit:    %lu\n",
      engp->sessionTime, engp->sessionMaxTime);  log_text(1);
  }

  sprintf(output_text, "Test started: %s", ctime(starttime));  log_text(1);
  sprintf(output_text, "Test ended:   %s", ctime(stoptime));  log_text(1);

  /* report results from an auto test (series of tests) */

  if (selectedMode == CLM_AUTO_TCP || selectedMode == CLM_AUTO_TCP_SEND) {
    /* report best TCP SEND results */
    sprintf(output_text, "TCP Send: %d bps (%s)\n",
      (int)(engp->bestTCPSendRate * 8.0), Int32ToString((int)(engp->bestTCPSendRate * 8.0)));
    log_text(0);
  }
  if (selectedMode == CLM_AUTO_TCP || selectedMode == CLM_AUTO_TCP_RECV) {
    /* report best TCP RECV results */
    sprintf(output_text, "TCP Recv: %d bps (%s)\n",
      (int)(engp->bestTCPRecvRate * 8.0), Int32ToString((int)(engp->bestTCPRecvRate * 8.0)));
    log_text(0);
  }
  if (selectedMode == CLM_AUTO_TCP_SEND || selectedMode == CLM_AUTO_TCP_RECV ||
      selectedMode == CLM_AUTO_TCP) {
    return;
  }

  /* report results from an individual test */

  if (selectedMode == CLM_TCP_SEND || selectedMode == CLM_TCP_RECV) {
    sprintf(output_text, "TCP Bytes: %lu\n", engp->tcpBytes);  log_text(1);
  }
  else {
    sprintf(output_text, "# of packets: %lu\n", engp->nPackets);  log_text(1);
    sprintf(output_text, "Packetsize:   %lu\n", engp->packetSize);  log_text(1);
  }

  tmPnt = localtime( (const time_t *)&(engp->stats.StartSend.tv_sec) );
  sprintf(output_text, "Send start: %04d-%02d-%02d %02d:%02d:%02d.%03ld\n",
    tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
    tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec,
    engp->stats.StartSend.tv_usec / 1000L );  log_text(1);
	
  tmPnt = localtime( (const time_t *)&(engp->stats.StopSend.tv_sec) );
  sprintf(output_text, "Send stop : %04d-%02d-%02d %02d:%02d:%02d.%03ld\n",
    tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
    tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec,
    engp->stats.StopSend.tv_usec / 1000L );  log_text(1);

  tmPnt = localtime( (const time_t *)&(engp->stats.StartRecv.tv_sec) );
  sprintf(output_text, "Recv start: %04d-%02d-%02d %02d:%02d:%02d.%03ld\n",
    tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
    tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec,
    engp->stats.StartRecv.tv_usec / 1000L );  log_text(1);

  tmPnt = localtime( (const time_t *)&(engp->stats.StopRecv.tv_sec) );
  sprintf(output_text, "Recv stop : %04d-%02d-%02d %02d:%02d:%02d.%03ld\n",
    tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
    tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec,
    engp->stats.StopRecv.tv_usec / 1000L );  log_text(1);

  if (selectedMode == CLM_UDP_SEND || selectedMode == CLM_UDP_RECV || selectedMode == CLM_UDP_FDX) {
    sprintf(output_text, "Packets sent:     %lu\n", engp->stats.PktsSent);  log_text(1);
    sprintf(output_text, "Packets received: %lu\n", engp->stats.PktsRecvd);  log_text(1);
    sprintf(output_text, "Packets lost:     %lu (%0.2f%%)\n", engp->stats.PktsSent - engp->stats.PktsRecvd, 
      ((float)(engp->stats.PktsSent - engp->stats.PktsRecvd) / (float)engp->stats.PktsSent) * 100.0);  
    log_text(1);
    sprintf(output_text, "Packets unsent:   %lu\n", engp->stats.PktsUnSent);  log_text(1);
    sprintf(output_text, "OO Packets:       %lu\n", engp->stats.ooCount);  log_text(1);
    if (selectedMode == CLM_UDP_FDX) {
      if (engp->stats.nRoundtrips > 0) {
        sprintf(output_text, "Max roundtrip: %0.3fms\n", 
          (double)engp->stats.MaxRoundtrip / 1000.0);  log_text(1);
        sprintf(output_text, "Min roundtrip: %0.3fms\n", 
          (double)engp->stats.MinRoundtrip / 1000.0);  log_text(1);
        sprintf(output_text, "Avg roundtrip: %0.3fms\n", 
          ((double)engp->stats.TotalRoundtrip / (double)engp->stats.nRoundtrips) / 1000.0);  log_text(1);
      }
    }
  }
  sprintf(output_text, "Bytes sent: %" LONG_LONG_PREFIX "d\n", engp->stats.BytesSent); log_text(1);
  sprintf(output_text, "Bytes rcvd: %" LONG_LONG_PREFIX "d\n", engp->stats.BytesRecvd); log_text(1);
  recvtime = timediff(&engp->stats.StartRecv, &engp->stats.StopRecv);
  if (recvtime > 0) 
    throughput = (int)((double)(engp->stats.BytesRecvd * 8)/((double)recvtime / 1000000.0));
  else
    throughput = 0;
  sprintf(output_text, "Throughput: %d bps (%s)\n", (int)throughput, Int32ToString((int)throughput));
  log_text(0);
}

LONG_LONG timediff(struct timeval * tv1, struct timeval * tv2) {
  LONG_LONG t1, t2;
  t1 = (LONG_LONG)tv1->tv_sec * (LONG_LONG)1000000 + 
       (LONG_LONG)tv1->tv_usec;
  t2 = (LONG_LONG)tv2->tv_sec * (LONG_LONG)1000000 + 
       (LONG_LONG)tv2->tv_usec;
  return t1 > t2 ? t1 - t2 : t2 - t1;
}

void log_error(char *str) {
  fprintf(stderr, "%s\n", str);
#ifdef UNIX
  if (syslog_verbosity)
    syslog(LOG_ERR, "%s\n", str);
#endif
}
