/*
 *	
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tpio_mac.c - Open transport routines for mac
 *
 * Written by
 *  Hans Green <hg@3tag.com>
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
#include <string.h>
#include <time.h>
#include <OpenTransport.h>
#include <MacErrors.h>

#include "tpengine.h"
#include "tpclient.h"
#include "tpio.h"
#include "tpio_mac.h"

// Status server definitions

#define DO_SEND_STAT_UDP	0				// Set to 1 to really send stat packet
#define STAT_UDP_SERVER		0xC02490C7		// Set to address of server (192.36.144.199)
#define STAT_UDP_PORT		3264			// Port on stat server

enum {
	kTransferBufferSize = 66000
};

// Local protos

static int OpenTcpEndpoints(TPEngine *engp);

// Local data

static Boolean			gOTInited;		// Set when Open Transport has been setup
static ThreadID			progressThread;	// ID of extra thread
static int				pThreadCount;	// Counter to give extra thread something to do
static OTNotifyUPP		myNotifyUPP;

// Comm specific data area

#define CONN_STATE_DISCONNECTED	0		// Endpoint is disconnected
#define CONN_STATE_CONNECTING	1		// Connect in progress
#define CONN_STATE_CONNECTED	2		// Endpoint is connected

// Info for notification routines

typedef struct NotifyContext {
	struct TPSockInfo	*sockInfo;		// Ptr to data for this endpoint
} NotifyContext;

typedef struct TPSockInfo {
	EndpointRef			ep;					// Ref to this endpoint
	int					epBound;			// Set when ep bound
	int					connEventSeen;		// Set when connect event received
	int					dataSeen;			// Set when data received
	int					gotRcvDisconnect;	// Set when disc event received
	int					gotRcvOrdRel;		// Set when ordrel received
	int					connState;			// CONN_STATE_XXXXX
	char				id[10];				// Ident for debugging
	NotifyContext		context;			// Context for notification
} TPSockInfo;

typedef struct TPOTInfo {
	InetSvcRef 			iSvc;				// Inet service Svc
	InetHostInfo		tpHostInfo;			// Host info for name lookup
	
	TPSockInfo			ctrlInfo;			// Host ctrl connection
	struct InetAddress	ctrlSin;	

	TPSockInfo			dataInfo;			// Host data connection
	
	TPSockInfo			udpSendInfo;		// Our UDP send port
	int					udpSendPort;
	struct InetAddress	udpSendSin;
	
	TPSockInfo			udpRcvInfo;			// Our UDP rcv port
	int					udpRcvPort;
	struct InetAddress	udpRcvSin;
	
	struct InetAddress	udpSrvRcvSin;		// Host UDP rcv port
	struct InetAddress	udpSrvSndSin;		// Host UDP send port

	Ptr					xferBuf;			// TCP Ctrl rcv buffer
	int					xferCharCnt;
	int					xferCharPos;
	int					replyCnt;
	
	char				sendCDBuf[1000];	// TCP ctrl send buffer
	int					sendCDPos;
	int					sendCDCnt;
	
	Ptr					junkBuf;			// TCP data rcv buffer
} TPOTInfo;


static OTTimeStamp	startTime;				// Base timestamp for gettimeofday simulator
static long			t0 = 0;					// Base time for gettimeofday simulator

/* ----------------------------------------------- ProgressThreadBody ---- *\

	Simple do-nothing thread used to enable yielding OT.
	
\* ----------------------------------------------------------------------- */

static pascal OSStatus ProgressThreadBody(void *junkParam)
{
#pragma unused(junkParam)

	while (true) {
		pThreadCount += 1;
		YieldToAnyThread();
	}

	return noErr;	
}

/* -------------------------------------------------------- TPOTSetup ---- *\

	Setup Open Transport things needed for TPTest
	
\* ----------------------------------------------------------------------- */

