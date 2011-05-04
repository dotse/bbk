/*
 * $Id: tpengine.cpp,v 1.1 2007/01/31 07:45:40 danron Exp $
 * $Source: /cvsroot-fuse/tptest/tptest5/src/net/tpengine.cpp,v $
 *	
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tpengine.c - main test engine code
 *
 * Written by
 *  Ragnar Lönn <prl@gatorhole.com>
 *  Hans Green <hg@3tag.com>
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

#include <stdio.h>
#include <time.h>
#include "tpengine.h"
#include "tpcommon.h"
#include "tpio.h"

#ifndef MAXINT
#define MAXINT INT_MAX
#endif

// *** init tcpDataSock in DoTCPSend/DoTCPRecv?


// Implemented in this file (tpengine.c)
int 	AddServerToList(TPEngine *engp, char * str);
int	CheckHelloReply(TPEngine *);
int	CheckServerBanner(TPEngine *);
int	CheckStatsMsg(TPEngine *);
int	CheckTestReply(TPEngine *);
int 	CheckTestMessage(TPEngine *);
void 	ClearBestRates(TPEngine *);
void 	ClearServerList(TPEngine *engp);
TPEngine *CreateContext(void);
void 	DeleteContext(TPEngine *);
int 	DoClientTCPRecv(TPEngine *);
int 	DoClientTCPSend(TPEngine *);
int 	DoClientUDPDuplex(TPEngine *);
int 	DoClientUDPRecv(TPEngine *);
int 	DoClientUDPSend(TPEngine *);
int 	DoNameLookup(TPEngine *engp);
int 	DoQueryMaster(TPEngine	*);
int 	DoServerTCPSend(TPEngine *);
int 	DoServerTCPRecv(TPEngine *);
int 	DoServerUDPDuplex(TPEngine *);
int 	DoServerUDPRecv(TPEngine *);
int 	DoServerUDPSend(TPEngine *);
void 	FailSession(TPEngine *, int);
char 	*GetSMStateName(TPEngine *, int);
void 	GenerateUDPDataPacket(TPEngine *);
int 	RunClientContext(TPEngine *);
int 	RunServerContext(TPEngine *);
int 	SendHeloLine(TPEngine *);
int 	SendStatLine(TPEngine *);
int 	SendTCPTestData(TPEngine *);
int 	SendTestLine(TPEngine *);
int 	SendTestOKMessage(TPEngine *);
int 	SendUDPDataPacket(TPEngine *);
void	SetSMState(TPEngine *, int);
void	SetTimeLimit(TPEngine *, int);
void	SetTimeLimit2(TPEngine *, int);
int 	StartClientContext(TPEngine *);
int 	StartServerContext(TPEngine *);
void 	StopContext(TPEngine *);
int 	TimeLimitExceeded(TPEngine *);
void 	InitRandBuf(void *, int);


char *clientModes[] = {
	"NONE",
	"IDLE", "CONNECTING", "CONNECTED", "WAITPORTS", "NATOPEN",
	"TESTLOOP", "SENDSTAT", "WAITSTAT", "DELAYQUIT", "TERM_WAIT", "FAILED",
	"SENDMHELO", "SERVERLIST", "COMPLETE", "DATACONNECTING"
};
#define MAX_CLIENT_STRING	(sizeof(clientModes) / sizeof(clientModes[0]))


char *GetSMStateName(TPEngine *engp, int state)
{
	char	*cp		= "??";
	
	if (state < 0) return cp;
	
	switch (engp->tpMode) {
	
	case M_TCP_SEND:
	case M_TCP_RECV:
	case M_UDP_SEND:
	case M_UDP_RECV:
	case M_UDP_FDX:
		if (state < MAX_CLIENT_STRING) cp = clientModes[state];
		break;
	}
	return cp;
}



int StartClientContext(
	TPEngine	*engp)
{

	int res;

	// Check supplied values, init packetInterval, set executor function
	switch (engp->tpMode) {

	case M_UDP_SEND:
	case M_UDP_RECV:
	case M_UDP_FDX:

		if (engp->sessionTime <= 0) goto failexit;
		if (engp->sessionMaxTime <= 0) {
			if ((engp->sessionTime + 10) > (engp->sessionTime * 2))
				engp->sessionMaxTime = engp->sessionTime + 10;
			else				
				engp->sessionMaxTime = engp->sessionTime * 2;
		}
		if (engp->nPackets <= 0) {
			if (engp->packetsPerSecond <= 0) goto failexit;
			engp->nPackets = engp->packetsPerSecond * engp->sessionTime;
		}
		if (engp->packetSize <= 0) goto failexit;

		engp->packetInterval = (engp->sessionTime * 1000000) / engp->nPackets;
		engp->packetsPerSecond = 1000000 / engp->packetInterval;
		engp->bitsPerSecond = engp->packetsPerSecond * engp->packetSize * 8;

		switch (engp->tpMode) {

		case M_UDP_FDX:
			engp->executor = DoClientUDPDuplex;
			break;

		case M_UDP_RECV:
			engp->executor = DoClientUDPRecv;
			break;

		case M_UDP_SEND:
			engp->executor = DoClientUDPSend;
			break;
		}
			
		break;

	case M_TCP_SEND:
	case M_TCP_RECV:

		if (engp->sessionMaxTime <= 0) 
			goto failexit;
		if (engp->tcpBytes <= 0) 
			goto failexit;
		if (engp->tpMode == M_TCP_SEND) 
			engp->executor = DoClientTCPSend;
		else
			engp->executor = DoClientTCPRecv;
		break;

	case M_QUERY_MASTER:
		engp->executor = DoQueryMaster;
		break;

	case M_NAME_LOOKUP:
		engp->executor = DoNameLookup;
		break;

	default:
		// Illegal mode
		goto failexit;
	}

    SetSMState(engp, CLSM_IDLE);
	engp->failCode = 0;
	engp->timeLimit = 0;
	engp->curSend = 0;
	engp->iAmServer = 0;
	engp->callMeAgain = 50000;
	engp->ioError = 0;
	engp->active = 1;
	time(&engp->startTime);

	if (engp->tpMode == M_NAME_LOOKUP) {
		int timeLimit = 
			engp->sessionMaxTime < 30 ? 30 : engp->sessionMaxTime;
		SetTimeLimit(engp, timeLimit);
		if (engp->wipeFlag) {
			int i;
			for (i = 0; i < engp->numHostIP; i++) {
				engp->hostIPTab[i].s_addr = 0;
			}
			engp->numHostIP = 0;
			engp->wipeFlag = 0;
		}
		res = InitNameLookup(engp);
		if (res == 0) {
			SetSMState(engp, CLSM_NAMELOOKUP);
		}
		else {
			FailSession(engp, TPER_NLINITFAIL);
		}
	} 

	engp->stats.PktsSent = engp->stats.PktsUnSent = engp->stats.PktsRecvd = 0;
	engp->stats.BytesSent = engp->stats.BytesRecvd = 0;
	engp->stats.MaxRoundtrip = 0;
	engp->stats.MinRoundtrip = 999999999;
	engp->stats.ooCount = 0;
	engp->prevPacket = 0;
	return 0;

	failexit:
		SetSMState(engp, CLSM_FAILED);
		engp->executor = 0;
		return -1;
}

int DoNameLookup(
	TPEngine	*engp)
{
	int				res;
	res = ContinueNameLookup(engp);
	if (res != 0) {
		FailSession(engp, TPER_NLFAIL);
	}
	return engp->state;
}


int RunClientContext(
	TPEngine	*engp)
{
	int				res;
	
	// Check if control socket has data or has gone
	
	if (CheckCtrlOpen(engp)) {
		res = SendCtrlData(engp);
		if (res != 0) {
			FailSession(engp, TPER_CTRLCLOSED);
			return 0;
		}
		res = ConsumeCtrlData(engp);
		if (res != 0) {
			FailSession(engp, TPER_CTRLCLOSED);
			return 0;
		}
	}
	
	if (engp->active == 0) return 0;
	if (engp->executor == 0) return 0;

	// Check if long timer has run out
	
	if (TimeLimitExceeded(engp)) {
		FailSession(engp, TPER_TIMEOUT);
		return 0;
	}

	switch (engp->state) {
			
	case CLSM_FAILED:		// Already failed
	case CLSM_COMPLETE:		// Already done
		break;
				
	case CLSM_IDLE:			// Starting a new test session
		
		ClearCtrlReply(engp);
		engp->tcpCtrlConnectComplete = 0;
		res = InitConnectTCP(engp, TP_SOCKINX_CTRL);		
		if (res == 0) {
			SetSMState(engp, CLSM_CONNECTING);
		} else {
			FailSession(engp, TPER_NOCTRL);
			return 0;
		}
		// Set time limit to either what's left of our session time or
		// 20 seconds, if remaining session time is larger
		SetTimeLimit2(engp, 20);
		break;

	case CLSM_CONNECTING:
		if (engp->tcpCtrlConnectComplete) {
			if (engp->tpMode == M_QUERY_MASTER) {
				SetSMState(engp, CLSM_SENDMHELO);
				break;
			}
			SetSMState(engp, CLSM_CONNECTED);
			// Set time limit to either what's left of our session time
			// or 10 seconds, if remaining session time is larger
			SetTimeLimit2(engp, 10);
		}
		else {
			res = ContinueConnectTCP(engp);
			if (res == 0) {
				// success
			} else {
				FailSession(engp, TPER_CONNECTFAIL);
			}
		}
		break;
		
	case CLSM_CONNECTED:		// Connected - start test phase
	
		if (engp->ctrlMessageComplete == 0) break;	// No reply yet
		
		res = CheckServerBanner(engp);				// Handle server welcome
		if (res != 200) {						// Not the 200 we hoped
			if (res >= 400 && res <= 499) {
				FailSession(engp, TPER_SERVERBUSY);
			}
			else if (res >= 500 && res <= 599) {
				FailSession(engp, TPER_SERVERDENY);
			}
			else {
				FailSession(engp, TPER_BADWELCOME);
			}
			break;
		}
		if (engp->tpMode == M_UDP_SEND
			|| engp->tpMode == M_UDP_RECV
			|| engp->tpMode == M_UDP_FDX) {
			res = InitUDP(engp);                            // Open our UDP sockets
			if (res != 0) {
				FailSession(engp, TPER_UDPOPENFAIL);
				break;
			}
		}
		ClearCtrlReply(engp);
		SendTestLine(engp);						// Send our TEST message
		SetSMState(engp, CLSM_WAITPORTS);

		// Set time limit to either what's left of our session time
		// or 10 seconds, if remaining session time is larger
		SetTimeLimit2(engp, 10);

		break;
		
	case CLSM_DELAYQUIT:
		{
			struct timeval nowTV;
			gettimeofday(&nowTV, NULL);
			if (TVCompare(&nowTV, &(engp->nextSendTV)) >= 0) {
				CloseAllSockets(engp);
				SetSMState(engp, CLSM_COMPLETE);
			}
		}
		break;
		
	default:
	
		engp->executor(engp);
	}
	return 0;
}



int StartServerContext(
	TPEngine	*engp)
{
	
	int res;

	engp->failCode = 0;
	engp->timeLimit = 0;
	engp->active = 1;
	engp->curSend = 0;
	engp->callMeAgain = 50000;
	engp->stats.ooCount = 0;
	engp->prevPacket = 0;
		
	time(&engp->startTime);
	engp->stats.PktsSent = engp->stats.PktsUnSent = engp->stats.PktsRecvd = 0;
	engp->stats.BytesSent = engp->stats.BytesRecvd = 0;
	engp->stats.MaxRoundtrip = 0;
	engp->stats.MinRoundtrip = 999999999;

	res = InitTCPSock(engp, TP_SOCKINX_SCTRL);
	if (res != 0) {
		SetSMState(engp, SSM_FAILED);
		return -1;
	}

	SetSMState(engp, SSM_LISTEN);
	engp->iAmServer = 1;

	return 0;
}


int RunServerContext(
	TPEngine	*engp)
{
	int				res;
	static char tBuf[200];
	
	// Check if control socket has data or has gone
	
	if (CheckCtrlOpen(engp)) {
		res = SendCtrlData(engp);
		if (res < 0) {
			FailSession(engp, TPER_CTRLCLOSED);
			return res;
		}
		res = ConsumeCtrlData(engp);
		if (res < 0) {
			FailSession(engp, TPER_CTRLCLOSED);
			return res;
		}
	}
	
	if (engp->active == 0) return 0;

	// Check if long timer has run out
	
	if (TimeLimitExceeded(engp)) {
		FailSession(engp, TPER_TIMEOUT);
		return 0;
	}

	switch (engp->state) {
			
	case SSM_FAILED:		// Already failed
	case SSM_COMPLETE:		// Already done
		break;
	case SSM_LISTEN:
		res = AcceptClient(engp, TP_SOCKINX_CTRL);
		if (res != 0) {
			FailSession(engp, TPER_ACCEPTFAIL);
			return res;
		}
		if (engp->tcpCtrlConnectComplete) {
			SetSMState(engp, SSM_SENDWELCOME);
		}
		break;
	case SSM_SENDWELCOME:
		engp->sessCookie = Rand32();
		ClearCtrlReply(engp);
		sprintf(tBuf, "200 vmajor=%d;vminor=%d;cookie=%ld\r\n",
			MAJORVERSION, MINORVERSION, engp->sessCookie);
		QueueCtrlData(engp, tBuf);
		SetSMState(engp, SSM_WAITTEST);
		SetTimeLimit(engp, 10);
		break;
	case SSM_WAITTEST:
		if (engp->ctrlMessageComplete == 0) break;	// No TEST line yet

		res = CheckTestMessage(engp);				// Handle TEST line. Set executor
		if (res != 0) {						// Not a valid or accepted TEST line
			FailSession(engp, TPER_BADTEST);
			break;
		}
		SetSMState(engp, SSM_POSTTEST);
		break;
	case SSM_POSTTEST:
		switch (engp->tpMode) {
		case M_UDP_RECV:
		case M_UDP_FDX:
			SetSMState(engp, SSM_WAITNAT);
			res = InitUDP(engp);
			if (res != 0)
				FailSession(engp, TPER_UDPOPENFAIL);
			break;
		case M_UDP_SEND:
			SetSMState(engp, SSM_TESTLOOP);
			res = InitUDP(engp);
			if (res != 0)
				FailSession(engp, TPER_UDPOPENFAIL);
			break;
		case M_TCP_SEND:
		case M_TCP_RECV:
			res = InitTCPSock(engp, TP_SOCKINX_SDATA);
			if (res != 0) {
				FailSession(engp, TPER_NOTCPDATASOCK);
				break;
			}
			SetSMState(engp, SSM_DATALISTEN);
			break;
		}
		ClearCtrlReply(engp);
		SendTestOKMessage(engp);
		SetTimeLimit(engp, engp->sessionMaxTime);
		break;

	case SSM_DELAYQUIT:
		{
			struct timeval nowTV;
			gettimeofday(&nowTV, NULL);
			if (TVCompare(&nowTV, &(engp->nextSendTV)) >= 0) {
				CloseAllSockets(engp);
				SetSMState(engp, SSM_COMPLETE);
			}
		}
		break;

	default:
		engp->executor(engp);
	}

	return 1;
}


int SendTestOKMessage(TPEngine *engp) {
	char sBuf[200];

	switch (engp->tpMode) {
	case M_TCP_SEND:
	case M_TCP_RECV:
		sprintf(sBuf, "210 tcpdataport=%u\r\n", (unsigned int)engp->boundTCPDataPort);
		break;
	case M_UDP_SEND:
	case M_UDP_RECV:
	case M_UDP_FDX:
		sprintf(sBuf, "210 udpsendport=%d;udprcvport=%d\r\n",
			(int)engp->boundUDPSendPort, (int)engp->boundUDPRecvPort);
		break;
	}

	QueueCtrlData(engp, sBuf);

	return 0;
}



int CheckTestMessage(TPEngine *engp) {
	
	char *p;
	char tBuf[30];

	if (strncmp(engp->ctrlMessage, "TEST ", 5) != 0)
		return -1;

	p = engp->ctrlMessage + 5;
	memset(tBuf, 0, 30);

	// Version numbers are optional
	if (CopyTagField(tBuf, 29, p, "vmajor"))
		engp->peerMajorVersion = atoi(tBuf);
	if (CopyTagField(tBuf, 29, p, "vminor"))
		engp->peerMinorVersion = atoi(tBuf);

	// Possibly mandatory cookie
	if (engp->sessCookie != 0) {	// Do we want a cookie?
		if (CopyTagField(tBuf, 29, p, "cookie")) {
			if (strtoul(tBuf, NULL, 0) != engp->sessCookie) 
				return TPER_WRONGCOOKIE;  // Cookie mismatch. Error code? ***
		}
		else // No cookie present
			return TPER_NOCOOKIE;
	}

	// Mandatory test mode
	if (CopyTagField(tBuf, 29, p, "mode")) {
		engp->tpMode = atoi(tBuf);
		switch (engp->tpMode) {
		case M_UDP_RECV:
			engp->executor = DoServerUDPSend;
			break;
		case M_UDP_SEND:
			engp->executor = DoServerUDPRecv;
			break;
		case M_UDP_FDX:
			engp->executor = DoServerUDPDuplex;
			break;
		case M_TCP_RECV:
			engp->executor = DoServerTCPSend;
			break;
		case M_TCP_SEND:
			engp->executor = DoServerTCPRecv;
			break;
		default:
			return TPER_WRONGMODE;
		}
	}
	else
		return TPER_NOMODE;

	// Mode-dependent stuff
	switch (engp->tpMode) {
	case M_UDP_SEND:
	case M_UDP_RECV:
	case M_UDP_FDX:
		if (CopyTagField(tBuf, 29, p, "time"))
			engp->sessionTime = atoi(tBuf);
		else
			return TPER_NOTIME;
		if (CopyTagField(tBuf, 29, p, "npackets"))
			engp->nPackets = atoi(tBuf);
		else
			return TPER_NONPACKETS;
		if (CopyTagField(tBuf, 29, p, "psize"))
			engp->packetSize = atoi(tBuf);
		else
			return TPER_NOPSIZE;
		if (CopyTagField(tBuf, 29, p, "udpsendport"))
			engp->hostUDPSendPort = (unsigned short)atoi(tBuf);
		else
			return TPER_NOUDPSENDPORT;
		if (CopyTagField(tBuf, 29, p, "udprcvport"))
			engp->hostUDPRecvPort = (unsigned short)atoi(tBuf);
		else
			return TPER_NOUDPRECVPORT;
		// timeout not mandatory
		if (CopyTagField(tBuf, 29, p, "timeout"))
			engp->sessionMaxTime = atoi(tBuf);
		else
			engp->sessionMaxTime = engp->sessionTime * 2;

		engp->packetInterval = (engp->sessionTime * 1000000) / engp->nPackets;
		engp->packetsPerSecond = 1000000 / engp->packetInterval;
		engp->bitsPerSecond = engp->packetsPerSecond * engp->packetSize * 8;

		break;
	case M_TCP_SEND:
	case M_TCP_RECV:
		if (CopyTagField(tBuf, 29, p, "timeout"))
			engp->sessionMaxTime = atoi(tBuf);
		else
			return TPER_NOTIMEOUT;
		if (CopyTagField(tBuf, 29, p, "tcpbytes"))
			engp->tcpBytes = strtoul(tBuf, NULL, 0);
		else
			return TPER_NOTCPBYTES;
		if (CopyTagField(tBuf, 29, p, "tcpdataport"))
			engp->hostTCPDataPort = atoi(tBuf);
		break;
	}

	return 0;
}
	



/* ----------------------------------------------------- SendTestLine ---- *\


\* ----------------------------------------------------------------------- */

