/*
 * $Id: tpio_unix.c,v 1.10 2004/03/23 17:04:28 rlonn Exp $
 * $Source: /cvsroot/tptest/os-dep/unix/tpio_unix.c,v $
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tpio_unix.c - Platform-dependent I/O routines
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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include "tpengine.h"
#include "tpio.h"
#include "tpio_unix.h"

/*
// functions defined in this file:
int AcceptClient(TPEngine *);
int CheckCtrlOpen(TPEngine *);
int	ConsumeCtrlData(TPEngine *);
int ConsumeTCPTestData(TPEngine *);
int ConsumeUDPData(TPEngine *);
int ContinueConnectTCP(TPEngine *);
void ClearCtrlReply(TPEngine *);
void CloseAllSockets(TPEngine *);
int CloseCtrlSock(TPEngine *);
void DeleteSessComm(TPEngine *);
#ifdef NO_GETTIMEOFDAY
void gettimeofday(struct timeval *, void *);
#endif
int InitConnectTCP(TPEngine *, int);
int Init_gettimeofday();
int InitSessComm(TPEngine *);
int InitTCPSock(TPEngine *, int);
int InitUDP(TPEngine *);
int QueueCtrlData(TPEngine *, char *);
int Rand32();
int SendCtrlData(TPEngine *);
int SendNATPacket(TPEngine *);
int SendTCPData(TPEngine *, char *, int, int);
int SendUDPPacket(TPEngine *, int);
*/


int InitNameLookup(TPEngine *engp) { return 0; }
int ContinueNameLookup(TPEngine *engp) { return 0; }




/* ---------------------------------------------------- SendNATPacket ---- *\

	Create and send a NAT open packet

	We send a packet from our recv socket to the server's send socket
	in order to open up NAT gateways.

\* ----------------------------------------------------------------------- */

int SendNATPacket(TPEngine *engp)
{
	WSInfo			*w;
	struct tpPacket		*tPnt;
	struct timeval		now;
	int					res;
  	char				buf[100];
	SOCKADDR_IN			toAddr;

	w = (WSInfo *) (engp->ctrlRefp);
	if (w == 0) return -1;	

	// Set up NATOPEN packet contents

//	printf("Sending NAT open packet to %s:%u\n", inet_ntoa(engp->hostIP), 
//		engp->hostUDPSendPort);
	
	memset(buf, 0, 100);
	tPnt = (struct tpPacket *) buf;
	tPnt->Header.Sequence = 0;
    tPnt->Header.ServerRecvTime.tv_sec = 0;
    tPnt->Header.ServerRecvTime.tv_usec = 0;
    tPnt->Header.ServerSendTime.tv_sec = 0;
    tPnt->Header.ServerSendTime.tv_usec = 0;
    tPnt->Header.DataSize = 0;
	tPnt->Header.Cookie = htonl(engp->sessCookie);
    gettimeofday(&now, NULL);
	tPnt->Header.ClientSendTime.tv_sec = htonl(now.tv_sec);
	tPnt->Header.ClientSendTime.tv_usec = htonl(now.tv_usec);
    
	// send the data from our receive port to server's send port

	toAddr.sin_addr.s_addr = engp->hostIP.s_addr;
	toAddr.sin_port = htons(engp->hostUDPSendPort);
	toAddr.sin_family = AF_INET;

	res = sendto(w->udpRecvSock, buf, 100, 0, (SOCKADDR *)&toAddr, sizeof (toAddr));

	if (res != 100) {
		if (res == SOCKET_ERROR)
			return errno;
		else
			return -1;
	}

	return 0;
}


int ConsumeTCPTestData(TPEngine *engp) {
	struct timeval tv, nowTV;
	fd_set fds;
	WSInfo * w;
	int res = 0;

	w = (WSInfo *)(engp->ctrlRefp);
	if (w == 0) return 0;

	tv.tv_sec = 0;
	tv.tv_usec = 100000;

	FD_ZERO(&fds);
	FD_SET(w->tcpDataSock, &fds);

	res = select(FD_SETSIZE, &fds, NULL, NULL, &tv);

	if (FD_ISSET(w->tcpDataSock, &fds)) {
		res = recv(w->tcpDataSock, engp->packetBuf, engp->packBufSize, 0);
		if (res > 0) {
			gettimeofday(&nowTV, NULL);
			engp->packetLen = res;
			if (engp->stats.BytesRecvd == 0)
				engp->stats.StartRecv = nowTV;
			engp->stats.BytesRecvd += res;
			engp->stats.StopRecv = nowTV;
		}
	}

	return 0;	
}

/* --------------------------------------------------- ConsumeUDPData ---- *\

  Read UDP packets from send and receive sockets and update counters,
  receive start/stop times, byte- and packet-counters, natOpen flag, etc. 
  in the TPEngine struct. Note that a lot of this functionality may be 
  moved to the engine later.

\* ----------------------------------------------------------------------- */