OSStatus TPOTSetup(void)
{
	OSErr		err;
	FILE		*fp;
	int			useLog		= 0;
	KeyMap		myMap;
	
	err = InitOpenTransportInContext(kInitOTForApplicationMask, nil);
	if (err != noErr) return err;
	
	gOTInited = true;			// Indicate success

	// Log file is used if:
	//
	//   There is a file named "TPLogAlways.txt"
	//   or
	//   The program is started with the option key down
	
	fp = fopen("TPLogAlways.txt", "r");
	if (fp) {
		fclose(fp);
		useLog = 1;
	}
	GetKeys(myMap);
	if (myMap[1] == 4) useLog = 1;

	if (useLog) TrcSetFile("TPTest.log");
	TrcEnable(1);
	TrcLog(1, ">>");
	TrcLog(1, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> STARTING UP >>>>");
	TrcLog(1, ">>");
	
	// Create support thread to enable yielding OT operation
	
	err = NewThread(kCooperativeThread,
						NewThreadEntryUPP((void *) &ProgressThreadBody), nil,
						0, kCreateIfNeeded, nil, &progressThread);

	Report("Version setup done");
	Report("Init completed OK");
	return err;
}

/* ----------------------------------------------------- TPOTShutDown ---- *\

	Terminate Open Transport operations
	
	Call this once before quitting program
	
\* ----------------------------------------------------------------------- */

void TPOTShutDown(void)
{
	TrcLog(1, ">>");
	TrcLog(1, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> SHUTTING DOWN >>>>");
	TrcLog(1, ">>");

	if (progressThread) DisposeThread(progressThread, nil, true);
	if (gOTInited) CloseOpenTransportInContext(nil);
}


/* ------------------------------------------------ Init_gettimeofday ---- *\

	Setup initial timer base to enable microsecond timing
	
\* ----------------------------------------------------------------------- */

int Init_gettimeofday(void)
{
	OTGetTimeStamp(&startTime);
	time(&t0);
	return 0;
}

/* ----------------------------------------------------- gettimeofday ---- *\

	Get time of day into a struct timeval.
	
	NOTE! Only works for about 2000 seconds after Init_gettimeofday
	since it is based on OTElapsedMicroseconds which overflows signed ints
	after 2**31 - 1 microsecs.
		
\* ----------------------------------------------------------------------- */

void gettimeofday(struct timeval *tvp, void *argp)
{
	unsigned long delta;
	
	argp;			// touch
	
	if (t0 == 0) Init_gettimeofday();			// Auto-setup first time

	delta = OTElapsedMicroseconds(&startTime);
	tvp->tv_sec = t0 + (delta / 1000000);
	tvp->tv_usec = delta % 1000000;
}


/* ----------------------------------------------------------- rand32 ---- *\

	Generate 32-bit random number
	
\* ----------------------------------------------------------------------- */

int Rand32() {
	int		res = rand();
	return res * rand();
}

/* ------------------------------------------------- YieldingNotifier ---- *\

	This simple notifier checks for kOTSyncIdleEvent and
	when it gets one calls the Thread Manager routine
	YieldToAnyThread.  Open Transport sends kOTSyncIdleEvent
	whenever it's waiting for something, eg data to arrive
	inside a sync/blocking OTRcv call.  In such cases, we
	yield the processor to some other thread that might
	be doing useful work.

\* ----------------------------------------------------------------------- */

static pascal void YieldingNotifier(void *contextPtr, OTEventCode code, 
									   OTResult result, void *cookie)
{
	NotifyContext	*ncp	= (NotifyContext *) contextPtr;
	TPSockInfo		*tsip	= 0;
	
	#pragma unused(result)
	#pragma unused(cookie)
	OSStatus junk;

	if (ncp) tsip = ncp->sockInfo;
	
	//if (code != kOTSyncIdleEvent) TrcLog(1, "Notify: %X", code);
	
	switch (code) {	
		case kOTSyncIdleEvent:
			junk = YieldToAnyThread();
			break;
		
		case T_CONNECT:
			if (tsip) {
				tsip->connEventSeen = 1;
				if (0) TrcLog(1, "YieldingNotifier: Connect Completed %08X", tsip->ep);
			}
			break;
			
		case T_BINDCOMPLETE:
			if (tsip) tsip->connEventSeen = 1;
			if (0) TrcLog(1, "YieldingNotifier: Bind Completed");
			break;
			
		case T_OPENCOMPLETE:
			if (tsip) tsip->connEventSeen = 1;
			if (0) TrcLog(1, "YieldingNotifier: Open Completed");
			break;
			
		case T_DATA:
			if (tsip) {
				tsip->dataSeen = 1;
				if (0) TrcLog(1, "YieldingNotifier: Got data on  %08X [%s] ", tsip->ep, tsip->id);
			}
			break;
			
		case T_DISCONNECT:
			if (tsip) {
				tsip->gotRcvDisconnect = 1;
				if (0) TrcLog(1, "YieldingNotifier: Got Disconnect on %08X [%s] ", tsip->ep, tsip->id);
			}
			break;
					
		case T_ORDREL:		
			if (0) TrcLog(1,"YieldingNotifier: Got OrdRel");
			if (tsip) tsip->gotRcvOrdRel = 1;
			break;
			
		default:
			// do nothing
			break;
	}
}

/* ----------------------------------------------------- InitSessComm ---- *\

	Create the TPOTInfo structure to be used by this packet
	
\* ----------------------------------------------------------------------- */

int InitSessComm(TPEngine *engp)
{
	TPOTInfo		*top;
	OSStatus		err;

	top = (TPOTInfo *) engp->ctrlRefp;
	if (top != 0) return 0;				// Already set up

	top = calloc(1, sizeof(TPOTInfo));
	
	top->xferBuf = OTAllocMemInContext(kTransferBufferSize, nil);
	top->junkBuf = OTAllocMemInContext(kTransferBufferSize, nil);
	engp->ctrlRefp = top;
		
	top->iSvc = OTOpenInternetServicesInContext(kDefaultInternetServicesPath, NULL, &err, 0);
	TrcLog(1, "OTOpenInternetServices gave %d", err);
	return 0;
}

/* --------------------------------------------------- DeleteSessComm ---- *\

	Release all storage allocated by the I/O packet
	
\* ----------------------------------------------------------------------- */

void DeleteSessComm(TPEngine *engp)
{
	TPOTInfo		*top;
	
	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return;				// Never allocated
	
	CloseAllSockets(engp);

	if (top->xferBuf != 0) {			// Release the ctrl data buffer
		OTFreeMem(top->xferBuf);
	}
	if (top->junkBuf != 0) {			// Release the tcp junk buffer
		OTFreeMem(top->junkBuf);
	}
	if (top->iSvc != 0) {				// Also close Inet services
		OTCloseProvider(top->iSvc);
	}
	free(top);							// And the local data
	engp->ctrlRefp = 0;
}

/* ---------------------------------------------------------- InitUDP ---- *\

	Create the OT endpoints we will use for UDP transfers
	
\* ----------------------------------------------------------------------- */

int InitUDP(TPEngine *engp)
{
	OSStatus 			err;
	EndpointRef 		ep;
	TBind				req, ret;
	TPOTInfo			*top;
	TPSockInfo			*tsip;
	struct InetAddress	tmpSin;
	
	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return -1;					// Never allocated

	// Open the UDP send endpoint for synchronous use
	
	if (top->udpSendInfo.ep == 0) {
		ep = OTOpenEndpointInContext(OTCreateConfiguration(kUDPName), 0, nil, &err, nil);
		if (err != noErr) {
			TrcLog(1,"InitUDP: send open failed with %d", err);
			return err;
		}
		tsip = &top->udpSendInfo;
		tsip->ep = ep;
		
		err = OTSetSynchronous(ep);
		err = OTSetBlocking(ep);
		
		// bind our send udp endpoint the to any local port

		top->udpSendPort = 0;								// Get any port		
		OTInitInetAddress(&top->udpSendSin, top->udpSendPort, (InetHost) 0);

		req.addr.len = sizeof(struct InetAddress);
		req.addr.buf = (UInt8*) &top->udpSendSin;
		req.qlen = 1;										// don't care for udp	
		ret.addr.maxlen = sizeof(struct InetAddress);
		ret.addr.buf = (UInt8*) &tmpSin;

		err = OTBind(ep, &req, &ret);
		if (err != kOTNoError) {
			TrcLog(1,"InitUDP: Error - bind udp send ep retured %d", err);
			return err;
		}
		engp->myUDPSendPort = tmpSin.fPort;
		tsip->epBound = 1;
		tsip->context.sockInfo = tsip;
		strcpy(tsip->id, "UDP-send");

		err = OTInstallNotifier(ep, myNotifyUPP, &tsip->context);
	}

	// Open the UDP recv endpoint

	if (top->udpRcvInfo.ep == 0) {
		ep = OTOpenEndpointInContext(OTCreateConfiguration(kUDPName), 0, nil, &err, nil);
		if (err != noErr) {
			TrcLog(1,"InitUDP: rcv open failed with %d", err);
			return err;
		}
		tsip = &top->udpRcvInfo;
		tsip->ep = ep;
		
		// bind our recv endpoint to any local port

		top->udpRcvPort = 0;	
		OTInitInetAddress(&top->udpRcvSin, top->udpRcvPort, (InetHost) 0);

		req.addr.len = sizeof(struct InetAddress);
		req.addr.buf = (UInt8*) &top->udpRcvSin;
		req.qlen = 1;										// don't care for udp	
		ret.addr.maxlen = sizeof(struct InetAddress);
		ret.addr.buf = (UInt8*) &tmpSin;

		err = OTBind(ep, &req, &ret);
		if (err != kOTNoError) {
			TrcLog(1, "InitUDP: Error - bind udp rcv ep retured %d", err);
			return err;
		}
		tsip->epBound = 1;

		engp->myUDPRecvPort = tmpSin.fPort;
		err = OTSetAsynchronous(ep);
		
		tsip->context.sockInfo = tsip;
		strcpy(tsip->id, "UDP-recv");

		err = OTInstallNotifier(ep, myNotifyUPP, &tsip->context);
	}
	
	return noErr;
}

/* --------------------------------------------- SetConnCompleteFlags ---- *\

	Set up ConnectComplete flags in common struct based on states
	in our local TPOTInfo
	
\* ----------------------------------------------------------------------- */

static void SetConnCompleteFlags(TPEngine *engp)
{
	TPOTInfo		*top		= 0;

	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return;					// Not setup correctly

	if (top->ctrlInfo.connState == CONN_STATE_CONNECTED) engp->tcpCtrlConnectComplete = 1;
	else engp->tcpCtrlConnectComplete = 0;
	if (top->dataInfo.connState == CONN_STATE_CONNECTED) engp->tcpDataConnectComplete = 1;
	else engp->tcpDataConnectComplete = 0;
}

/* ----------------------------------------------------- SendCtrlData ---- *\

	Send data to the TCP CTRL port
	
\* ----------------------------------------------------------------------- */

int SendCtrlData(TPEngine *engp)
{
	OSStatus 		err;
	long			bytesSent;
	char			*datap;
	int				len;
	TPOTInfo		*top;
	TPSockInfo		*tsip;
	
	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0 || top->ctrlInfo.ep == 0) return -1;	// Never allocated
	tsip = &top->ctrlInfo;
	if (top->sendCDCnt <= 0) return noErr;				// Nothing to send
	if (tsip->connState != CONN_STATE_CONNECTED) return noErr;		// Not connected
	
	datap = top->sendCDBuf + top->sendCDPos;
	len = OTStrLength(datap);
	bytesSent = OTSnd(tsip->ep, (void *) datap, len, 0);

	err = noErr;
	if (bytesSent > 0) {
		top->sendCDCnt -= bytesSent;				// We managed to send some data
		if (top->sendCDCnt == 0) {					// In fact, all of it
			top->sendCDPos = 0;
		} else {
			top->sendCDPos += bytesSent;
		}
	} else if (err == kOTFlowErr) {					// Just temporarily filled
		// do nothing
	} else {										// Real error
		err = bytesSent;
	}
	return err;
}

/* ------------------------------------------------------ SendTCPData ---- *\

	Send data to the TCP DATA port
	
\* ----------------------------------------------------------------------- */

int SendTCPData(
	TPEngine	*engp,
	char		*buf,
	int			len,
	int			sockInx)
{
	int			res;
	OTResult 	lookResult;
	TPOTInfo	*top;
	TPSockInfo	*tsip;
	
	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return 0;				// Never allocated

	if (sockInx == TP_SOCKINX_CTRL) tsip = &top->ctrlInfo;
	else tsip = &top->dataInfo;

	if (tsip->connState != CONN_STATE_CONNECTED) return -1;
	
	res = OTSnd(tsip->ep, buf, len, 0);
	if (res < 0) {
		if (res == kOTFlowErr) {
			res = 0;					// Try again later			
		} else if (res == kOTLookErr) {
			lookResult = OTLook(tsip->ep);
			if (lookResult == T_DISCONNECT) {
				TrcLog(1, "SendTCPData: Disconnected");
				tsip->connState = CONN_STATE_DISCONNECTED;
				res = -1;
			} else {
				TrcLog(1, "SendTCPData: Send failed - event = %X", lookResult);
			}
		}
		if (res) TrcLog(1,"OTSnd %d bytes failed with code %d", len, res);
	}
	SetConnCompleteFlags(engp);
	return res;
}

/* ---------------------------------------------------- QueueCtrlData ---- *\

	Put data in send buffer for later output to CTRL port

\* ----------------------------------------------------------------------- */

int QueueCtrlData(TPEngine *engp, char *data)
{
	OSStatus 		err;
	int				len;
	TPOTInfo		*top;
	char			*cp;
	
	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return -1;						// Never allocated

	strcpy(top->sendCDBuf + top->sendCDCnt, data);	// Just add to buffer
	len = strlen(data);
	top->sendCDCnt += len;
	
	err = SendCtrlData(engp);						// Try to send it

	// Debug printout
	
	cp = strchr(data, 13);
	if (cp) *cp = 0;		// Don't inlcude CR in log
	TrcLog(1, "QueueCtrlData: '%s'", data);
	if (cp) *cp = 13;		// Restore CR

	return err;
}

/* -------------------------------------------------------- DoUDPSend ---- *\

	Try sending UDP packet to destination IP/port
	
\* ----------------------------------------------------------------------- */

static OSStatus DoUDPSend(EndpointRef ep, struct InetAddress *sinp, char *datap, int length)
{
	OSStatus	err = kOTNoError;
	TUnitData	unitdata;

	if (ep == kOTInvalidEndpointRef) return noErr;
	if (datap == 0) return noErr;
	
	unitdata.addr.len = sizeof(struct InetAddress);
	unitdata.addr.buf = (UInt8*) sinp;
	unitdata.opt.len = 0;
	unitdata.opt.buf = 0;
	unitdata.udata.len = length;
	unitdata.udata.buf = (UInt8*) datap;
	
	err = OTSndUData(ep, &unitdata);
	return err;
}

/* ---------------------------------------------------- SendNATPacket ---- *\

	Create and send a NAT open packet

	We send a packet from our recv socket to the server's send socket
	in order to open up NAT gateways.

\* ----------------------------------------------------------------------- */

int SendNATPacket(TPEngine *engp)
{
	TPOTInfo			*top		= 0;
	TPSockInfo			*tsip;
	struct tpPacket		*tPnt;
	struct timeval		now;
	int					res;
  	char				buf[100];

	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return -1;					// Nothing allocated

	tsip = &top->udpRcvInfo;	
	if (tsip->ep == 0) return -1;				// No endpoint

	// Set up NATOPEN packet contents
	
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

	OTInitInetAddress(&top->udpSrvSndSin, engp->hostUDPSendPort, engp->hostIP.s_addr);
	res = DoUDPSend(tsip->ep, &top->udpSrvSndSin, (char *) tPnt, 100);
	return 0;
}

/* ---------------------------------------------------- SendUDPPacket ---- *\

	Create and send next UDP packet

\* ----------------------------------------------------------------------- */

int SendUDPPacket(TPEngine *engp, int length)
{
	TPOTInfo		*top		= 0;
	TPSockInfo		*tsip;
	struct tpPacket	*tPnt;
	int				sendLength;
	int				res;
	
	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return -1;				// nothing allocated
	
	tsip = &top->udpSendInfo;
	if (tsip->ep == 0) return -1;			// No endpoint opened

	tPnt = (struct tpPacket *) engp->packetBuf;
	if (tPnt == 0) return -1;				// No packet buffer set
	
	// Set up packet header and random payload
	
	sendLength = length;
		
	// Send the data to the socket
		
	OTInitInetAddress(&top->udpSrvRcvSin, engp->hostUDPRecvPort, engp->hostIP.s_addr);
	res = DoUDPSend(tsip->ep, &top->udpSrvRcvSin, (char *) tPnt, sendLength);

	if (res == noErr) return length;
	return 0;
}

typedef union LBVariant {
	InetHost			addr;
	UInt8				bytes[4];
} LBVariant;

/* --------------------------------------------------- InitNameLookup ---- *\

	in: hostName
	
	out: numHostIp + hostIPTab

	typedef UInt32					InetHost;
	struct InetHostInfo {
		InetDomainName      name;
		InetHost            addrs[10];
	};
	typedef struct InetHostInfo		InetHostInfo;
	
\* ----------------------------------------------------------------------- */

int InitNameLookup(TPEngine *engp)
{
	int				retCode		= -1;
	TPOTInfo		*top		= 0;
	int				i;
	EndpointRef		ep;
	LBVariant		tmpaddr;
	
	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) goto fail;
	if (top->iSvc == 0) goto fail;
	
	OpenTcpEndpoints(engp);
		
	ep = top->ctrlInfo.ep;
	if (ep == 0) goto fail;

	// First convert host to address and save for later use
	
	TrcLog(1, "Start looking for %s...", engp->hostName);
	retCode = OTInetStringToAddress (top->iSvc, engp->hostName, &top->tpHostInfo);
	
	engp->numHostIP = 0;
	if (retCode == noErr) {
		for (i = 0 ; i < MAX_LOOKUP_IP ; i++) {
			if (top->tpHostInfo.addrs[i] == 0) break;
			engp->hostIPTab[i].s_addr = top->tpHostInfo.addrs[i];
			tmpaddr.addr = top->tpHostInfo.addrs[i];
			TrcLog(1, "Address = %d.%d.%d.%d",
		 		tmpaddr.bytes[0], tmpaddr.bytes[1], tmpaddr.bytes[2], tmpaddr.bytes[3]);
			engp->numHostIP += 1;
		}
	}

	TrcLog(1, "Done looking for %s. Result = %d. numHostIP = %d", engp->hostName, retCode, engp->numHostIP);

fail:
	TrcClose();
	return retCode;
}

