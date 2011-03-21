/*
 * $Id: tpio_unix.h,v 1.3 2002/09/16 14:10:42 rlonn Exp $                                               
 * $Source: /cvsroot/tptest/os-dep/unix/tpio_unix.h,v $                                       
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tpio_unix.h - header file
 *
 * Written by
 *  Ragnar Lönn <prl@gatorhole.com>
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

#ifndef _TPIO_SOLARIS_H_
#define TPIO_SOLARIS_H_

#define XFERBUF_SIZE	(66000)
#define REPLYBUF_SIZE	(4096)
#define CR				('\015')
#define LF				('\012')

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
	char sendBuf[REPLYBUF_SIZE * 2];
	int sendBufPos, sendBufCnt;
	char * lookupBuf;
} WSInfo;


#endif