int ConsumeUDPData(TPEngine *engp)
{
	int				ret;
	int				len;
	int				fromLen;
	UINT32			rTrip;
	struct tpPacket		*tPnt;
	SOCKADDR_IN		fromAdr;
	WSInfo * w;
	struct timeval tv;
	fd_set fds;
	
	w = (WSInfo *) (engp->ctrlRefp);
	if (w == 0) return 0;				// Never allocated

	tv.tv_sec = tv.tv_usec = 0;

	
	if (engp->iAmServer) {
		if (engp->tpMode == M_UDP_FDX || engp->tpMode == M_UDP_SEND)
			tv.tv_usec = 100000;
	}
	else {
		if (engp->tpMode == M_UDP_RECV)
			tv.tv_usec = 100000;
		else if (engp->tpMode == M_UDP_FDX) {
			struct timeval nowTV;
			gettimeofday(&nowTV, NULL);
			tv.tv_sec = engp->nextSendTV.tv_sec - nowTV.tv_sec;
			tv.tv_usec = engp->nextSendTV.tv_usec - nowTV.tv_usec;
			if (tv.tv_usec < 0) {
				if (tv.tv_sec > 0) {
					tv.tv_sec -= 1;
					tv.tv_usec += 1000000;
				}
				else
					tv.tv_usec = 0;
			}
		}
	}

	FD_ZERO(&fds);
	FD_SET(w->udpRecvSock, &fds);
	FD_SET(w->udpSendSock, &fds);
	ret = select(FD_SETSIZE, &fds, NULL, NULL, &tv);

	fromLen = sizeof(fromAdr);

	while (FD_ISSET(w->udpRecvSock, &fds)) {
		len = recvfrom(w->udpRecvSock, engp->packetBuf, engp->packBufSize, 
			       0, (SOCKADDR *)&fromAdr, (socklen_t*)&fromLen);

		// We got something on our receive socket
		if (len > 0) {
			// Provide the engine with info about this packet
			engp->packetfromAdr.s_addr = fromAdr.sin_addr.s_addr;
			engp->packetfromPort = ntohs(fromAdr.sin_port);
			engp->packetLen = len;

			tPnt = (struct tpPacket *) engp->packetBuf;

//			printf("%d-byte packet from %s:%d\n",
//				len, inet_ntoa(fromAdr.sin_addr), ntohs(fromAdr.sin_port));

			// Check if this seems to be a valid data packet
			if (engp->packetfromAdr.s_addr == engp->hostIP.s_addr &&
				ntohl(tPnt->Header.Cookie) == engp->sessCookie) {

				// Check if packet out of order and if so, increase ooCount
				// *** Think about moving this code to the engine instead
				if (ntohl(tPnt->Header.Sequence) < engp->prevPacket)
					engp->stats.ooCount += 1;
				else
					engp->prevPacket = ntohl(tPnt->Header.Sequence);

				// If we haven't received anything before, set StartRecv time
				if (engp->stats.BytesRecvd == 0) 
					gettimeofday(&(engp->stats.StartRecv), NULL);

				// Always set StopRecv time
				gettimeofday(&(engp->stats.StopRecv), NULL);

				// Update receive stats
				engp->stats.PktsRecvd += 1;
				engp->stats.BytesRecvd += ( len + IP_UDP_SIZE );		 		

				// Update roundtrip time stats unless we're in server mode in which case we
				// just return the packets instead
				if (engp->tpMode == M_UDP_FDX) {
					if (engp->iAmServer != 1) {
			   			rTrip = ( engp->stats.StopRecv.tv_sec - ntohl( tPnt->Header.ClientSendTime.tv_sec ) ) * 1000000;
			   			rTrip += ( engp->stats.StopRecv.tv_usec - ntohl( tPnt->Header.ClientSendTime.tv_usec ) );
						if ( rTrip > engp->stats.MaxRoundtrip ) 
							engp->stats.MaxRoundtrip = rTrip;
				    		else if ( rTrip < engp->stats.MinRoundtrip )	// *** else if?
							engp->stats.MinRoundtrip = rTrip;
				    		engp->stats.TotalRoundtrip += rTrip;
				    		engp->stats.nRoundtrips++;
			 		}
					else {
						if (engp->packetLen > 0) {  // We got a packet
							if (engp->packetfromAdr.s_addr == engp->hostIP.s_addr) {  // And from the right host
								if (((unsigned long)(engp->sessCookie)) == ntohl(((TPPacket *)(engp->packetBuf))->Header.Cookie) &&
									engp->sessCookie != 0) {  // Packet contained the right cookie
									TPPacket *tPnt = (TPPacket *)engp->packetBuf;
									tPnt->Header.ServerRecvTime.tv_sec = htonl( engp->stats.StopRecv.tv_sec );
									tPnt->Header.ServerRecvTime.tv_usec = htonl( engp->stats.StopRecv.tv_usec );
									SendUDPDataPacket(engp);
								}
								else {
//									printf("Wrong cookie\n");
									// From the right host but with invalid cookie.
								}
							}
							else {
//								printf("Wrong host\n");
								// packet from some other host than our peer
							}
						}
					}
				}
				// Consider NAT open procedure to have succeeded
				engp->natOpen = 1;
			}
//			else
//				printf("Invalid packet\n");
		}
		FD_ZERO(&fds);
		FD_SET(w->udpRecvSock, &fds);
		tv.tv_sec = tv.tv_usec = 0;
		ret = select(FD_SETSIZE, &fds, NULL, NULL, &tv);
	}
	FD_ZERO(&fds);
	FD_SET(w->udpSendSock, &fds);
	tv.tv_sec = tv.tv_usec = 0;
	ret = select(FD_SETSIZE, &fds, NULL, NULL, &tv);

	// Check for incoming data on our *SEND* socket
	// we do this to detect NAT open packets
	while (FD_ISSET(w->udpSendSock, &fds)) {

		len = recvfrom(w->udpSendSock, engp->packetBuf, engp->packBufSize,
			       0, (SOCKADDR *)&fromAdr, (socklen_t*)&fromLen);

		if (len > 0) {
			// Provide the engine with info about this packet
			engp->packetfromAdr.s_addr = fromAdr.sin_addr.s_addr;
			engp->packetfromPort = ntohs(fromAdr.sin_port);
			engp->packetLen = len;
			tPnt = (TPPacket *)engp->packetBuf;

			// Check if it's a valid NAT open packet
			if (engp->packetfromAdr.s_addr == engp->hostIP.s_addr && 
				ntohl(tPnt->Header.Cookie) == engp->sessCookie) {
//				printf("It's a NATOPEN! Yiehaa. From %u to %u\n",
//					engp->packetfromPort, engp->myUDPSendPort);
				// We have a real NAT-open packet. Hopefully.
				engp->natOpen = 1;
				engp->hostUDPRecvPort = engp->packetfromPort;
			}
		}
		FD_ZERO(&fds);
		FD_SET(w->udpSendSock, &fds);
		tv.tv_sec = tv.tv_usec = 0;
		ret = select(FD_SETSIZE, &fds, NULL, NULL, &tv);
	}

	return 0;
}