/* ----------------------------------------------- ContinueNameLookup ---- *\


\* ----------------------------------------------------------------------- */

int ContinueNameLookup(TPEngine *engp)
{
	SetSMState(engp, CLSM_COMPLETE);
	return 0;
}

/* ------------------------------------------------- OpenTcpEndpoints ---- *\


\* ----------------------------------------------------------------------- */

static int OpenTcpEndpoints(TPEngine *engp)
{
	int				retCode		= -1;
	TPOTInfo		*top		= 0;
	OSStatus 		err;
	TPSockInfo		*tsip;
	EndpointRef		ep;

	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) goto fail;

	// Set up a notification handler to use for callbacks
	
	if (myNotifyUPP == 0) myNotifyUPP = NewOTNotifyUPP(YieldingNotifier);
	
	// Open a TCP endpoint for ctrl flow
	tsip = &top->ctrlInfo;
	if (tsip->ep == 0) {
		ep = OTOpenEndpointInContext(OTCreateConfiguration(kTCPName), 0, nil, &err, nil);
		
		if (err != noErr) {
			TrcLog(1, "OpenTcpEndpoints: OTOpenEndpointInContext failed with code %d", err);
			goto fail;
		}

		// Establish the modes of operation.

		tsip->ep = ep;
		tsip->context.sockInfo = tsip;
		strcpy(tsip->id, "TCP-ctrl");
		
		err = OTSetSynchronous(ep);
		err = OTSetBlocking(ep);
		err = OTInstallNotifier(ep, myNotifyUPP, &tsip->context);		
		err = OTUseSyncIdleEvents(ep, true);
		TrcLog(1, "OpenTcpEndpoints: Ctrl EP = %08X", ep);
	}

	// Open a TCP endpoint for data flow
		
	tsip = &top->dataInfo;
	if (tsip->ep == 0) {
		ep = OTOpenEndpointInContext(OTCreateConfiguration(kTCPName), 0, nil, &err, nil);
		
		if (err != noErr) {
			TrcLog(1, "OpenTcpEndpoints: OTOpenEndpointInContext failed with code %d", err);
			goto fail;
		}

		// Establish the modes of operation.

		tsip->ep = ep;
		tsip->context.sockInfo = tsip;
		strcpy(tsip->id, "TCP-data");

		err = OTSetSynchronous(ep);
		err = OTSetBlocking(ep);
		err = OTInstallNotifier(ep, myNotifyUPP, &tsip->context);		
		err = OTUseSyncIdleEvents(ep, true);
		TrcLog(1, "OpenTcpEndpoints: Data EP = %08X", ep);
	}
	retCode = 0;
	
