/*
 * $Id: tpio_win32.h,v 1.1 2007/01/31 07:45:40 danron Exp $                                               
 * $Source: /cvsroot-fuse/tptest/tptest5/src/net/tpio_win32.h,v $                                       
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tpio_win32.h - header file
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
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA         
 * 02111-1307  USA.
 *
 */

#ifndef _TPIO_WIN32_H_
#define TPIO_WIN32_H_

#include <winbase.h>
#include <winsock2.h>
#include <time.h>

#define XFERBUF_SIZE	(66000)
#define REPLYBUF_SIZE	(4096)
#define CR				('\015')
#define LF				('\012')

int	AcceptClient(TPEngine * engp, int sockInx);
int	CheckCtrlOpen(TPEngine *);
int	ConsumeCtrlData(TPEngine *);
int	ConsumeTCPTestData(TPEngine *);
int	ConsumeUDPData(TPEngine *);
int	ContinueConnectTCP(TPEngine *);
void	ClearCtrlReply(TPEngine *);
void	CloseAllSockets(TPEngine *);
void	DeleteSessComm(TPEngine *);
int	InitConnectTCP(TPEngine *, int);
int	InitSessComm(TPEngine *);
int	InitTCPSock(TPEngine *, int);
int	InitUDP(TPEngine *);
int	QueueCtrlData(TPEngine *, char *);
int	Rand32();
int	SendCtrlData(TPEngine *);
int	SendNATPacket(TPEngine *);
int	SendTCPData(TPEngine *, char *, int, int);
int	SendUDPPacket(TPEngine *, int);
int	InitNameLookup(TPEngine *);
int	ContinueNameLookup(TPEngine *);

typedef struct WSInfo {
	SOCKET udpRecvSock, udpSendSock, tcpCtrlSock, tcpDataSock;
	SOCKET tcpServerCtrlSock, tcpServerDataSock;
	char replyBuf[REPLYBUF_SIZE];
	char xferBuf[XFERBUF_SIZE];
	int xferPos, replyPos, xferCnt, replyCnt;
	int lastErr;
	int ctrlConnOK, dataConnOK;
	int crSeen;
	int ctrlConnInProg, dataConnInProg;
	int nameLookupInProgress;
	char sendBuf[REPLYBUF_SIZE * 2];
	int sendBufPos, sendBufCnt;
	ULONG	lookupThread;
} WSInfo;

_int64 performance_frequency;
_int64 counter_start_value;
time_t	start_sec;

int Init_gettimeofday();

#endif