/* --------------------------------------------------- ClearCtrlReply ---- *\

  Clear old control message. Might be renamed to ClearCtrlMessage() ***

\* ----------------------------------------------------------------------- */

void	ClearCtrlReply(TPEngine *engp)
{
	WSInfo		*w;

	w = (WSInfo *) (engp->ctrlRefp);
	if (w == 0) return;				// Never allocated

	engp->ctrlMessageComplete = 0;			// No reply seen
	w->replyCnt = w->replyPos = w->crSeen = 0;		// Nothing in line buffer
}

void DeleteSessComm(TPEngine *engp) {
	CloseAllSockets(engp);
}

void CloseAllSockets(TPEngine *engp) {

	WSInfo * w;

	w = (WSInfo *)(engp->ctrlRefp);
	if (w == 0) return;

	close(w->tcpCtrlSock);
	w->tcpCtrlSock = INVALID_SOCKET;
	close(w->tcpDataSock);
	w->tcpDataSock = INVALID_SOCKET;
	close(w->tcpServerCtrlSock);
	w->tcpServerCtrlSock = INVALID_SOCKET;
	close(w->tcpServerDataSock);
	w->tcpServerDataSock = INVALID_SOCKET;
	close(w->udpRecvSock);
	w->udpRecvSock = INVALID_SOCKET;
	close(w->udpSendSock);
	w->udpSendSock = INVALID_SOCKET;

	w->ctrlConnInProg = 0;
	w->ctrlConnOK = 0;
	w->dataConnInProg = 0;
	w->dataConnOK = 0;
	engp->tcpCtrlConnectComplete = 0;
	engp->tcpDataConnectComplete = 0;

}



// ContinueConnectTCP() continues with the current TCP connect()
// procedure in progress. It sets either tcpCtrlConnectComplete = 1
// or tcpDataConnectComplete = 1 when connected, depending on what
// socket is being connected.
//

int ContinueConnectTCP(TPEngine *engp)
{
	WSInfo *w	= 0;
	fd_set wfds, efds;
	struct timeval tv;
	SOCKET sock;

	w = (WSInfo *)(engp->ctrlRefp);

	if (w->ctrlConnInProg)
		sock = w->tcpCtrlSock;
	else if (w->dataConnInProg)
		sock = w->tcpDataSock;
	else
		return -1;

	FD_ZERO(&wfds);
	FD_ZERO(&efds);
	FD_SET(sock, &wfds);
	FD_SET(sock, &efds);
	tv.tv_sec = tv.tv_usec = 0;

	if (select(FD_SETSIZE, NULL, &wfds, &efds, &tv) == SOCKET_ERROR) {
		return errno;
	}

	if (w->ctrlConnInProg) {
		if (FD_ISSET(sock, &wfds)) {
			w->xferCnt = w->xferPos = 0;
			w->replyCnt = w->replyPos = 0;
			w->sendBufCnt = w->sendBufPos = 0;
			w->ctrlConnOK = 1;
			w->ctrlConnInProg = 0;
			engp->tcpCtrlConnectComplete = 1;
		}
		else if (FD_ISSET(sock, &efds)) {
			w->ctrlConnInProg = 0;
			return -1;
		}
	}
	else {
		if (FD_ISSET(sock, &wfds)) {
			w->dataConnOK = 1;
			w->dataConnInProg = 0;
			engp->tcpDataConnectComplete = 1;
		}
		else if (FD_ISSET(sock, &efds)) {
			w->dataConnInProg = 0;
			return -1;
		}
	}
	return 0;
}



// AcceptClient()
// Executed by RunServerContext() to accept an incoming client connect
// on either the TCP control port or the TCP data port. Sets
// tcpCtrlConnectComplete = 1 or tcpDataConnectComplete = 1 on success
// Also stores the remote host IP:portno in engp->hostIP and engp->hostCtrlPort
// or engp->hostDataPort

int AcceptClient(TPEngine *engp, int sockInx) {
       WSInfo * w;
        SOCKADDR_IN clientSa;
        SOCKET s;
        int saLen;
        fd_set fds;
        struct timeval tv;

        w = (WSInfo *)(engp->ctrlRefp);
        if (w == 0) return -1;

        saLen = sizeof(clientSa);

        if (sockInx == TP_SOCKINX_CTRL)
                s = w->tcpServerCtrlSock;
        else if (sockInx == TP_SOCKINX_DATA)
                s = w->tcpServerDataSock;

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_SET(s, &fds);        
        select(FD_SETSIZE, &fds, NULL, NULL, &tv);

        if (FD_ISSET(s, &fds)) {
	  s = accept(s, (SOCKADDR *)&clientSa, (socklen_t*)&saLen);
        }
        else
                return 0;

        if (s == INVALID_SOCKET) {
#ifdef WHATOSDOESTHIS
		if (errno != EWOULDBLOCK) 
#else
		if (errno != EINPROGRESS) 
#endif
                        return 0;
                return errno;
        }

        if (sockInx == TP_SOCKINX_CTRL) {
                w->tcpCtrlSock = s;
                engp->hostIP.s_addr = clientSa.sin_addr.s_addr;
                engp->hostCtrlPort = ntohs(clientSa.sin_port);
                engp->tcpCtrlConnectComplete = 1;
                w->xferCnt = w->xferPos = 0;
                w->replyCnt = w->replyPos = 0;
                w->sendBufCnt = w->sendBufPos = 0;
                w->ctrlConnOK = 1;
        }
	else if (sockInx == TP_SOCKINX_DATA) {
		int optlen = sizeof(int);
		w->tcpDataSock = s;
		engp->tcpDataConnectComplete = 1;
		w->dataConnOK = 1;
		if (engp->socket_sndbuf != 0) {
			setsockopt(s, SOL_SOCKET, SO_SNDBUF, 
				&engp->socket_sndbuf, optlen);
		}
		getsockopt(s, SOL_SOCKET, SO_SNDBUF,
			   &engp->cur_socket_sndbuf, (socklen_t*)&optlen);
		if (engp->socket_rcvbuf != 0) {
			setsockopt(s, SOL_SOCKET, SO_RCVBUF, 
				&engp->socket_rcvbuf, optlen);
		}
		getsockopt(s, SOL_SOCKET, SO_RCVBUF,
			   &engp->cur_socket_rcvbuf, (socklen_t*)&optlen);
	}

	return 0;

}