fail:
	return retCode;
}

/* --------------------------------------------------- InitConnectTCP ---- *\

	Start a TCP connect
	
\* ----------------------------------------------------------------------- */

int InitConnectTCP(TPEngine *engp, int sockInx)
{
	int				retCode		= -1;
	TPOTInfo		*top		= 0;
	TPSockInfo		*tsip;
	OSStatus 		err;
	EndpointRef		ep;
	int				thePort;
	TCall 			ctrlCall;
	TBind			ret;
	struct InetAddress	tmpSin;
	
	if (engp->hostIP.s_addr == 0) goto done;	// No target address set
	
	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) goto done;					// Not setup correctly
	if (top->iSvc == 0) goto done;				// Not setup correctly
	
	OpenTcpEndpoints(engp);
	
	if (sockInx == TP_SOCKINX_CTRL) {
		tsip = &top->ctrlInfo;					// Connecting ctrl port
		thePort = engp->hostCtrlPort;
	} else {
		tsip = &top->dataInfo;					// Connecting data port
		thePort = engp->hostTCPDataPort;
	}
				
	if (tsip->connState != CONN_STATE_DISCONNECTED) {	// Already connected - keep using it
		TrcLog(1,"InitConnectTCP: Port already connected");
		retCode = 0;
		goto done;
	}
	ep = tsip->ep;
	
	if (ep == 0) goto done;

	tsip->connEventSeen = 0;

	// Bind the endpoint.  Because we're an outgoing connection,
	// we don't have to bind it to a specific address.
			
	ret.addr.maxlen = sizeof(struct InetAddress);
	ret.addr.buf = (UInt8*) &tmpSin;
	err = OTBind(ep, nil, &ret);
	if (err != noErr) {
		TrcLog(1,"InitConnectTCP: OTBind failed - %d", err);
		goto done;
	}
	tsip->epBound = 1;
	tsip->dataSeen = 0;
	tsip->gotRcvDisconnect = 0;
	tsip->gotRcvOrdRel = 0;
	if (sockInx == TP_SOCKINX_CTRL) {
		top->sendCDCnt = 0;					// No unsent ctrl data
		top->sendCDPos = 0;
		top->xferCharCnt = 0;
		top->xferCharPos = 0;
		top->replyCnt = 0;
	}
	
	// Set up the address part of the control port
	
	OTInitInetAddress(&top->ctrlSin, thePort, engp->hostIP.s_addr);

	ctrlCall.addr.buf 	= (UInt8 *) &top->ctrlSin;
	ctrlCall.addr.len 	= sizeof(struct InetAddress);
	ctrlCall.opt.buf 	= nil;		// no connection options
	ctrlCall.opt.len 	= 0;
	ctrlCall.udata.buf 	= nil;		// no connection data
	ctrlCall.udata.len 	= 0;
	ctrlCall.sequence 	= 0;		// ignored by OTConnect

	err = OTSetAsynchronous(ep);
	err = OTSetBlocking(ep);
	err = OTConnect(ep, &ctrlCall, nil);
	
	if (err == noErr) {
		TrcLog(1, "OTConnect completed", err);
		tsip->connState = CONN_STATE_CONNECTED;
		
	} else if (err == kOTNoDataErr) {
		TrcLog(1, "OTConnect started on %08X for %08X port %d", ep, engp->hostIP.s_addr, thePort);
		tsip->connState = CONN_STATE_CONNECTING;
		
	} else {
		TrcLog(1, "ERROR: OTConnect failed - %d", err);
		tsip->connState = CONN_STATE_DISCONNECTED;
		goto done;
	}
	err = OTSetAsynchronous(ep);

	retCode = 0;