int	SendTestLine(TPEngine *engp)
{
	char			tbuf[400];

	switch (engp->tpMode) {

	case M_UDP_SEND:
	case M_UDP_RECV:
	case M_UDP_FDX:
		sprintf(tbuf, "TEST vmajor=%d;vminor=%d;mode=%ld;time=%lu;npackets=%lu;psize=%lu;cookie=%lu;"
			"udpsendport=%u;udprcvport=%u;client=%s",
			MAJORVERSION, MINORVERSION, engp->tpMode, engp->sessionTime, engp->nPackets,
			engp->packetSize, engp->sessCookie, 
			(unsigned int)engp->boundUDPSendPort, 
			(unsigned int)engp->boundUDPRecvPort, 
			engp->clientInfo);
		break;

	case M_TCP_SEND:
	case M_TCP_RECV:
		sprintf(tbuf, "TEST vmajor=%d;vminor=%d;mode=%ld;timeout=%lu;tcpbytes=%lu;cookie=%lu;"
			"client=%s",
			MAJORVERSION, MINORVERSION, engp->tpMode, engp->sessionMaxTime, engp->tcpBytes,
			engp->sessCookie, engp->clientInfo);
		break;

	default:
		return -1;
	}

	strcat(tbuf, "\r\n");
	return QueueCtrlData(engp, tbuf);
}