// 32-bit random number generator.
int Rand32() {
	struct timeval tm;
	gettimeofday(&tm, NULL);
	srand(tm.tv_usec);
	return (rand() * rand());
}

// Sends data directly from a buffer with a specified length to the
// connected remote host, via either the TCP control socket or the
// TCP data socket. Returns the number of bytes sent.
//
int SendTCPData(TPEngine *engp, char * buf, int len, int sockInx) {
	WSInfo * w;
	int res = 0;
	struct timeval tv;
	fd_set fds;

	w = (WSInfo *)(engp->ctrlRefp);
	if (w == 0) return -1;

	tv.tv_sec = 0;
	tv.tv_usec = engp->callMeAgain;

	FD_ZERO(&fds);

	if (sockInx == TP_SOCKINX_CTRL) {
		FD_SET(w->tcpCtrlSock, &fds);
		if (select(FD_SETSIZE, NULL, &fds, NULL, &tv) == 1)
			res = send(w->tcpCtrlSock, buf, len, 0);
	}
	else if (sockInx == TP_SOCKINX_DATA) {
		FD_SET(w->tcpDataSock, &fds);
		if (select(FD_SETSIZE, NULL, &fds, NULL, &tv) == 1)
			res = send(w->tcpDataSock, buf, len, 0);
	}

	return res;
}