done:

	TrcClose();
	SetConnCompleteFlags(engp);
	return retCode;
}

/* ----------------------------------------------- ContinueConnectTCP ---- *\

	Continue nonblocking TCP connect
	
\* ----------------------------------------------------------------------- */

int ContinueConnectTCP(TPEngine *engp)
{
	TPOTInfo		*top		= 0;
	TPSockInfo		*tsip;
	int				x;
	int				retCode		= -1;
	OSStatus 		err;
	OTResult 		lookResult;

	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return 0;
	
	for (x = 1 ; x <= 2 ; x++) {
		if (x == 1) tsip = &top->ctrlInfo;
		else tsip = &top->dataInfo;
		if (tsip->connState != CONN_STATE_CONNECTING) continue;
		
		if (tsip->gotRcvDisconnect) {
			err = OTRcvConnect(tsip->ep, 0);
			if (err == kOTLookErr) {
				lookResult = OTLook(tsip->ep);
				if (lookResult == T_DISCONNECT) {
					TrcLog(1, "ContinueConnectTCP: Connection refused");
				} else {
					TrcLog(1, "ContinueConnectTCP: Connect failed - event = %X", lookResult);
				}
			} else {
				TrcLog(1, "ContinueConnectTCP: Connect failed - %d", err);
			}
			tsip->connState = CONN_STATE_DISCONNECTED;
			goto done;
		}
		if (tsip->connEventSeen) {
			err = OTRcvConnect(tsip->ep, 0);
			if (err == noErr) {
				tsip->connState = CONN_STATE_CONNECTED;
				TrcLog(1, "ContinueConnectTCP: Connect Completed on %08X", tsip->ep);
			} else {
				TrcLog(1, "ContinueConnectTCP: Connect failed on %08X - %d", tsip->ep, err);
				goto done;
			}
		}
	}
	retCode = 0;

done:
	SetConnCompleteFlags(engp);
	return retCode; 
}