// Send TCP test data
int SendTCPTestData(TPEngine *engp)
{
	int		bytes_to_send;
	int		off;
	int		i;	
	
	off = Rand32() % RANDBUFSIZE;
	bytes_to_send = (int)(engp->tcpBytes - engp->stats.BytesSent);
	
    bytes_to_send = (int)(engp->tcpBytes - engp->stats.BytesSent);
	if (bytes_to_send > (RANDBUFSIZE - off))
        bytes_to_send = RANDBUFSIZE - off;
	if (bytes_to_send > (int)((engp->tcpBytes / 10) + 1))
		bytes_to_send = (int)((engp->tcpBytes / 10) + 1);

	i = SendTCPData(engp, engp->randBuf+off, bytes_to_send, TP_SOCKINX_DATA);

	if (i > 0) {
		if (engp->stats.BytesSent == 0) 
			gettimeofday(&(engp->stats.StartSend), NULL);
		engp->stats.BytesSent += i;
		gettimeofday(&(engp->stats.StopSend), NULL);
	}

	return i;
}




/* ----------------------------------------------------- SendStatLine ---- *\


\* ----------------------------------------------------------------------- */

int	SendStatLine(TPEngine *engp)
{
	char			tbuf[600];

	CreateLineFromStats(&(engp->stats), tbuf);
	strcat(tbuf, "\r\n");
	return QueueCtrlData(engp, tbuf);
}



TPEngine *CreateContext(void)
{
	int ret = -1;
	TPEngine	*engp = (TPEngine *) calloc(1, sizeof(TPEngine));
	if (engp == NULL) {
		goto fail;
	}
	engp->packBufSize = PACKBUFSIZE;
	engp->packetBuf = (char*)malloc(engp->packBufSize);		// Allocate the packet buffer
	if (engp->packetBuf == NULL) {
		goto fail;
	}

	engp->randBufSize = RANDBUFSIZE;
	engp->randBuf = (char*)malloc(engp->randBufSize);
	if (engp->randBuf == NULL)
		goto fail;

	ret = InitSessComm(engp);

	InitRandBuf(engp->randBuf, engp->randBufSize);

	engp->start_tcprecv_bytes = engp->start_tcpsend_bytes = START_TCP_BYTES;

	if (ret != 0)
		goto fail;

	return engp;

fail:
	if (engp != NULL) {
		if (engp->packetBuf != NULL) 
			free(engp->packetBuf);
		if (engp->randBuf != NULL)
			free(engp->randBuf);
		free(engp);
	}
	return NULL;
}



