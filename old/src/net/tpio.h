/*
 * $Id: tpio.h,v 1.3 2002/09/16 14:10:42 rlonn Exp $
 * $Source: /cvsroot/tptest/engine/tpio.h,v $
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tpio.h - I/O function prototypes
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

#ifndef _TPIO_H_
#define _TPIO_H_

/* ----------------------------------------------------------- tpio.h ---- *\

  This file (tpio.h) contains prototypes for the functions the engine 
  (tpengine.c) needs in order to perform TPTEST bandwidth measurements.

  If you intend to port TPTEST to a new platform, the functions described
  here are the ones you need to provide to make things tick. The file 
  containing the functions should be named tpio_platform.c where 'platform'
  is a string representing your platform. Like e.g. "linux" or "win32".
  
  Below you'll find an explanation of what every function does.
  The functions are listed in alphabetical order (most of them).

\* ----------------------------------------------------------------------- */


int		AcceptClient(TPEngine * engp, int sockInx);
int		CheckCtrlOpen(TPEngine *);
int		ConsumeCtrlData(TPEngine *);
int		ConsumeTCPTestData(TPEngine *);
int		ConsumeUDPData(TPEngine *);
int		ContinueConnectTCP(TPEngine *);
void		ClearCtrlReply(TPEngine *);
void		CloseAllSockets(TPEngine *);
void		DeleteSessComm(TPEngine *);
int		InitConnectTCP(TPEngine *, int);
int		InitSessComm(TPEngine *);
int		InitTCPSock(TPEngine *, int);
int		InitUDP(TPEngine *);
int		QueueCtrlData(TPEngine *, char *);
int		Rand32();
int		SendCtrlData(TPEngine *);
int		SendNATPacket(TPEngine *);
int		SendTCPData(TPEngine *, char *, int, int);
int		SendUDPPacket(TPEngine *, int);
int		InitNameLookup(TPEngine *);
int		ContinueNameLookup(TPEngine *);

#ifdef NO_GETTIMEOFDAY
extern void		gettimeofday(struct timeval *, void *);
#endif




/* ----------------------------------------------------- AcceptClient ---- *\
	int AcceptClient(TPEngine * engp, int sockInx)

	Executed by RunServerContext() to accept an incoming client TCP 
	connection.

	If sockInx == TP_SOCKINX_CTRL AcceptClient() should:

		1.	Check for a pending connection on the TCP control socket

		2.	When a connection is detected and accepted, AcceptClient() must
			set engp->tcpCtrlConnectComplete = 1 to signal to the engine 
			that a connection has been established. AcceptClient() must also 
			store the remote host's IP address in engp->hostIP and the remote
			host's TCP port number in engp->hostCtrlPort

	If sockInx == TP_SOCKINX_DATA AcceptClient() should:

		1.	Check for a pending connection on the TCP data socket

		2.	When a connection is detected and accepted, AcceptClient() must
			set engp->tcpDataConnectComplete = 1 to signal to the engine that
			a connection has been established. AcceptClient() must also store
			the remote host's TCP port number in engp->hostTCPDataPort

	Return values:

		AcceptClient() should return 0 unless an error occurs, in which case
		it should return a non-zero error code and also set engp->ioError to 
		value that reflects the kind of error that occurred.
\* ----------------------------------------------------------------------- */





/* ---------------------------------------------------- CheckCtrlOpen ---- *\
	int CheckCtrlOpen(TPEngine * engp)

	Executed by the engine to determine whether the control TCP connection
	is still open or not.

	Return values:
	
		CheckCtrlOpen should return 1 if a TCP control connection is open
		and 0 (zero) if one isn't.
\* ----------------------------------------------------------------------- */





/* -------------------------------------------------- ConsumeCtrlData ---- *\
	int	ConsumeCtrlData(TPEngine * engp)

	Executed repeatedly by the engine to read more data from the TCP control
	socket until a full line ending with CR+LF has been read.

	ConsumeCtrlData() should read data from the TCP control socket, if 
	there is data to read, and store it in a temporary buffer until a 
	complete line ending with the characters CR (carriage return, ASCII 13)
	and LF (linefeed, ASCII 10) has been read. When such a line is 
	encountered, ConsumeCtrlData() should store the whole line, except for 
	the CR and LF characters and with a terminating NULL character, in 
	engp->ctrlMessage and set the variable engp->ctrlMessageComplete = 1 to 
	signal to the engine that a control command line has been received from 
	the remote peer.

	Caution:

		-	Be sure not to copy more than REPLYBUFSIZE characters into 
			engp->ctrlMessage

		-	Don't forget to NULL-terminate engp->ctrlMessage

		-	Don't forget that a read operation on the TCP control socket
			may give you a whole line, ending with CR+LF, *and* another
			line or part of another line. The code must handle this. Look
			at the implementation of ConsumeCtrlData() in tpio_win32.c

	Return values:

		ConsumeCtrlData() should return 0 unless an error occurs, in which
		case it should return your non-zero error code of choice and set
		engp->ioError to some appropriate value. 
\* ----------------------------------------------------------------------- */





/* ----------------------------------------------- ConsumeTCPTestData ---- *\
	int	ConsumeTCPTestData(TPEngine * engp)

	Executed repeatedly during a TCP test on the receiving side to read TCP
	test data on the TCP data socket.

	ConsumeTCPData() should:
	
		1.	Check if any data is available on the TCP data socket.

		2.	If so, ConsumeTCPTestData() should try to read engp->packBufSize
			bytes and store them in engp->packetBuf

		If more than 0 (zero) bytes were read from the socket, 
		ConsumeTCPTestData() should also do 3-5 below:

		3.	Check if engp->stats.BytesRecvd == 0 (zero) in which case this is
			the first time we see test data on the socket and that means
			means ConsumeTCPData() should store the current time in 
			engp->stats.StartRecv

		4.	Add the number of bytes received to engp->stats.BytesRecvd

		5.	Store the current time in engp->stats.StopRecv


	Return values:

		ConsumeTCPTestData() should return 0 unless an error occurs, in which
		case it should return your non-zero error code of choice and set
		engp->ioError to some appropriate value. 
\* ----------------------------------------------------------------------- */






/* --------------------------------------------------- ConsumeUDPData ---- *\
	int	ConsumeUDPData(TPEngine * engp)

\* ----------------------------------------------------------------------- */






/* ----------------------------------------------------- SendStatLine ---- *\


\* ----------------------------------------------------------------------- */

/* ----------------------------------------------------- SendStatLine ---- *\


\* ----------------------------------------------------------------------- */

/* ----------------------------------------------------- SendStatLine ---- *\


\* ----------------------------------------------------------------------- */

/* ----------------------------------------------------- SendStatLine ---- *\


\* ----------------------------------------------------------------------- */

/* ----------------------------------------------------- SendStatLine ---- *\


\* ----------------------------------------------------------------------- */

/* ----------------------------------------------------- SendStatLine ---- *\


\* ----------------------------------------------------------------------- */

/* ----------------------------------------------------- SendStatLine ---- *\


\* ----------------------------------------------------------------------- */

/* ----------------------------------------------------- SendStatLine ---- *\


\* ----------------------------------------------------------------------- */

/* ----------------------------------------------------- SendStatLine ---- *\


\* ----------------------------------------------------------------------- */

/* ----------------------------------------------------- SendStatLine ---- *\


\* ----------------------------------------------------------------------- */


#endif