/* ----------------------------------------------- ConsumeTCPTestData ---- *\

	Consume incoming TCP test data
	
\* ----------------------------------------------------------------------- */

int		ConsumeTCPTestData(TPEngine *engp)
{
	TPSockInfo		*tsip;
	TPOTInfo		*top;
	int				cnt;
	OSStatus 		err;
	OTFlags 		flags;
	struct timeval	nowTV;
	
	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return -1;				// Never allocated
	
	tsip = &top->dataInfo;

	// Handle any detected Disconnect events if no data seen
	
	if (tsip->dataSeen == 0) {		
		if (tsip->gotRcvDisconnect != 0) {
			err = OTRcvDisconnect(tsip->ep, nil);
			TrcLog(1,"OTRcvDisconnect gave %d", err);
			tsip->gotRcvDisconnect = 0;
			tsip->connState = CONN_STATE_DISCONNECTED;
		}
					
		if (tsip->gotRcvOrdRel != 0) {
			err = OTRcvOrderlyDisconnect(tsip->ep);
			TrcLog(1,"OTRcvOrderlyDisconnect gave %d", err);
			if (err == noErr) {
				err = OTSndOrderlyDisconnect(tsip->ep);
			}
			tsip->gotRcvOrdRel = 0;
			tsip->connState = CONN_STATE_DISCONNECTED;
		}
	}
	
	// Process (just discard) any received data bytes
	
	if (tsip->dataSeen) {		
		tsip->dataSeen = 0;
		
		cnt = OTRcv(tsip->ep, (void *) top->junkBuf, kTransferBufferSize - 1, &flags);
		if (cnt > 0) tsip->dataSeen = 1;	// Come back until we get kOTNoDataErr
		
		if (cnt >= 0) {
			gettimeofday(&nowTV, NULL);
			engp->packetLen = cnt;
			if (engp->stats.BytesRecvd == 0) engp->stats.StartRecv = nowTV;
			engp->stats.BytesRecvd += cnt;
			engp->stats.StopRecv = nowTV;
			if (0) TrcLog(1, "ConsumeTCPTestData: Read %d bytes to %08X", cnt, top->junkBuf);
			return cnt;			
		} else {
			if (cnt != kOTNoDataErr) {
				TrcLog(1,"ERROR: Rcv() failed with %d", cnt);
				return -1;
			}	
			return 0;		
		}
	}

	return -1;
}