void DeleteContext(TPEngine * engp) {

	DeleteSessComm(engp);
	free(engp->randBuf);
	free(engp->packetBuf);
	free(engp);

}


void ClearBestRates(TPEngine * engp) {
	engp->bestTCPSendRate = 0;
	engp->bestTCPRecvRate = 0;
	engp->bestUDPSendRate = 0;
	engp->bestUDPRecvRate = 0;
}

/* ------------------------------------------------ CheckServerBanner ---- *\


\* ----------------------------------------------------------------------- */

int		CheckServerBanner(TPEngine *engp)
{
	int				rplCode;
	char tBuf[30];
	
	memset(tBuf, 0, 30);
	rplCode = ReplyCode(engp->ctrlMessage);
	if (rplCode == 200) {
		if (CopyTagField(tBuf, 29, engp->ctrlMessage, "vmajor"))
			engp->peerMajorVersion = atoi(tBuf);
		if (CopyTagField(tBuf, 29, engp->ctrlMessage, "vminor"))
			engp->peerMinorVersion = atoi(tBuf);
		if (CopyTagField(tBuf, 29, engp->ctrlMessage, "cookie"))
			engp->sessCookie = strtoul(tBuf, NULL, 0);
	}
	return rplCode;
}


/* ---------------------------------------------------- CheckTestReply ---- *\


\* ----------------------------------------------------------------------- */

int		CheckTestReply(TPEngine *engp)
{
	int				rplCode;
	int				port;
	char			tBuf[30];

	rplCode = ReplyCode(engp->ctrlMessage);

	if (rplCode == 210) {
		if (CopyTagField(tBuf, 29, engp->ctrlMessage+4, "tcpdataport")) {
			port = atoi(tBuf);
			if (port < 1024 || port > 65535) 
				return -1;
			engp->hostTCPDataPort = (unsigned short)port;
		}
		if (CopyTagField(tBuf, 29, engp->ctrlMessage+4, "udprcvport")) {
			port = atoi(tBuf);
			if (port < 1024 || port > 65535)
				return -1;
			engp->hostUDPRecvPort = (unsigned short)port;
		}
		if (CopyTagField(tBuf, 29, engp->ctrlMessage+4, "udpsendport")) {
			port = atoi(tBuf);
			if (port < 1024 || port > 65535)
				return -1;
			engp->hostUDPSendPort = (unsigned short)port;
		} 
	}

	return rplCode;
}



/* ---------------------------------------------------- CheckStatsMsg ---- *\


\* ----------------------------------------------------------------------- */

int		CheckStatsMsg(TPEngine *engp)
{
	int				retCode		= 0;
	TPStats			tmpStat;
	TPStats			*sp;

	retCode = GetStatsFromLine(engp->ctrlMessage, &tmpStat);
	if (retCode == 0) {
		sp = &(engp->stats);
		if ( (engp->iAmServer && (engp->tpMode == M_TCP_RECV || engp->tpMode == M_UDP_RECV)) ||
			( !(engp->iAmServer) && (engp->tpMode == M_TCP_SEND || engp->tpMode == M_UDP_SEND))) {
			sp->BytesRecvd = tmpStat.BytesRecvd;
			sp->PktsRecvd = tmpStat.PktsRecvd;
			sp->ooCount = tmpStat.ooCount;
			sp->StartRecv.tv_sec = tmpStat.StartRecv.tv_sec;
			sp->StartRecv.tv_usec = tmpStat.StartRecv.tv_usec;
			sp->StopRecv.tv_sec = tmpStat.StopRecv.tv_sec;
			sp->StopRecv.tv_usec = tmpStat.StopRecv.tv_usec;
		}
		if ((engp->iAmServer && (engp->tpMode == M_TCP_SEND || engp->tpMode == M_UDP_SEND)) ||
			(!(engp->iAmServer) && (engp->tpMode == M_TCP_RECV || engp->tpMode == M_UDP_RECV))) {
			sp->BytesSent = tmpStat.BytesSent;
			sp->PktsSent = tmpStat.PktsSent;
			sp->PktsUnSent = tmpStat.PktsUnSent;
			sp->StartSend.tv_sec = tmpStat.StartSend.tv_sec;
			sp->StartSend.tv_usec = tmpStat.StartSend.tv_usec;
			sp->StopSend.tv_sec = tmpStat.StopSend.tv_sec;
			sp->StopSend.tv_usec = tmpStat.StopSend.tv_usec;
		}
	}
	return retCode;
}


void StopContext(TPEngine *engp)
{
	CloseAllSockets(engp);
	if (engp->active == 0) return;
	FailSession(engp, TPER_USERABORT);
}

void FailSession(TPEngine *engp, int failCode)
{
	engp->failCode = failCode;
	if (engp->iAmServer)
		SetSMState(engp, SSM_FAILED);
	else
		SetSMState(engp, CLSM_FAILED);
	CloseAllSockets(engp);
	engp->active = 0;
}

void	SetTimeLimit(TPEngine *engp, int limSec)
{
	time_t	now;
	
	if (limSec == 0) {
		engp->timeLimit = 0;
	} else {
		time(&now);
		engp->timeLimit = (INT32)now + limSec;
	}
}


void	SetTimeLimit2(TPEngine *engp, int limSec)
{
	time_t	now;
	int	newlimit;

	time(&now);
	
	if ((engp->sessionMaxTime - (now - engp->startTime)) < (unsigned int)limSec)
		newlimit = (int)engp->sessionMaxTime - (int)(now - engp->startTime);
	else
		newlimit = limSec;
	if (newlimit == 0)
		newlimit = -1;

	SetTimeLimit(engp, newlimit);
}



int TimeLimitExceeded(TPEngine *engp)
{
	time_t	now;
	
	if (engp->timeLimit == 0) return 0;		// No timer active
	
	time(&now);
	if (now >= engp->timeLimit) {			// Timer has run out
		return 1;
	}
	return 0;								// No tiemout yet
}


/* ------------------------------------------------------- SetSMState ---- *\


\* ----------------------------------------------------------------------- */

void SetSMState(TPEngine *engp, int state)
{
	engp->state = state;
	if (engp->state == CLSM_COMPLETE) engp->active = 0;
	if (engp->state == CLSM_FAILED) engp->active = 0;
	return;
}


/* -------------------------------------------------------- DoSendFSM ---- *\


\* ----------------------------------------------------------------------- */

int DoClientUDPSend(
	TPEngine	*engp)
{
	int				redo;
	int				res;
	struct timeval	nowTV;

	for (redo = 1 ; redo != 0 ;) {
	
		redo = 0;
		gettimeofday(&nowTV, 0);
		
		switch (engp->state) {

		case CLSM_WAITPORTS:		// Waiting for PORTS info
			engp->callMeAgain = CMA_CLIWAITPORTS;
			if (engp->ctrlMessageComplete == 0) break;	// No reply yet
			
			res = CheckTestReply(engp);				// Look at PORTS reply
			if (res != 210) {						// Invalid PORTS reply
				FailSession(engp, TPER_BADPORTS);
				break;
			}
			ClearCtrlReply(engp);
			SetSMState(engp, CLSM_TESTLOOP);
			SetTimeLimit2(engp, MAXINT);
			engp->nextSendTV = nowTV;
			redo = 1;
			break;
			
		case CLSM_TESTLOOP:		// Running the test
			engp->callMeAgain = CMA_CLIUDPSEND;
			if (engp->ctrlMessageComplete != 0) {			// Data seen on ctrl line
				// Handle Server Data
				FailSession(engp, TPER_SRVABORT);
				break;
			}
			if (TVCompare(&nowTV, &engp->nextSendTV) >= 0 && 
				engp->curSend < engp->nPackets) {	// Time for another packet
				GenerateUDPDataPacket(engp);					// Generate new packet
				SendUDPDataPacket(engp);						// Send next packet
				TVAddUSec(&engp->nextSendTV, engp->packetInterval);
			}
			if (engp->curSend >= engp->nPackets) {			
				SetSMState(engp, CLSM_SENDSTAT);			// Data out, send STATS	
				engp->nextSendTV = nowTV;
				TVAddUSec(&engp->nextSendTV, USEC_STATDELAY);
			}
			break;

		case CLSM_SENDSTAT:
			engp->callMeAgain = CMA_CLISENDSTAT;
			if (TVCompare(&nowTV, &engp->nextSendTV) < 0) {	// Not yet time for STATS
				break;
			}
			ClearCtrlReply(engp);
			SendStatLine(engp);
			SetSMState(engp, CLSM_WAITSTAT);
			break;
		
		case CLSM_WAITSTAT:
			engp->callMeAgain = CMA_CLIWAITSTAT;
			if (engp->ctrlMessageComplete == 0) break;	// No reply yet
			
			res = CheckStatsMsg(engp);
			SetSMState(engp, CLSM_COMPLETE);			// Done	
			CloseAllSockets(engp);
			ClearCtrlReply(engp);
			break;
		}
	}
	return engp->state;
}