// Initiate a TCP control or data connection to a server
int InitConnectTCP(TPEngine *engp, int sockInx)
{
	int				res		= -1;
	WSInfo		*w		= 0;
	SOCKADDR_IN		serverAdr;
	
	w = (WSInfo *) engp->ctrlRefp;
	if (w == 0) return -1;
	
	serverAdr.sin_family = AF_INET;
	serverAdr.sin_addr.s_addr = engp->hostIP.s_addr;

	res = InitTCPSock(engp, sockInx);
	if (res != 0)
		return res;

	switch (sockInx) {
	case TP_SOCKINX_CTRL:	
		serverAdr.sin_port = htons(engp->hostCtrlPort);
		res = connect(w->tcpCtrlSock, (SOCKADDR *)&serverAdr, 
			sizeof(SOCKADDR_IN));
		w->ctrlConnInProg = 1;
		break;
	case TP_SOCKINX_DATA:
		serverAdr.sin_port = htons(engp->hostTCPDataPort);
		res = connect(w->tcpDataSock, (SOCKADDR *)&serverAdr, 
			sizeof(SOCKADDR_IN));
		w->dataConnInProg = 1;
		break;
	default:
		return -1;	// Invalid TCP socket to connect
	}
	
	if (res == SOCKET_ERROR) {
#ifdef WHATOSDOESTHIS
		if (errno != EWOULDBLOCK) {
#else
		if (errno != EINPROGRESS) {
#endif
			switch (sockInx) {
				case TP_SOCKINX_CTRL:
					w->ctrlConnInProg = 0;
					break;
				case TP_SOCKINX_DATA:
					w->dataConnInProg = 0;
					break;
			}
			return -1;
		}
	}

	return 0;
}

int CloseCtrlSock(TPEngine *engp) {
	WSInfo * w;
	int ret;

	w = (WSInfo *)(engp->ctrlRefp);
	if (w == 0) return -1;

	w->ctrlConnInProg = 0;
	w->ctrlConnOK = 0;
	engp->tcpCtrlConnectComplete = 0;

	ret = close(w->tcpCtrlSock);
	w->tcpCtrlSock = INVALID_SOCKET;
	if (ret == SOCKET_ERROR)
		return errno;
	
	return 0;
}


/* ---------------------------------------------------- SendUDPPacket ---- *\

	Send next UDP packet to the remote host's UDP receive port from our
	UDP send socket. Packet content is taken from engp->packetBuf.

\* ----------------------------------------------------------------------- */

int SendUDPPacket(TPEngine *engp, int length)
{
	WSInfo		*w		= 0;
	struct tpPacket	*tPnt;
	SOCKADDR_IN udpPeerRecvAdr;
	
	w = (WSInfo *) engp->ctrlRefp;
	if (w == 0) return 0;	

	tPnt = (struct tpPacket *) engp->packetBuf;
	if (tPnt == 0) return 0;				// No packet buffer set

    udpPeerRecvAdr.sin_family = AF_INET;
	udpPeerRecvAdr.sin_addr.s_addr = engp->hostIP.s_addr;
	udpPeerRecvAdr.sin_port = htons(engp->hostUDPRecvPort);

	// Send the data to the socket
//	printf("Sending %d byte UDP packet from %u to %u\n",
//		length, engp->myUDPSendPort, engp->hostUDPRecvPort);
		
	return sendto(w->udpSendSock, (char *)tPnt, length, 0, (SOCKADDR *)&(udpPeerRecvAdr), sizeof(SOCKADDR_IN));

}

/*
int InitHostNameLookup(TPEngine *engp) {
	WSInfo * w;
	int i;

	w = (WSInfo *)(engp->ctrlRefp);
	if (w == 0) return -1;

	if (strlen(engp->hostName) == 0)
		return TPER_NOHOSTNAME;

	if (w->lookupBuf == 0)
		w->lookupBuf = (char *)calloc(MAXGETHOSTSTRUCT+1, 1);

	for (i = 0; i < MAX_LOOKUP_IP; i++)
		engp->hostIPTab[i].s_addr = (UINT32)0;

	if (inet_addr(engp->hostName) == INADDR_NONE) {
		memset(w->lookupBuf, 0, MAXGETHOSTSTRUCT);
		w->lookupHandle = WSAAsyncGetHostByName(NULL, 0, engp->hostName, w->lookupBuf, MAXGETHOSTSTRUCT);
	}
	else 
		engp->hostIPTab[0].s_addr = inet_addr(engp->hostName);

	return 0;
}
*/
/*
int ContinueHostNameLookup(TPEngine *engp) {
	WSInfo * w;

	w = (WSInfo *)(engp->ctrlRefp);
	if (w == 0) return -1;



}
*/


// Queue data for transmission on the TCP control socket
int QueueCtrlData(TPEngine *engp, char *data)
{
	int				len;
	int				err;
	WSInfo		*w;
	
	w = (WSInfo *) engp->ctrlRefp;
	if (w == 0) return -1;	// Never allocated
	if (!w->ctrlConnOK) return -1;

	len = strlen(data);
	if ((w->sendBufCnt + len) < (REPLYBUF_SIZE * 2))
		strcpy(w->sendBuf + w->sendBufCnt, data);	// Just add o buffer

	w->sendBufCnt += len;
	
	err = SendCtrlData(engp);				// Try to get rid of it
	return err;
}


// Actually try to send the data on the control socket
int SendCtrlData(TPEngine *engp) {
	WSInfo *w;
	char * p;
	int len, bytesSent;

	w = (WSInfo *)(engp->ctrlRefp);
	if (w == 0) return -1;

	if (w->tcpCtrlSock == INVALID_SOCKET) {
		return -1;
	}

	if (w->sendBufCnt <= 0) return 0;

	p = w->sendBuf + w->sendBufPos;
	len = w->sendBufCnt - w->sendBufPos;
	
	bytesSent = send(w->tcpCtrlSock, w->sendBuf, len, 0);

	if (bytesSent > 0) {
		if (bytesSent == len) 
			w->sendBufCnt = w->sendBufPos = 0;
		else
			w->sendBufPos += bytesSent;
	}
	else {
		if (bytesSent == SOCKET_ERROR) {
			w->lastErr = errno;
		}
	}
	return 0;
}


int CheckCtrlOpen(TPEngine *engp) 
{
	WSInfo *w;

	w = (WSInfo *)(engp->ctrlRefp);
	if (w == 0)
		return 0;			// Never allocated

	if (w->ctrlConnOK) return 1;
	return 0;
}


int ConsumeCtrlData(TPEngine *engp)
{
	WSInfo		*w;
	int c, ret, cnt;
	fd_set fds;
	struct timeval tv;

	w = (WSInfo *) engp->ctrlRefp;
	if (w == 0) return 0;				// Never allocated
	
	if (engp->ctrlMessageComplete != 0) goto done;		// Old data still unprocessed

	while (w->xferCnt > 0) {					// We have some data in xfer buffer
		c = w->xferBuf[w->xferPos] & 255;
		w->xferPos += 1;
		w->xferCnt -= 1;
		
		if (c == CR) {
			w->crSeen = 1;
			continue;
		}
		if (c == LF && w->crSeen) {
			w->replyBuf[w->replyCnt] = 0;
			strncpy(engp->ctrlMessage, w->replyBuf, REPLYBUFSIZE-1);
			engp->ctrlMessageComplete = 1;
			w->crSeen = 0;
//			if (1) dprintf("Ctrl data: <%s>\n", top->replyBuf);
//			printf("Ctrl data: %s\n", w->replyBuf);
			break;
			
		} else {
			// Do not overwrite our line buffer. If the line is too long,
			// do not copy any characters beyond our limit (REPLYBUF_SIZE-1)
			if (w->replyCnt < (REPLYBUF_SIZE - 1))
				w->replyBuf[w->replyCnt] = c;
			w->replyCnt++;
		}
	}
	
	if (engp->ctrlMessageComplete != 0) goto done;		// Old data still unprocessed

	// Read new data from socket
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(w->tcpCtrlSock, &fds);
	ret = select(FD_SETSIZE, &fds, NULL, NULL, &tv);	
        if (ret == SOCKET_ERROR) {
		return errno;
	}
	if (FD_ISSET(w->tcpCtrlSock, &fds)) {
		cnt = recv(w->tcpCtrlSock, w->xferBuf, XFERBUF_SIZE - 1, 0);
		if (cnt == SOCKET_ERROR) {
			w->ctrlConnOK = 0;
			w->lastErr = errno;
		}
		else {
			w->xferCnt = cnt;
			w->xferPos = 0;
		}
	}

	
done:
	return 0;							// Dummmy
}


#ifdef NO_GETTIMEOFDAY
/*
 * Init gettimeofday() variables
 * Returns 1 if system supports high-resolution timer
 */
int Init_gettimeofday()
{
	if (!QueryPerformanceFrequency((LARGE_INTEGER*)&performance_frequency))
		return 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&counter_start_value);
	start_sec = time(NULL);
	return 1;
}