/* -------------------------------------------------- ConsumeCtrlData ---- *\

	Consume incoming TCP control data

	Data is received into top->xferBuf and then assembled into a
	complete message in engp->ctrlMessage.
	
\* ----------------------------------------------------------------------- */

int		ConsumeCtrlData(TPEngine *engp)
{
	TPOTInfo		*top;
	TPSockInfo		*tsip;
	int				cnt;
	OSStatus 		err;
	OTFlags 		flags;
	int				c;
	
	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return 0;				// Never allocated

	tsip = &top->ctrlInfo;
	
	if (top->xferCharCnt == 0) {
		if (tsip->gotRcvDisconnect != 0) {
			// If we get a T_DISCONNECT event, the remote peer
			// has disconnected the stream in a dis-orderly
			// fashion.  HTTP servers will often just disconnect
			// a connection like this to indicate the end of the
			// data, so all we need do is clear the T_DISCONNECT
			// event on the endpoint.

			err = OTRcvDisconnect(tsip->ep, nil);
			TrcLog(1,"OTRcvDisconnect gave %d", err);
			tsip->gotRcvDisconnect = 0;
			tsip->connState = CONN_STATE_DISCONNECTED;
		}
					
		if (tsip->gotRcvOrdRel != 0) {
			// If we get a T_ORDREL event, the remote peer
			// has disconnected the stream in an orderly
			// fashion.  This orderly disconnect indicates that
			// the end of the data.  We respond by clearing
			// the T_ORDREL, and then calling OTSndOrderlyDisconnect
			// to acknowledge the orderly disconnect at
			// the remote peer.
			
			err = OTRcvOrderlyDisconnect(tsip->ep);
			TrcLog(1,"OTRcvOrderlyDisconnect gave %d", err);
			if (err == noErr) {
				err = OTSndOrderlyDisconnect(tsip->ep);
			}
			tsip->gotRcvOrdRel = 0;
			tsip->connState = CONN_STATE_DISCONNECTED;
		}
	}
		
	if (engp->ctrlMessageComplete != 0) goto done;	// Old data still unprocessed

	while (top->xferCharCnt > 0) {					// We have some data in xfer buffer
		c = top->xferBuf[top->xferCharPos] & 255;
		top->xferCharPos += 1;
		top->xferCharCnt -= 1;
		
		if (c == '\012') {
			engp->ctrlMessage[top->replyCnt] = 0;
			engp->ctrlMessageComplete = 1;
			TrcLog(1, "Ctrl data: '%s'", engp->ctrlMessage);
			break;

		} else if (c == '\015') {
			// Just ignore CR
			
		} else {
			if (top->replyCnt < sizeof(engp->ctrlMessage) - 1) {
				engp->ctrlMessage[top->replyCnt++] = c;
			}
		}
	}
	
	if (engp->ctrlMessageComplete != 0) goto done;		// Old data still unprocessed

	if (tsip->dataSeen) {
		
		// Now read any real information into xfer buffer
		
		tsip->dataSeen = 0;
		cnt = OTRcv(tsip->ep, (void *) top->xferBuf, kTransferBufferSize - 1, &flags);
		if (cnt > 0) tsip->dataSeen = 1;
		
		if (cnt >= 0) {
			top->xferCharCnt = cnt;
			top->xferCharPos = 0;
			
		} else {
			if (cnt != kOTNoDataErr) {
				TrcLog(1,"ERROR: Rcv() failed with %d", cnt);
			}			
		}
	}
	
done:
	return 0;							// Dummmy
}

/* --------------------------------------------------- ConsumeUDPData ---- *\


\* ----------------------------------------------------------------------- */