/* -------------------------------------------------------- DoSendFSM ---- *\


\* ----------------------------------------------------------------------- */

int DoServerUDPSend(
	TPEngine	*engp)
{
	int				res;
	time_t			now;
	struct timeval	nowTV;

	gettimeofday(&nowTV, 0);
		
	switch (engp->state) {

	case SSM_WAITNAT:			// Waiting for NAT-open packet
		engp->callMeAgain = CMA_SRVWAITNAT;
		if (engp->ctrlMessageComplete != 0) {			// Data seen on ctrl line
			// Handle Server Data
			FailSession(engp, TPER_CLIABORT);
			break;
		}
		ConsumeUDPData(engp);
		if (engp->natOpen) {
			/*
			// We got a NAT-open packet. Return it.
			tPnt = (TPPacket *)engp->packetBuf;
			gettimeofday( &(engp->stats.StopRecv), NULL );
			tPnt->Header.ServerRecvTime.tv_sec = htonl( engp->stats.StopRecv.tv_sec );
			tPnt->Header.ServerRecvTime.tv_usec = htonl( engp->stats.StopRecv.tv_usec );
			SendUDPDataPacket(engp);
*/
			SetSMState(engp, SSM_TESTLOOP);
			engp->nextSendTV = nowTV;
			SetTimeLimit(engp, engp->sessionMaxTime);
			ClearCtrlReply(engp);
		}
		break;
			
	case SSM_TESTLOOP:		// Running the test
		engp->callMeAgain = CMA_SRVUDPSEND;		
		if (engp->ctrlMessageComplete != 0) {			// Data seen on ctrl line
			// Handle Client Data
			FailSession(engp, TPER_CLIABORT);
			break;
		}

		if (TVCompare(&nowTV, &engp->nextSendTV) >= 0 &&
			engp->curSend < engp->nPackets) {	// Time for another packet
			GenerateUDPDataPacket(engp);
			SendUDPDataPacket(engp);	// Send next packet
			TVAddUSec(&engp->nextSendTV, engp->packetInterval);
		}
		if (engp->curSend >= engp->nPackets) {			
			SetSMState(engp, SSM_SENDSTAT);			// Data out, send STATS		
			engp->nextSendTV = nowTV;
			TVAddUSec(&engp->nextSendTV, USEC_STATDELAY);
		}
		break;

	case SSM_SENDSTAT:
		engp->callMeAgain = CMA_SRVSENDSTAT;		
		if (TVCompare(&nowTV, &engp->nextSendTV) < 0) {	// Not yet time for STATS
			break;
		}
		ClearCtrlReply(engp);
		SendStatLine(engp);
		SetSMState(engp, SSM_WAITSTAT);
		SetTimeLimit(engp, 10);
		break;
		
	case SSM_WAITSTAT:
		engp->callMeAgain = CMA_SRVWAITSTAT;
		if (engp->ctrlMessageComplete == 0) break;	// No reply yet
			
		res = CheckStatsMsg(engp);
		SetSMState(engp, SSM_COMPLETE);			// Done	
		CloseAllSockets(engp);
		time(&now);
//		sprintf(engp->infoMsg, "Test time was %d seconds for %lld bytes", now - engp->startTime, Stats.BytesSent);
		break;
	}
	return engp->state;
}





/* -------------------------------------------------------- DoRecvFSM ---- *\


\* ----------------------------------------------------------------------- */

int DoClientUDPRecv(
	TPEngine	*engp)
{
	int				redo;
	int				res;
	struct timeval	nowTV;

	for (redo = 1 ; redo != 0 ;) {
	
		redo = 0;
		gettimeofday(&nowTV, 0);
		
		switch (engp->state) {

		case CLSM_WAITPORTS:	// Waiting for PORTS info
			engp->callMeAgain = CMA_CLIWAITPORTS;
			if (engp->ctrlMessageComplete == 0) break;	// No reply yet
			
			res = CheckTestReply(engp);				// Look at PORTS reply
			if (res != 210) {						// Invalid PORTS reply
				FailSession(engp, TPER_BADPORTS);
				break;
			}
			ClearCtrlReply(engp);
			SetSMState(engp, CLSM_NATOPEN);
			SetTimeLimit2(engp, 5);
			engp->natCount = 0;
			engp->natOpen = 0;
			engp->packetfromPort = 0;
			engp->nextSendTV = nowTV;
			redo = 1;
			break;
			
		case CLSM_NATOPEN:		// Opening NAT ports
			engp->callMeAgain = CMA_CLINATOPEN;
			ConsumeUDPData(engp);
			if (engp->natOpen) {						// We have seen data on UDP port, go on
				ClearCtrlReply(engp);
				SetSMState(engp, CLSM_TESTLOOP);
				SetTimeLimit2(engp, MAXINT);
				break;
			}
			if (TVCompare(&nowTV, &engp->nextSendTV) >= 0) {	// Not yet time to send another
				engp->natCount += 1;
				if (engp->natCount > 5) {
					FailSession(engp, TPER_NATFAIL);
					break;
				}
				SendNATPacket(engp);						// Send next packet
				engp->nextSendTV = nowTV;
				TVAddUSec(&engp->nextSendTV, USEC_NATOPEN);
			}
			break;
			
		case CLSM_TESTLOOP:		// Running the test
			engp->callMeAgain = CMA_CLIUDPRECV;
			ConsumeUDPData(engp);
			if (engp->ctrlMessageComplete == 0) {				// No Stat seen yet
				break;
			}
			res = CheckStatsMsg(engp);
			if (res != 0) {
				FailSession(engp, TPER_STATFAIL);
				break;
			}
			SetSMState(engp, CLSM_SENDSTAT);			// Send our STATS		
			engp->nextSendTV = nowTV;
			break;

		case CLSM_SENDSTAT:
			engp->callMeAgain = CMA_CLISENDSTAT;
			if (TVCompare(&nowTV, &engp->nextSendTV) < 0) {	// Not yet time for STATS
				break;
			}
			ClearCtrlReply(engp);
			SendStatLine(engp);
			engp->nextSendTV = nowTV;
			TVAddUSec(&engp->nextSendTV, USEC_DELAYQUIT);
			SetSMState(engp, CLSM_DELAYQUIT);
			break;
		}
	}
	return engp->state;
}


/* -------------------------------------------------------- DoRecvFSM ---- *\


\* ----------------------------------------------------------------------- */

int DoServerUDPRecv(
	TPEngine	*engp)
{
	int				res;
	struct timeval	nowTV;

		gettimeofday(&nowTV, 0);
		
	switch (engp->state) {
			
	case SSM_TESTLOOP:		// Running the test
		engp->callMeAgain = CMA_SRVUDPRECV;
		ConsumeUDPData(engp);
		if (engp->ctrlMessageComplete == 0) {				// No Stat seen yet
			break;
		}
		res = CheckStatsMsg(engp);
		SetSMState(engp, SSM_SENDSTAT);			// Send our STATS		
		engp->nextSendTV = nowTV;
		break;

	case SSM_SENDSTAT:
		engp->callMeAgain = CMA_SRVSENDSTAT;
		if (TVCompare(&nowTV, &engp->nextSendTV) < 0) {	// Not yet time for STATS
			break;
		}
		ClearCtrlReply(engp);
		SendStatLine(engp);
		engp->nextSendTV = nowTV;
		TVAddUSec(&engp->nextSendTV, USEC_DELAYQUIT);
		SetSMState(engp, SSM_DELAYQUIT);
		break;
	}
	return engp->state;
}