// Faked gettimeofday() function for Windows, using the 
// Win32 performance counter
void gettimeofday(struct timeval *tv, void *tz)
{
	_int64 time_now, dtime;
	int dsecs, dnsecs;
	QueryPerformanceCounter((LARGE_INTEGER *)&time_now);

	dtime = time_now - counter_start_value;
	dsecs = (int)(dtime / performance_frequency);
	dnsecs = (int)( ((double)(dtime % performance_frequency)) / ((double)performance_frequency) * 1000000.0);

	tv->tv_sec = start_sec + dsecs;
	tv->tv_usec = dnsecs;
}
#endif


// Create UDP send and receive sockets. Bind them to allocate local port numbers
// and find out what local port numbers were allocated so we can tell this to 
// our peers when setting up new test sessions. Store the local port numbers in
// engp->myUDPSendPort and engp->myUDPRecvPort.
//
// Suggested is also to check the value of engp->myUDPSendPort and myUDPRecvPort
// before doing the bind. If they have a value other than zero it means you should
// try to bind to that port instead of binding to any available port like is done
// below.
//

int InitUDP(TPEngine *engp) {
	int saLen, noBlock, optlen;
	SOCKADDR_IN rcvSa;

	WSInfo * w;

	w = (WSInfo *)(engp->ctrlRefp);
	if (w == 0) return -1;

	w->udpRecvSock = socket(AF_INET, SOCK_DGRAM, 0);
	if( w->udpRecvSock == INVALID_SOCKET ) {
//		MessageBox(CurDlg, "Can't create rcv data socket", TPERROR, MB_OK);
		goto fail;
	}

	w->udpSendSock = socket(AF_INET, SOCK_DGRAM, 0);
	if( w->udpSendSock == INVALID_SOCKET ) {
//		MessageBox(CurDlg, "Can't create send data socket", TPERROR, MB_OK);
		goto fail;
	}

	noBlock = fcntl(w->udpSendSock, F_GETFL);
	noBlock |= O_NONBLOCK;
	fcntl(w->udpSendSock, F_SETFL, noBlock);
	if (!(fcntl(w->udpSendSock, F_GETFL) & O_NONBLOCK)) {
//		MessageBox(CurDlg, "Can't set non-blocking mode for send socket", TPERROR, MB_OK);
		goto fail;
	}

	noBlock = fcntl(w->udpRecvSock, F_GETFL);
	noBlock |= O_NONBLOCK;
	fcntl(w->udpRecvSock, F_SETFL, noBlock);
	if (!(fcntl(w->udpRecvSock, F_GETFL) & O_NONBLOCK)) {
//		MessageBox(CurDlg, "Can't set non-blocking mode for rcv socket", TPERROR, MB_OK);
		goto fail;
	}

	/*
	 * Bind the data sockets to allocate port numbers.
	 */
	memset( &rcvSa, 0, sizeof rcvSa );
#ifdef HAS_SINLEN
		rcvSa.sin_len = sizeof( struct sockaddr_in );
#endif
	rcvSa.sin_family = AF_INET;
	rcvSa.sin_addr.s_addr = INADDR_ANY;
	rcvSa.sin_port =  0;
	if( bind( w->udpRecvSock, (struct sockaddr *)(&rcvSa), sizeof rcvSa ) < 0 ) {
 //   	MessageBox(CurDlg, "Can't bind rcv data socket", TPERROR, MB_OK);
		goto fail;
	}
	/*
	 * Get the local port number assigned to the data socket.
	 */
	saLen = sizeof rcvSa;
	if( getsockname( w->udpRecvSock, (struct sockaddr *)(&rcvSa), (socklen_t*)&saLen ) != 0 ) {
// 		MessageBox(CurDlg, "Can't get local port number on rcv data socket", TPERROR, MB_OK);
		goto fail;
	}
	engp->myUDPRecvPort = ntohs(((SOCKADDR_IN *)&rcvSa)->sin_port);


	memset( &rcvSa, 0, sizeof rcvSa );
#ifdef HAS_SINLEN
		rcvSa.sin_len = sizeof( struct sockaddr_in );
#endif
	rcvSa.sin_family = AF_INET;
	rcvSa.sin_addr.s_addr = INADDR_ANY;
	rcvSa.sin_port =  0;
	if( bind( w->udpSendSock, (struct sockaddr *)(&rcvSa), sizeof rcvSa ) < 0 ) {
 //   	MessageBox(CurDlg, "Can't bind rcv data socket", TPERROR, MB_OK);
		goto fail;
	}
	/*
	 * Get the local port number assigned to the data socket.
	 */
	saLen = sizeof rcvSa;
	if( getsockname( w->udpSendSock, (struct sockaddr *)(&rcvSa), (socklen_t*)&saLen ) != 0 ) {
// 		MessageBox(CurDlg, "Can't get local port number on rcv data socket", TPERROR, MB_OK);
		goto fail;
	}
	engp->myUDPSendPort = ntohs(((SOCKADDR_IN *)&rcvSa)->sin_port);

//	printf("myUDPSendPort = %d      myUDPRecvPort=%d\n", 
//		engp->myUDPSendPort, engp->myUDPRecvPort);


	optlen = sizeof(int);
	if (engp->socket_sndbuf != 0) {
		setsockopt(w->udpSendSock, SOL_SOCKET, SO_SNDBUF, 
			&engp->socket_sndbuf, optlen);
		setsockopt(w->udpRecvSock, SOL_SOCKET, SO_SNDBUF, 
			&engp->socket_sndbuf, optlen);
	}
	getsockopt(w->udpSendSock, SOL_SOCKET, SO_SNDBUF,
		   &engp->cur_socket_sndbuf, (socklen_t*)&optlen);
	if (engp->socket_rcvbuf != 0) {
		setsockopt(w->udpSendSock, SOL_SOCKET, SO_RCVBUF, 
			&engp->socket_rcvbuf, optlen);
		setsockopt(w->udpRecvSock, SOL_SOCKET, SO_RCVBUF, 
			&engp->socket_rcvbuf, optlen);
	}
	getsockopt(w->udpSendSock, SOL_SOCKET, SO_RCVBUF,
		   &engp->cur_socket_rcvbuf, (socklen_t*)&optlen);


	return 0;

fail:
	CloseAllSockets(engp);
	if (errno != 0)
		return errno;
	return -1;
}