int ConsumeUDPData(TPEngine *engp)
{
	TPOTInfo		*top;
	TPSockInfo		*tsip;
	OTFlags 		flags;
	TUnitData		unitdata;
	int				err;
	int				len;
	int				rTrip;
	struct InetAddress	tmpSin;
	struct tpPacket		*tPnt;
	
	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return 0;				// Never allocated

	tsip = &top->udpRcvInfo;
	if (tsip->dataSeen == 0) return 0;	// Nothing to consume
	
	tsip->dataSeen = 0;
	while (1) {
		unitdata.addr.maxlen = sizeof(struct InetAddress);
		unitdata.opt.maxlen = 0;
		unitdata.opt.buf = 0;
		unitdata.udata.maxlen = engp->packBufSize - 1;
		unitdata.udata.buf = (unsigned char *) engp->packetBuf;
		unitdata.addr.buf = (UInt8*) &tmpSin;
	
		err = OTRcvUData(tsip->ep, &unitdata, &flags);
		if (err == kOTNoDataErr) break;
		
		gettimeofday(&engp->stats.StopRecv, NULL);
		if (engp->stats.BytesRecvd == 0) engp->stats.StartRecv = engp->stats.StopRecv;

		len = unitdata.udata.len;
		if (0) TrcLog(1,"RcvUData gave %d, len = %d", err, len);
		if (0) TrcLog(1,"RcvUData from %X:%d", tmpSin.fHost, tmpSin.fPort);
		if (engp->natOpen) {
			engp->stats.PktsRecvd += 1;
	 		engp->stats.BytesRecvd += ( len + IP_UDP_SIZE );
	 		tPnt = (struct tpPacket *) engp->packetBuf;
	 		if (engp->tpMode == M_UDP_FDX) {
	   			rTrip = ( engp->stats.StopRecv.tv_sec - ntohl( tPnt->Header.ClientSendTime.tv_sec ) ) * 1000000;
	   			rTrip += ( engp->stats.StopRecv.tv_usec - ntohl( tPnt->Header.ClientSendTime.tv_usec ) );
				if ( rTrip > engp->stats.MaxRoundtrip ) engp->stats.MaxRoundtrip = rTrip;
	    		else if ( rTrip < engp->stats.MinRoundtrip ) engp->stats.MinRoundtrip = rTrip;

			    engp->stats.TotalRoundtrip += rTrip;
				engp->stats.nRoundtrips++;
	 		}
		} else {
			engp->natOpen = 1;
		}
	}
	return 0;
}

/* ---------------------------------------------------- CheckCtrlOpen ---- *\


\* ----------------------------------------------------------------------- */

int CheckCtrlOpen(TPEngine *engp)
{
	TPOTInfo		*top;
	
	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return 0;				// Never allocated

	if (top->ctrlInfo.connState == CONN_STATE_CONNECTED) return 1;
	return 0;
}

/* -------------------------------------------------- SendStatMessage ---- *\

	Currently not used. But this is where you insert the code
	when the need arises. Will do things if DO_SEND_STAT_UDP is nonzero.
	
\* ----------------------------------------------------------------------- */

void SendStatMessage(TPEngine *engp)
{
	TPOTInfo			*top		= 0;
	TPSockInfo			*tsip;
	int					res;
  	char				buf[100];

	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return;					// Nothing allocated
	if (DO_SEND_STAT_UDP == 0) return;		// Not configured to do this
	
	tsip = &top->udpSendInfo;
	if (tsip->ep == 0) {
		InitUDP(engp);
	}
	if (tsip->ep == 0) return;				// No endpoint
	
	sprintf(buf, "client=%s;test=%d;tcpsr=%d;tcprr=%d", engp->clientInfo, engp->tpMode,
		(int)(engp->bestTCPSendRate * 8.0), (int)(engp->bestTCPRecvRate * 8.0));

	OTInitInetAddress(&top->udpSrvSndSin, STAT_UDP_PORT, STAT_UDP_SERVER);
	res = DoUDPSend(tsip->ep, &top->udpSrvSndSin, buf, strlen(buf) + 1);

	return;
}

/* ------------------------------------------------- CloseOneTPSocket ---- *\


\* ----------------------------------------------------------------------- */

static void CloseOneTPSocket(
	TPSockInfo	*tsip)
{
	if (tsip->epBound) {					// Unbind the endpoint
		OTUnbind(tsip->ep);
		tsip->epBound = 0;
	}
	if (tsip->ep != 0) {					// And close it
		OTCloseProvider(tsip->ep);
		tsip->ep = 0;
	}
	tsip->connState = CONN_STATE_DISCONNECTED;
}

/* -------------------------------------------------- CloseAllSockets ---- *\


\* ----------------------------------------------------------------------- */

void CloseAllSockets(TPEngine *engp)
{
	TPOTInfo		*top;
	
	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return;				// Never allocated

	CloseOneTPSocket(&top->ctrlInfo);
	CloseOneTPSocket(&top->dataInfo);
	CloseOneTPSocket(&top->udpSendInfo);
	CloseOneTPSocket(&top->udpRcvInfo);
	TrcLog(1,"CloseAllSockets: All sockets closed");
	return;
}

/* --------------------------------------------------- ClearCtrlReply ---- *\


\* ----------------------------------------------------------------------- */

void	ClearCtrlReply(TPEngine *engp)
{
	TPOTInfo		*top;

	top = (TPOTInfo *) engp->ctrlRefp;
	if (top == 0) return;				// Never allocated

	engp->ctrlMessageComplete = 0;		// No reply seen
	top->replyCnt = 0;					// Nothing in line buffer
}