/* ------------------------------------------------------ DoDuplexFSM ---- *\


\* ----------------------------------------------------------------------- */

int DoClientUDPDuplex(
	TPEngine	*engp)
{
	int				redo;
	int				res;
	time_t			now;
	struct timeval	nowTV;

	for (redo = 1 ; redo != 0 ;) {
	
		redo = 0;
		gettimeofday(&nowTV, 0);
		
		switch (engp->state) {

		case CLSM_WAITPORTS:	// Waiting for PORTS info
			engp->callMeAgain = CMA_CLIUDPFDX;
			if (engp->ctrlMessageComplete == 0) break;	// No reply yet
			
			res = CheckTestReply(engp);				// Look at PORTS reply
			if (res != 210) {						// Invalid PORTS reply
				FailSession(engp, TPER_BADPORTS);
				break;
			}
			ClearCtrlReply(engp);
			SetSMState(engp, CLSM_NATOPEN);
			SetTimeLimit2(engp, 5);
			engp->natCount = 0;
			engp->natOpen = 0;
			engp->nextSendTV = nowTV;
			redo = 1;
			break;
			
		case CLSM_NATOPEN:		// Opening NAT ports
			engp->callMeAgain = CMA_CLINATOPEN;		
			if (engp->ctrlMessageComplete) {
				if (strncmp(engp->ctrlMessage, "NATACK ", 7) == 0) {
					engp->natOpen = 1;
					SetSMState(engp, CLSM_TESTLOOP);
					SetTimeLimit2(engp, MAXINT);
					ClearCtrlReply(engp);
				}
				else 
					FailSession(engp, TPER_BADNATACK);
				break;
			}
			if (TVCompare(&nowTV, &engp->nextSendTV) >= 0) {	// Not yet time to send another
				engp->natCount += 1;
				if (engp->natCount > 5) {
					FailSession(engp, TPER_NATFAIL);
					break;
				}
				SendNATPacket(engp);						// Send next packet
				engp->nextSendTV = nowTV;
				TVAddUSec(&engp->nextSendTV, USEC_NATOPEN);
			}
			break;
			
		case CLSM_TESTLOOP:		// Running the test
			engp->callMeAgain = CMA_CLIUDPFDX;
			ConsumeUDPData(engp);
			if (engp->ctrlMessageComplete != 0) {				// Data seen on ctrl line
				// Handle Server Data
				FailSession(engp, TPER_SRVABORT);
				break;
			}
			if (TVCompare(&nowTV, &engp->nextSendTV) >= 0 &&
				engp->curSend < engp->nPackets) {// Time for another packet
				GenerateUDPDataPacket(engp);
				SendUDPDataPacket(engp);					// Send next packet
				TVAddUSec(&engp->nextSendTV, engp->packetInterval);
			}
			if (engp->curSend >= engp->nPackets) {	
				SetSMState(engp, CLSM_WAITFDX);
				engp->nextSendTV = nowTV;
				TVAddUSec(&engp->nextSendTV, USEC_STATDELAY);
			}
			break;

		case CLSM_WAITFDX:
			engp->callMeAgain = CMA_CLIUDPFDX;
			ConsumeUDPData(engp);
			if (engp->ctrlMessageComplete != 0) {
				FailSession(engp, TPER_SRVABORT);
				break;
			}
			if (TVCompare(&nowTV, &engp->nextSendTV) >= 0 || 
				engp->stats.PktsRecvd >= engp->stats.PktsSent) {
				SetSMState(engp, CLSM_SENDSTAT);			// Send stats
			}
			break;

		case CLSM_SENDSTAT:
			engp->callMeAgain = CMA_CLISENDSTAT;
			ClearCtrlReply(engp);
			SendStatLine(engp);
			SetSMState(engp, CLSM_WAITSTAT);
			break;
		
		case CLSM_WAITSTAT:
			engp->callMeAgain = CMA_CLIWAITSTAT;
			if (engp->ctrlMessageComplete == 0) break;		// No reply yet
			
			res = CheckStatsMsg(engp);
			SetSMState(engp, CLSM_COMPLETE);			// Done	
			CloseAllSockets(engp);
			time(&now);
			break;
		}
	}
	return engp->state;
}

int SendUDPDataPacket(TPEngine *engp) {
	int res;
	int sendLength;
	struct timeval now;
	TPPacket * tPnt;

	gettimeofday(&now, NULL);
	tPnt = (TPPacket *)(engp->packetBuf);

	if (engp->iAmServer) {
		tPnt->Header.ServerSendTime.tv_sec = htonl(now.tv_sec);
		tPnt->Header.ServerSendTime.tv_usec = htonl(now.tv_usec);
	}
	else {
		tPnt->Header.ClientSendTime.tv_sec = htonl(now.tv_sec);
		tPnt->Header.ClientSendTime.tv_usec = htonl(now.tv_usec);
	}

	if (engp->curSend == 0) {
		engp->stats.StartSend.tv_sec = now.tv_sec;
		engp->stats.StartSend.tv_usec = now.tv_usec;
	}

	sendLength = engp->packetSize - IP_UDP_SIZE;

	res = SendUDPPacket(engp, sendLength);

	if (res != sendLength) {
		engp->stats.PktsUnSent++;
	} else {
		engp->stats.PktsSent++;
		engp->stats.BytesSent += engp->packetSize;
	}
	gettimeofday(&(engp->stats.StopSend), NULL);
	engp->curSend += 1;

	return 0;
}

void GenerateUDPDataPacket(TPEngine *engp) {
	TPPacket * tPnt;
	int sendLength;

	tPnt = (struct tpPacket *) engp->packetBuf;
	if (tPnt == 0) return;				// No packet buffer set
	
	// Set up packet header and random payload
	
	sendLength = engp->packetSize - IP_UDP_SIZE;
		
	tPnt->Header.Sequence = htonl(engp->curSend);

    tPnt->Header.ServerRecvTime.tv_sec = 0;
    tPnt->Header.ServerRecvTime.tv_usec = 0;
    tPnt->Header.ServerSendTime.tv_sec = 0;
    tPnt->Header.ServerSendTime.tv_usec = 0;
    tPnt->Header.DataSize = htonl(engp->packetSize);
	tPnt->Header.Cookie = htonl(engp->sessCookie);
    
	memcpy(tPnt->Data, &(engp->randBuf[ ( Rand32() % ( engp->randBufSize / 2 ) ) ]),
				sendLength - sizeof(struct tpHeader));

}


/* ------------------------------------------------------ DoDuplexFSM ---- *\


\* ----------------------------------------------------------------------- */