// InitTCPSock() - Create TCP sockets.
//
// TP_SOCKINX_CTRL = a client control socket
// TP_SOCKINX_SCTRL = a server control socket that is set to listen for
// incoming connections. 
//
// Try to bind to the IP address stored in engp->myLocalAddress or
// any address if the value is 0 (zero)
//
// TP_SOCKINX_CTRL: try to bind to the port number stored in engp->myTCPControlPort
//					or any port if value is zero
// TP_SOCKINX_DATA: try to bind to the port number stored in engp->myTCPDataPort
//					or any port if value is zero
// TP_SOCKINX_SCTRL: Bind to myTCPControlPort if non-zero or DEFAULT_CONTROL_PORT
//					otherwise. Set the socket to listen for incoming connections
// TP_SOCKINX_SDATA: Bind to myTCPDataPort if non-zero or any port otherwise.
//					 Set the socket to listen for incoming connections and store
//					 the local port number in engp->myTCPDataPort.
//

int InitTCPSock(TPEngine *engp, int sockInx) {
	WSInfo * w;
	SOCKADDR_IN locSa;
	int saLen, optlen;
	int ret, noBlock;
	struct linger ling;
	int one = 1;

	ling.l_onoff = 0;

	w = (WSInfo *)(engp->ctrlRefp);
	if (w == 0)
		return -1;

	saLen = sizeof(locSa);
	memset(&locSa, 0, sizeof(locSa));
	locSa.sin_family = AF_INET;
	if (engp->myLocalAddress.s_addr != (UINT32)0)
		locSa.sin_addr.s_addr = engp->myLocalAddress.s_addr;
	else
		locSa.sin_addr.s_addr = INADDR_ANY;
	
	switch (sockInx) {

	case TP_SOCKINX_CTRL:		// Client opening a control socket

		// Create socket
		w->tcpCtrlSock = socket(AF_INET, SOCK_STREAM, 0);
		if ( w->tcpCtrlSock == INVALID_SOCKET ) 
			goto fail;
		// Set non-blocking mode

		noBlock = fcntl(w->tcpCtrlSock, F_GETFL);
		noBlock |= O_NONBLOCK;
		fcntl(w->tcpCtrlSock, F_SETFL, noBlock);
		if (!(fcntl(w->tcpCtrlSock, F_GETFL) & O_NONBLOCK))
			goto fail;

		// Check if we want to bind to a specific local port
		if ( engp->myTCPControlPort != 0 ) 
			locSa.sin_port = htons(engp->myTCPControlPort);
		else
			locSa.sin_port = 0;
		// If we want a specific port or address, bind to them
		if (engp->myTCPControlPort != 0 || engp->myLocalAddress.s_addr != 0) {
			ret = bind(w->tcpCtrlSock, (SOCKADDR *)&locSa, saLen);
			if (ret != 0)
				goto fail;
		}
		
		// Get bound address/port combination
		if (getsockname(w->tcpCtrlSock, (SOCKADDR *)(&locSa), (socklen_t*)&saLen) == SOCKET_ERROR)
			goto fail;
		engp->myLocalAddress.s_addr = locSa.sin_addr.s_addr;
		engp->myTCPControlPort = ntohs(locSa.sin_port);

		break;

	case TP_SOCKINX_DATA:		// Client opening a data socket
		w->tcpDataSock = socket(AF_INET, SOCK_STREAM, 0);
		if ( w->tcpDataSock == INVALID_SOCKET ) 
			goto fail;

		noBlock = fcntl(w->tcpDataSock, F_GETFL);
		noBlock |= O_NONBLOCK;
		fcntl(w->tcpDataSock, F_SETFL, noBlock);
		if (!(fcntl(w->tcpDataSock, F_GETFL) & O_NONBLOCK))
			goto fail;

		optlen = sizeof(int);
		if (engp->socket_sndbuf != 0) {
			setsockopt(w->tcpDataSock, SOL_SOCKET, SO_SNDBUF, 
				&engp->socket_sndbuf, optlen);
		}
		getsockopt(w->tcpDataSock, SOL_SOCKET, SO_SNDBUF,
			   &engp->cur_socket_sndbuf, (socklen_t*)&optlen);
		if (engp->socket_rcvbuf != 0) {
			setsockopt(w->tcpDataSock, SOL_SOCKET, SO_RCVBUF, 
				&engp->socket_rcvbuf, optlen);
		}
		getsockopt(w->tcpDataSock, SOL_SOCKET, SO_RCVBUF, 
			   &engp->cur_socket_rcvbuf, (socklen_t*)&optlen);


		if (engp->myTCPDataPort != 0)
			locSa.sin_port = htons(engp->myTCPDataPort);
		if (engp->myLocalAddress.s_addr != 0 || engp->myTCPDataPort != 0) {
			ret = bind(w->tcpDataSock, (SOCKADDR *)&locSa, saLen);
			if (ret != 0)
				goto fail;
		}
		break;

	case TP_SOCKINX_SCTRL:		// Server opening a listen control socket

		w->tcpServerCtrlSock = socket(AF_INET, SOCK_STREAM, 0);
		if ( w->tcpServerCtrlSock == INVALID_SOCKET ) 
			goto fail;

		noBlock = fcntl(w->tcpServerCtrlSock, F_GETFL);
		noBlock |= O_NONBLOCK;
		fcntl(w->tcpServerCtrlSock, F_SETFL, noBlock);
		if (!(fcntl(w->tcpServerCtrlSock, F_GETFL) & O_NONBLOCK))
			goto fail;

		// REUSEADDR so the OS will let use reuse that particular port next time
		if ( setsockopt( w->tcpServerCtrlSock, SOL_SOCKET, SO_REUSEADDR, (char *)(&one),
			sizeof( one ) ) != 0 )
			goto fail;
		// DONTLINGER to avoid keeping sockets with unsent data alive
/*
		if ( setsockopt( w->tcpServerCtrlSock, SOL_SOCKET, SO_LINGER, (char *)(&ling),
			sizeof( struct linger ) ) != 0 )
			goto fail;
*/
		locSa.sin_family = AF_INET;
		if (engp->myLocalAddress.s_addr != 0)
			locSa.sin_addr.s_addr = engp->myLocalAddress.s_addr;
		else
			locSa.sin_addr.s_addr = INADDR_ANY;
		if (engp->myTCPControlPort != 0)
			locSa.sin_port = htons(engp->myTCPControlPort);
		else
			locSa.sin_port = htons(DEFAULT_CONTROL_PORT);

		ret = bind( w->tcpServerCtrlSock, (SOCKADDR *)&locSa, saLen );
		if (ret != 0)
			goto fail;

		// Get bound address/port combination
		if (getsockname(w->tcpServerCtrlSock, (SOCKADDR *)(&locSa), (socklen_t*)&saLen) == SOCKET_ERROR)
			goto fail;
		engp->myLocalAddress.s_addr = locSa.sin_addr.s_addr;
		engp->myTCPControlPort = ntohs(locSa.sin_port);

		ret = listen( w->tcpServerCtrlSock, LISTEN_BACKLOG );
		if (ret != 0) 
			goto fail;

		break;

	case TP_SOCKINX_SDATA:		// Server opening a listen data socket

		w->tcpServerDataSock = socket(AF_INET, SOCK_STREAM, 0);
		if ( w->tcpServerDataSock == INVALID_SOCKET ) 
			goto fail;

		noBlock = fcntl(w->tcpServerDataSock, F_GETFL);
		noBlock |= O_NONBLOCK;
		fcntl(w->tcpServerDataSock, F_SETFL, noBlock);
		if (!(fcntl(w->tcpServerDataSock, F_GETFL) & O_NONBLOCK))
			goto fail;

		if ( setsockopt( w->tcpServerDataSock, SOL_SOCKET, SO_REUSEADDR, (char *)(&one),
			sizeof( one ) ) != 0 )
			goto fail;

/*
		if ( setsockopt( w->tcpServerDataSock, SOL_SOCKET, SO_LINGER, (char *)(&ling),
			sizeof( struct linger ) ) != 0 ) 
			goto fail;
*/		

		optlen = sizeof(int);
		if (engp->socket_sndbuf != 0) {
			setsockopt(w->tcpServerDataSock, SOL_SOCKET, SO_SNDBUF, 
				&engp->socket_sndbuf, optlen);
		}
		getsockopt(w->tcpServerDataSock, SOL_SOCKET, SO_SNDBUF,
			   &engp->cur_socket_sndbuf, (socklen_t*)&optlen);
		if (engp->socket_rcvbuf != 0) {
			setsockopt(w->tcpServerDataSock, SOL_SOCKET, SO_RCVBUF, 
				&engp->socket_rcvbuf, optlen);
		}
		getsockopt(w->tcpServerDataSock, SOL_SOCKET, SO_RCVBUF, 
			   &engp->cur_socket_rcvbuf, (socklen_t*)&optlen);

		locSa.sin_family = AF_INET;
		if (engp->myLocalAddress.s_addr != 0)
			locSa.sin_addr.s_addr = engp->myLocalAddress.s_addr;
		else
			locSa.sin_addr.s_addr = INADDR_ANY;
		if (engp->myTCPDataPort != 0)
			locSa.sin_port = htons(engp->myTCPDataPort);
		else 
			locSa.sin_port = 0;
		
		ret = bind( w->tcpServerDataSock, (SOCKADDR *)&locSa, saLen );
		if (ret != 0)
			goto fail;

		ret = getsockname(w->tcpServerDataSock, (SOCKADDR *)&locSa, (socklen_t*)&saLen);
		if (ret != 0) 
			goto fail;
		engp->myTCPDataPort = ntohs(locSa.sin_port);

		ret = listen( w->tcpServerDataSock, LISTEN_BACKLOG );
		if (ret != 0) 
			goto fail;

		break;

	default:
		return -1;
	}

	return 0;


fail:

	switch (sockInx) {

	case TP_SOCKINX_CTRL:
		close(w->tcpCtrlSock);
		break;
	case TP_SOCKINX_SCTRL:
		close(w->tcpServerCtrlSock);
		break;
	case TP_SOCKINX_DATA:
		close(w->tcpDataSock);
		break;
	case TP_SOCKINX_SDATA:
		close(w->tcpServerDataSock);
	}
	if (ret != 0)
		return errno;
	else
		return -1;
}


int InitSessComm(TPEngine *engp)
{	
	WSInfo *w;
	// Watch out
	engp->ctrlRefp = (void *)calloc(1, sizeof(WSInfo));
	w = (WSInfo *)(engp->ctrlRefp);
	if (w == NULL)
		return -1;

	w->tcpCtrlSock = INVALID_SOCKET;
	w->tcpDataSock = INVALID_SOCKET;
	w->tcpServerCtrlSock = INVALID_SOCKET;
	w->tcpServerDataSock = INVALID_SOCKET;
	w->udpSendSock = INVALID_SOCKET;
	w->udpRecvSock = INVALID_SOCKET;

	// success
	return 0;

}