int DoServerUDPDuplex(
	TPEngine	*engp)
{


	int				res;
	struct timeval	nowTV;

	gettimeofday(&nowTV, 0);
		
	switch (engp->state) {
	
		
	case SSM_WAITNAT:			// Waiting for NAT-open packet
		engp->callMeAgain = CMA_SRVWAITNAT;
		if (engp->ctrlMessageComplete != 0) {			// Data seen on ctrl line
			// Handle Server Data
			FailSession(engp, TPER_CLIABORT);
			break;
		}
		ConsumeUDPData(engp);
		if (engp->natOpen) {
			char buf[50];
			sprintf(buf, "NATACK cookie=%lu\r\n", engp->sessCookie);
			QueueCtrlData(engp, buf);
			SetSMState(engp, SSM_TESTLOOP);
			SetTimeLimit(engp, engp->sessionMaxTime);
		}
		break;

	case SSM_TESTLOOP:		// Running the test
		engp->callMeAgain = CMA_SRVUDPFDX;
		ConsumeUDPData(engp);
		if (engp->ctrlMessageComplete == 0) break;

		// Is it a stats message?
		res = CheckStatsMsg(engp);
		if (res != 0) {
			FailSession(engp, TPER_STATFAIL);
			break;
		}
		ClearCtrlReply(engp);
		SetSMState(engp, SSM_SENDSTAT);
		engp->nextSendTV = nowTV;
		TVAddUSec(&engp->nextSendTV, USEC_STATDELAY);
		break;

	case SSM_SENDSTAT:
		engp->callMeAgain = CMA_SRVSENDSTAT;
		if (TVCompare(&nowTV, &engp->nextSendTV) < 0) {	// Not yet time for STATS
			break;
		}
		SendStatLine(engp);
		engp->nextSendTV = nowTV;
		TVAddUSec(&engp->nextSendTV, USEC_DELAYQUIT);
		SetSMState(engp, SSM_DELAYQUIT);
		break;

	}
	return engp->state;
}




/* -------------------------------------------------------- DoTCPRecv ---- *\


\* ----------------------------------------------------------------------- */

int DoClientTCPRecv(
	TPEngine	*engp)
{
	int				res;
	struct timeval	nowTV;

	gettimeofday(&nowTV, 0);
		
	switch (engp->state) {

	case CLSM_WAITPORTS:	// Waiting for PORTS info
		engp->callMeAgain = CMA_CLIWAITPORTS;
		if (engp->ctrlMessageComplete == 0) break;	// No reply yet
			
		res = CheckTestReply(engp);				// Look at PORTS reply
		if (res != 210) {						// Invalid PORTS reply
			FailSession(engp, TPER_BADPORTS);
			break;
		}
		ClearCtrlReply(engp);
		SetTimeLimit2(engp, 20);
		engp->tcpDataConnectComplete = 0;
		res = InitConnectTCP(engp, TP_SOCKINX_DATA);		
		if (res == 0) {
			SetSMState(engp, CLSM_DATACONNECTING);
		} else {
			FailSession(engp, TPER_NODATA);
		}
		break;

	case CLSM_DATACONNECTING:
		engp->callMeAgain = CMA_CLIDATACONNECT;
		if (engp->tcpDataConnectComplete) {
			SetSMState(engp, CLSM_TESTLOOP);
			SetTimeLimit2(engp, MAXINT);
		}
		else {
			res = ContinueConnectTCP(engp);
			if (res == 0) {
				// success
			} else {
				FailSession(engp, TPER_NODATA);
			}
		}
		break;
	
	case CLSM_TESTLOOP:
		engp->callMeAgain = CMA_CLITCPRECV;
		res = ConsumeTCPTestData(engp);
        if (res != 0) {
			FailSession(engp, TPER_DATACLOSED);
			return 0;
		}
		if (engp->ctrlMessageComplete == 1) {
			res = CheckStatsMsg(engp);
			SendStatLine(engp);
			engp->nextSendTV = nowTV;
			TVAddUSec(&engp->nextSendTV, USEC_DELAYQUIT);
			SetSMState(engp, CLSM_DELAYQUIT);
		}
		break;
	}
	return engp->state;
}

/* -------------------------------------------------------- DoTCPSend ---- *\


\* ----------------------------------------------------------------------- */

int DoClientTCPSend(
	TPEngine	*engp)
{

	int				res;
	int				off;
	int				bytesToSend;
	struct timeval	nowTV;

	gettimeofday(&nowTV, 0);
		
	switch (engp->state) {

	case CLSM_WAITPORTS:	// Waiting for PORTS info
		engp->callMeAgain = CMA_CLIWAITPORTS;
		if (engp->ctrlMessageComplete == 0) break;	// No reply yet
			
		res = CheckTestReply(engp);				// Look at PORTS reply
		if (res != 210) {						// Invalid PORTS reply
			FailSession(engp, TPER_BADPORTS);
			break;
		}
		ClearCtrlReply(engp);
		SetTimeLimit2(engp, 20);
		engp->tcpDataConnectComplete = 0;
		res = InitConnectTCP(engp, TP_SOCKINX_DATA);		
		if (res == 0) {
			SetSMState(engp, CLSM_DATACONNECTING);
		} else {
			FailSession(engp, TPER_NODATA);
		}
		break;

	case CLSM_DATACONNECTING:
		engp->callMeAgain = CMA_CLIDATACONNECT;
		if (engp->tcpDataConnectComplete) {
			SetSMState(engp, CLSM_TESTLOOP);
			SetTimeLimit2(engp, MAXINT);
		}
		else {
			res = ContinueConnectTCP(engp);
			if (res == 0) {
				// success
			} else {
				FailSession(engp, TPER_NODATA);
			}
		}
		break;
	
	case CLSM_TESTLOOP:		// Running the test
		engp->callMeAgain = CMA_CLITCPSEND;
		if (engp->ctrlMessageComplete != 0) {			// Data seen on ctrl line
			// Handle Client Data
			FailSession(engp, TPER_SRVABORT);
			break;
		}
		if (engp->stats.BytesSent < engp->tcpBytes) {
			off = ((unsigned int)Rand32()) % ((RANDBUFSIZE * 2)/3);
			bytesToSend = (int)(engp->tcpBytes - engp->stats.BytesSent);
			if (bytesToSend > (RANDBUFSIZE - off))
				bytesToSend = RANDBUFSIZE - off;
			if (bytesToSend > (int)((engp->tcpBytes / 10) + 1))
				bytesToSend = (int)((engp->tcpBytes / 10) + 1);

			res = SendTCPData(engp, engp->randBuf + off, bytesToSend, TP_SOCKINX_DATA);

			if (res > 0) {
				if (engp->stats.BytesSent == 0)
					engp->stats.StartSend = nowTV;
				engp->stats.BytesSent += res;
				engp->stats.StopSend = nowTV;
			}
            else if (res < 0) {
                FailSession(engp, TPER_DATACLOSED);
            }
		}
		else {
			SetSMState(engp, CLSM_SENDSTAT);			// Data out, send STATS		
			engp->nextSendTV = nowTV;
			TVAddUSec(&engp->nextSendTV, USEC_STATDELAY);
		}
		break;

	case CLSM_SENDSTAT:
		if (TVCompare(&nowTV, &engp->nextSendTV) < 0) {	// Not yet time for STATS
			engp->callMeAgain = (engp->nextSendTV.tv_sec - nowTV.tv_sec) * 1000000 +
				(engp->nextSendTV.tv_usec - nowTV.tv_usec);
		}
		else
			engp->callMeAgain = CMA_CLISENDSTAT;
		ClearCtrlReply(engp);
		SendStatLine(engp);
		SetSMState(engp, CLSM_WAITSTAT);
		break;
		
	case CLSM_WAITSTAT:
		
		if (engp->ctrlMessageComplete == 0) break;	// No reply yet
			
		res = CheckStatsMsg(engp);
		SetSMState(engp, CLSM_COMPLETE);			// Done	
		CloseAllSockets(engp);
		break;
	}
	return engp->state;
}





int	SendHeloLine(TPEngine *engp)
{
	char			tBuf[200];

	sprintf(tBuf, "HELO vmajor=%d;vminor=%d\r\n", MAJORVERSION, MINORVERSION);
	return QueueCtrlData(engp, tBuf);
}


void ClearServerList(TPEngine *engp) {
	int i;
	for (i = 0; i < MAX_SERVERS; i++) {
		memset(engp->serverNameList[i], 0, MAX_SERVER_NAME);
		memset(engp->serverInfoList[i], 0, MAX_SERVER_INFO);
		engp->serverPortList[i] = (USHORT)0;
	}
	engp->numServers = 0;
}

int AddServerToList(TPEngine *engp, char * str) {
	char tBuf[MAX_SERVER_NAME + MAX_SERVER_INFO];

	if (engp->numServers >= MAX_SERVERS)
		return TPER_MAXSERVERS;
	// Expecting string in format: hostname=name/ip;port=no;proto=x;info=y


	memset(tBuf, 0, MAX_SERVER_NAME + MAX_SERVER_INFO);

	// Server address is mandatory
	if (CopyTagField(tBuf, MAX_SERVER_NAME - 1, str, "hostname")) 
		strcpy(engp->serverNameList[engp->numServers], tBuf);
	else
		return TPER_NOSERVNAME;


	memset(tBuf, 0, MAX_SERVER_NAME + MAX_SERVER_INFO);

	// Check if we support server protocol
	if (CopyTagField(tBuf, MAX_SERVER_NAME, str, "proto")) {
		if (strcmp(tBuf, "TCP"))
			return TPER_UNSUPPROTO;
	}


	memset(tBuf, 0, MAX_SERVER_NAME + MAX_SERVER_INFO);

	// Server port is optional
	if (CopyTagField(tBuf, MAX_SERVER_NAME, str, "port"))
		engp->serverPortList[engp->numServers] = (USHORT)(atoi(tBuf));
	else
		engp->serverPortList[engp->numServers] = (USHORT)DEFAULT_CONTROL_PORT;


	memset(tBuf, 0, MAX_SERVER_NAME + MAX_SERVER_INFO);

	// Server info is optional
	if (CopyTagField(tBuf, MAX_SERVER_INFO, str, "info"))
		strncpy(engp->serverInfoList[engp->numServers], tBuf, MAX_SERVER_INFO);

	engp->numServers += 1;

	return 0;		
}

int DoQueryMaster(
	TPEngine	*engp)
{
	int				code;
	struct timeval	nowTV;

	gettimeofday(&nowTV, 0);
		
	switch (engp->state) {

	case CLSM_SENDMHELO:		// Connected - Send HELO 201

		ClearCtrlReply(engp);
		SendHeloLine(engp);							// Send our HELO message
		SetSMState(engp, CLSM_SERVERLIST);
		SetTimeLimit2(engp, MAXINT);
		break;
		
	case CLSM_SERVERLIST:	// Waiting for Server list

		if (engp->ctrlMessageComplete == 0) break;		// No reply yet
			
		code = ReplyCode(engp->ctrlMessage);
		if (code == 210) {
			if (strlen(engp->ctrlMessage) >= 4) {
				if (engp->wipeFlag) {
					ClearServerList(engp);
					engp->wipeFlag = 0;
				}
				AddServerToList(engp, engp->ctrlMessage+4);
				if (engp->ctrlMessage[3] != '-') {
					SetSMState(engp, CLSM_COMPLETE);
					CloseAllSockets(engp);
					engp->active = 0;
				}
			} 
			else {
				FailSession(engp, TPER_BADMASTERREPLY);
			}
		} else {
			if (code >= 400 && code < 500) {				
				FailSession(engp, TPER_MASTERBUSY);				

			} else if (code >= 500 && code < 600) {				
			FailSession(engp, TPER_MASTERDENIED);					

			} else {
				FailSession(engp, TPER_BADMASTERREPLY);
			}
		}
		ClearCtrlReply(engp);
			
		break;
			
	}
	return engp->state;
}

int DoServerTCPSend(TPEngine *engp) {

	struct timeval nowTV;
	int res = 0;
	int off;
	int bytesToSend;

	gettimeofday(&nowTV, 0);
		
	switch (engp->state) {

	case SSM_DATALISTEN:
		engp->callMeAgain = CMA_SRVDATALISTEN;
		res = AcceptClient(engp, TP_SOCKINX_DATA);
		if (res != 0) {
			FailSession(engp, TPER_ACCEPTFAIL);
			return 0;
		}
		if (engp->tcpDataConnectComplete) 
			SetSMState(engp, SSM_TESTLOOP);
		break;

	case SSM_TESTLOOP:		// Running the test
		engp->callMeAgain = CMA_SRVTCPTEST;
		if (engp->ctrlMessageComplete != 0) {			// Data seen on ctrl line
			// Handle Client Data
			FailSession(engp, TPER_CLIABORT);
			break;
		}

		if (engp->stats.BytesSent < engp->tcpBytes) {
			off = ((unsigned int)Rand32()) % ((RANDBUFSIZE * 2)/3);
			bytesToSend = (int)(engp->tcpBytes - engp->stats.BytesSent);
			if (bytesToSend > (RANDBUFSIZE - off))
				bytesToSend = RANDBUFSIZE - off;
			if (bytesToSend > (int)((engp->tcpBytes / 10) + 1))
				bytesToSend = (int)((engp->tcpBytes / 10) + 1);

			res = SendTCPData(engp, engp->randBuf + off, bytesToSend, TP_SOCKINX_DATA);

			if (res > 0) {
				if (engp->stats.BytesSent == 0)
					engp->stats.StartSend = nowTV;
				engp->stats.BytesSent += res;
				engp->stats.StopSend = nowTV;
			}
            else if (res < 0) {
                FailSession(engp, TPER_DATACLOSED);
            }
		}
		else {
			SetSMState(engp, SSM_SENDSTAT);			// Data out, send STATS		
			engp->nextSendTV = nowTV;
			TVAddUSec(&engp->nextSendTV, USEC_STATDELAY);
		}
		break;

	case SSM_SENDSTAT:
		if (TVCompare(&nowTV, &engp->nextSendTV) < 0) {	// Not yet time for STATS
			engp->callMeAgain = (engp->nextSendTV.tv_sec - nowTV.tv_sec) * 1000000 +
				(engp->nextSendTV.tv_usec - nowTV.tv_usec);
			break;
		}
		else
			engp->callMeAgain = CMA_SRVSENDSTAT;
		ClearCtrlReply(engp);
		SendStatLine(engp);
		SetSMState(engp, SSM_WAITSTAT);
		SetTimeLimit(engp, 10);
		break;
		
	case SSM_WAITSTAT:
		engp->callMeAgain = CMA_SRVWAITSTAT;
		if (engp->ctrlMessageComplete == 0) break;	// No reply yet			
		res = CheckStatsMsg(engp);
		SetSMState(engp, SSM_COMPLETE);			// Done	
		CloseAllSockets(engp);
//		sprintf(engp->infoMsg, "Test time was %d seconds for %lld bytes", now - engp->startTime, Stats.BytesSent);
		break;
	}
	
	
	return 0;
}

int DoServerTCPRecv(TPEngine *engp) {
	struct timeval nowTV;
	int res = 0;

	gettimeofday(&nowTV, 0);

	switch (engp->state) {

	case SSM_DATALISTEN:
		engp->callMeAgain = CMA_SRVDATALISTEN;
		res = AcceptClient(engp, TP_SOCKINX_DATA);
		if (res != 0) {
			FailSession(engp, TPER_ACCEPTFAIL);
			return 0;
		}
		if (engp->tcpDataConnectComplete) {
			SetSMState(engp, SSM_TESTLOOP);
			ClearCtrlReply(engp);
		}
		break;

	case SSM_TESTLOOP:
		engp->callMeAgain = CMA_SRVTCPTEST;
		res = ConsumeTCPTestData(engp);
        if (res != 0) {
            FailSession(engp, TPER_DATACLOSED);
            return 0;
        }
		if (engp->ctrlMessageComplete == 1) {
			res = CheckStatsMsg(engp);
			SendStatLine(engp);
			engp->nextSendTV = nowTV;
			TVAddUSec(&engp->nextSendTV, USEC_DELAYQUIT);
			SetSMState(engp, SSM_DELAYQUIT);
		}
	}

	return 0;
}





/*
 * Fill buffer with random numbers.
 */
void InitRandBuf(void *buf, int len)              
{
        struct timeval tm;
        int             i;

        gettimeofday( &tm, NULL );                 
        srand( tm.tv_sec | tm.tv_usec );
        for ( i = 0; i < (int)(len / sizeof( int )); i++ )
                *((int *)(&(((char *)buf)[i * sizeof( int )]))) = Rand32();
}
