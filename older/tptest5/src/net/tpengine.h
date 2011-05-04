/*
 * $Id: tpengine.h,v 1.1 2007/01/31 07:45:40 danron Exp $
 * $Source: /cvsroot-fuse/tptest/tptest5/src/net/tpengine.h,v $
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tpengine.h - main test engine header file
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

#ifndef _TPENGINE_H_
#define _TPENGINE_H_

#define MAJORVERSION	3
#define MINORVERSION	18


// Select platform
// #define __WIN32
// #define MACOS

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Win32 specifics

#ifdef WIN32
	#define LONG_LONG_PREFIX	"I64"
	#define NO_GETTIMEOFDAY
	typedef _int64			LONG_LONG;
	typedef unsigned short	USHORT;
	#define dprintf			printf
	#include <winsock2.h>
#endif


// Unix specifics

#ifdef UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef long long               LONG_LONG;
typedef unsigned short          USHORT;
typedef long                    INT32;
typedef unsigned long           UINT32;

typedef struct sockaddr         SOCKADDR;
typedef struct sockaddr_in      SOCKADDR_IN;
typedef int                     SOCKET;

#define SOCKET_ERROR            (-1)
#define INVALID_SOCKET          (-1)

// Solaris specifics

#ifdef SOLARIS
#define LONG_LONG_PREFIX        "ll"
#define INADDR_NONE		(-1)
#endif	// SOLARIS

// Linux specifics

#ifdef LINUX
#define LONG_LONG_PREFIX        "ll"
#endif	// LINUX

// OpenBSD specifics

#ifdef OPENBSD
#define LONG_LONG_PREFIX	"q"
#endif

#ifdef MACOSX
#define LONG_LONG_PREFIX	"ll"
#endif

#endif	// UNIX


// MAC specifics

#ifdef MACOS

struct timeval {
	int		tv_sec;
	int		tv_usec;
};
struct in_addr {
	u_int32_t s_addr;
};
#define LONG_LONG_PREFIX	"ll"
#Define NO_GETTIMEOFDAY
#define NO_HTONL
#define NO_NTOHL
typedef long long		LONG_LONG;
typedef unsigned short	USHORT;
typedef long			INT32;
typedef unsigned long	UINT32;

long	htonl(long l);
long	ntohl(long l);
void	Report(char *str);
void	ClearTextWindow(void);
#endif  // MACOS

typedef struct in_addr IN_ADDR;

#define TP_DEBUGLEVEL		0
//#define TP_DEBUGLEVEL		3


#define DEFAULT_CONTROL_PORT	1634

#define MAXINT		(2147483647)


/*
* Test modes.
*/

#define M_NONE			0
#define M_UDP_FDX		1
#define M_UDP_SEND		2
#define M_UDP_RECV		3
#define M_TCP_SEND		4
#define M_TCP_RECV		5
#define M_QUERY_MASTER	6
#define M_NAME_LOOKUP	7


/*
* Client engine states. 
*
*/
enum TPCLIENTSM_STATE {
	CLSM_IDLE		= 1,
	CLSM_CONNECTING,
	CLSM_CONNECTED,
	CLSM_WAITPORTS,
	CLSM_NATOPEN,
	CLSM_TESTLOOP,
	CLSM_SENDSTAT,
	CLSM_WAITSTAT,
	CLSM_DELAYQUIT,
	CLSM_TERM_WAIT,
	CLSM_FAILED,
	CLSM_SENDMHELO,
	CLSM_SERVERLIST,
	CLSM_COMPLETE,
	CLSM_DATACONNECTING,
	CLSM_NAMELOOKUP,
	CLSM_WAITFDX
};


/*
* Server engine states 
*
*/
enum TPSERVERSM_STATE {
	SSM_IDLE	= 101,
	SSM_LISTEN,
	SSM_SENDWELCOME,
	SSM_WAITTEST,
	SSM_POSTTEST,
	SSM_WAITNAT,
	SSM_WAITTCPDATA,
	SSM_TESTLOOP,
	SSM_SENDSTAT,
	SSM_WAITSTAT,
	SSM_DELAYQUIT,
	SSM_FDXWAIT,
	SSM_COMPLETE,
	SSM_FAILED,
	SSM_DATALISTEN,
	SSM_INITTCPDATA
};


/*
* Socket identifiers
*
*/
enum TP_SOCKINX {
	TP_SOCKINX_CTRL = 1,
	TP_SOCKINX_DATA,
	TP_SOCKINX_SCTRL,
	TP_SOCKINX_SDATA
};


/*
* CallMeAgain-constants. Not so useful yet
*
*/
#define CMA_CLIWAITPORTS	100000
#define CMA_CLIUDPSEND		20000
#define CMA_CLISENDSTAT		100000
#define CMA_CLIWAITSTAT		100000
#define CMA_CLIWAITPORTS	100000
#define CMA_CLINATOPEN		50000
#define CMA_CLIUDPRECV		20000
#define CMA_CLIUDPFDX		20000
#define CMA_CLIDATACONNECT	100000
#define CMA_CLITCPSEND		50000
#define CMA_CLITCPRECV		50000

#define CMA_SRVWAITNAT		50000
#define CMA_SRVUDPSEND		20000
#define CMA_SRVSENDSTAT		100000
#define CMA_SRVWAITSTAT		100000
#define CMA_SRVUDPRECV		20000
#define CMA_SRVUDPFDX		20000
#define CMA_SRVDATALISTEN	100000
#define CMA_SRVTCPTEST		50000


/*
* Engine delays between states
*
*/
#define USEC_STATDELAY		500000
#define USEC_NATOPEN		1000000
#define USEC_DELAYQUIT		500000


/*
* Size of IP+UDP header
*
*/
#define IP_UDP_SIZE 28

/*
* Size of various data structures/buffers/arrays
*
*/
#define MAX_LOOKUP_IP		10
#define MAX_SERVERS		30
#define MAX_SERVER_NAME		40
#define MAX_SERVER_INFO		40
#define TP_CTRL_MSG_SIZE	200
#define TP_HOST_NAME_SIZE	200
#define PACKBUFSIZE		66000
#define RANDBUFSIZE		(524288)
#define REPLYBUFSIZE		512


/* 
* Other constants
*/
#define LISTEN_BACKLOG		5
#define TP_1KBPS		1000
#define TP_1MBPS		1000000
#define START_TCP_BYTES		51200

/*
* Error codes
*
*/
#define TPER_CTRLCLOSED		2001
#define TPER_TIMEOUT		2002
#define TPER_NOCTRL			2003
#define TPER_BADHELLO		2004
#define TPER_BADPORTS		2005
#define TPER_SRVABORT		2006
#define TPER_BADMODE		2007
#define TPER_NATFAIL		2008
#define TPER_UDPOPENFAIL	2009
#define TPER_USERABORT		2010
#define TPER_MASTERBUSY		2011
#define TPER_BADMASTERREPLY 2012
#define TPER_MASTERDENIED	2013
#define TPER_BADCOOKIE		2014
#define TPER_BADNATACK		2015
#define TPER_NOTCPDATASOCK	2016
#define TPER_NODATA			2017
#define TPER_MAXSERVERS		2018
#define TPER_NOSERVNAME		2019
#define TPER_UNSUPPROTO		2020
#define TPER_NOHOSTNAME		2021
#define TPER_CONNECTFAIL	2022
#define TPER_BADWELCOME		2023
#define TPER_WRONGCOOKIE	2024
#define TPER_NOCOOKIE		2025
#define TPER_WRONGMODE		2026
#define TPER_NOMODE			2027
#define TPER_NOTIME			2028
#define TPER_NONPACKETS		2029
#define TPER_NOPSIZE		2030
#define TPER_NOUDPSENDPORT	2031
#define TPER_NOUDPRECVPORT	2032
#define TPER_NOTIMEOUT		2033
#define TPER_NOTCPBYTES		2034
#define TPER_SERVERBUSY		2035
#define TPER_SERVERDENY		2036
#define TPER_NLINITFAIL		2037
#define TPER_NLFAIL			2038
#define TPER_DATACLOSED		2039

#define TPER_ACCEPTFAIL		3001
#define TPER_BADTEST		3002
#define TPER_CLIABORT		3003
#define TPER_STATFAIL		3004


/* Boolean flags */
#define TPENGINE_FLAGS_NOINITRATE	1


/*
* Data packet used in tests.
*/
struct tpHeader { 
	unsigned int	Sequence;
	struct timeval	ClientSendTime;
	struct timeval	ServerRecvTime;
	struct timeval	ServerSendTime;
	UINT32		DataSize;
        UINT32		Cookie;
};

typedef struct tpPacket { 
	struct tpHeader	Header;
	unsigned char	Data[ 1 ];
} TPPacket;


#define MIN_PKT_SIZE	((sizeof(struct tpHeader))+(IP_UDP_SIZE))

/*
 * Test results.
 */
typedef struct TPStats { 
	USHORT		MajorVersion;
	USHORT		MinorVersion;
	UINT32		PktsSent;
	UINT32		PktsUnSent;
	UINT32		PktsRecvd;
	LONG_LONG	BytesSent;
	LONG_LONG	BytesRecvd;
	UINT32		nRoundtrips;
	UINT32		TotalRoundtrip;
	UINT32		MaxRoundtrip;
	UINT32		MinRoundtrip;
	UINT32		ooCount;
	struct timeval	StartSend;
	struct timeval	StopSend;
	struct timeval	StartRecv;
	struct timeval	StopRecv;
	char email[101];
	char pwd[101];
} TPStats;



/*
* TPEngine is the general info-command-struct used to communicate
* with the test engine.
*/
typedef struct TPEngine {

	/*
	 * Variables supplied by user in order to start a test
	 */

	/* Needed for all modes */
	INT32			tpMode;							// Test mode
	IN_ADDR			hostIP;							// Host address
	USHORT			hostCtrlPort;					// Server TCP control port number
	UINT32			sessionMaxTime;					// max test time (before timeout)


	/* Needed when tpMode is M_TCP_SEND or M_TCP_RECV */
	UINT32			tcpBytes;						// Number of bytes to transfer in TCP test

	/* Needed when tpMode is M_UDP_SEND, M_UDP_RECV or M_UDP_FDX */
	UINT32			packetSize;						// Packet size
	UINT32			nPackets;						// Number of packets to send / receive
	UINT32			sessionTime;					// Total send time


	/*
	 * Variables supplied by user to perform other actions
	 */

	/* bitsPerSecond and packetsPerSecond can be used to calculate packetSize, nPackets and 
	   packetInterval. Just set the variables you know and zero the others, then call 
	   RecalculatePPSSZ() */
	UINT32			bitsPerSecond;					// RecalculatePPSSZ
	UINT32			packetsPerSecond;				// RecalculatePPSSZ

	/* HostName allows the engine to do an asynchronous host name lookup for the user */
	char		hostName[TP_HOST_NAME_SIZE+2];	// Servername (string) for hostname lookup
	int			numHostIP;						// Number of IP addresses for the host
	IN_ADDR		hostIPTab[MAX_LOOKUP_IP];		// Array with IP addresses for hostname

	/* Server list */
	/* Set hostIP to the IP of the master server you want to use */
	char		serverNameList[MAX_SERVERS][MAX_SERVER_NAME];
	char		serverInfoList[MAX_SERVERS][MAX_SERVER_INFO];
	USHORT		serverPortList[MAX_SERVERS];
	int			numServers;

        /* Requested socket options */
        /* The application sets these values and the IO module tries */
        /* to make sure data sockets use them. If the IO module is not */
        /* able to use these values, it should either fill in the values */
        /* actually used, or zero (0) in the "cur_socket_sndbuf" and */
	/* "cur_socket_rcvbuf" variables */
        int		socket_sndbuf;
	int		socket_rcvbuf;

	int		cur_socket_sndbuf;
	int		cur_socket_rcvbuf;

	/* an application should check cur_socket_sndbuf/cur_socket_rcvbuf */
	/* when a test is running (and data sockets have been set up) to */
	/* make sure its requested socket options have been set */

	/* These variables can be set to try and force the local machine */
	/* to allocate port numbers within a certain range for its data ports */
	USHORT		local_tcp_pr_start;
	USHORT		local_tcp_pr_end;
	USHORT		local_udp_pr_start;
	USHORT		local_udp_pr_end;


	/* Boolean flags */
	/* Defined in this file as TPENGINE_FLAGS_XXX... */
	int		flags;

	/*
	 * Internal variables
	 * These can be read but should NOT be modified from outside the engine
	 */
	int			active;				// Bool. Is the engine running
	int			state;				// Engine state
	time_t		startTime;			// time_t for start by StartClientContext
	UINT32		packetInterval;		// send interval per packet (microseconds)
	INT32		failCode;
	INT32		timeLimit;			// Internal state timer set by SetTimeLimit
	INT32		callMeAgain;		// How many microseconds before RunServerContext() or
									// RunClientContext() wants another call
	
	UINT32		sessCookie;			// set by server

	UINT32		natCount;			// How many NAT-open packets have we sent
	UINT32		curSend;			// Send packet counter
	int			wipeFlag;			// Used internally when querying
	int			natOpen;			// Bool. Is NAT-open finished?


	UINT32		packetsRecvd;		// # of packets received
	UINT32		packetsSent;		// # of packets sent
	UINT32		prevPacket;			// Maximum sequence number seen
	UINT32		badPackets;			// received packets with incorrect cookie
	UINT32		badBytes;			// received bytes in bad packets
	
	char			*packetBuf;			// Packet buffer pointer
	UINT32			packBufSize;		// Size of packet buffer
	IN_ADDR			packetfromAdr;			// Where did incoming packet come from
	USHORT			packetfromPort;		// What port did incoming packet come from
	UINT32			packetLen;				// Length of incoming packet

	char		*randBuf;			// Random buffer for data generation
	UINT32		randBufSize;		// Size of random buffer

	TPStats		stats;			// Struct for storing test results/statistics
	
	struct timeval nextSendTV;		// timeval for next sen
	
	int			(*executor)(struct TPEngine *ctxp);	// Select executor for RunContext
	
	void		*ctrlRefp;			// Local data for I/O handler
	
	int			ctrlMessageComplete;		// Bool. Set when complete command seen
	char		ctrlMessage[REPLYBUFSIZE];	// Latest command read from control port
	
	int			tcpCtrlConnectComplete;			// Bool.
	int			tcpDataConnectComplete;			// Bool.
	int			iAmServer;					// Bool.
	
	INT32		ioError;			// Platform dependent io error code - set by engine
		
	USHORT		hostTCPDataPort;	// Port used by remote host to transmit/receive TCP data
	USHORT		hostUDPRecvPort;		// Set by CheckPortsMessage el vad det nu blir
	USHORT		hostUDPSendPort;

	IN_ADDR		myLocalAddress;		// bind to this local address (optional)
	USHORT		myTCPDataPort;		// bind to this local TCP data port (optional)
	USHORT		myUDPRecvPort;		// bind to this local UDP receive data port (optional)
	USHORT		myUDPSendPort;		// bind to this local UDP send data port (optional)
	USHORT		myTCPControlPort;	// bind to this local TCP control port (optional)
	USHORT		boundTCPDataPort;	// Port bound to
	USHORT		boundTCPControlPort;	// Port bound to
	USHORT		boundUDPRecvPort;	// Port bound to
	USHORT		boundUDPSendPort;	// Port bound to

	USHORT		peerMajorVersion;		// Major/minor version no of peer
	USHORT		peerMinorVersion;

	double		bestTCPSendRate;		// Best rates seen. Bytes/sec. For the client support functions
	double		bestTCPRecvRate;
	double		bestUDPSendRate;
	double		bestUDPRecvRate;

	UINT32		start_tcpsend_bytes;	// starting value for tcpBytes when doing TCP_AUTO_SEND
	UINT32		start_tcprecv_bytes;	// starting value for tcpBytes when doing TCP_AUTO_RECV

	char		clientInfo[80];		// Client info. E.g. "TPtest 3.0.1 MacOS9"
	
	void*		userdata;			// whatever data user need with this test
} TPEngine;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
* Engine function prototypes
*
*/
int AddServerToList(TPEngine *, char *);
int	CheckHelloReply(TPEngine *);
int	CheckServerBanner(TPEngine *);
int	CheckStatsMsg(TPEngine *);
int	CheckTestReply(TPEngine *);
int CheckTestMessage(TPEngine *);
void ClearBestRates(TPEngine *);
void ClearServerList(TPEngine *);
TPEngine *CreateContext(void);			// Called by user
void DeleteContext(TPEngine *);			// Called by user
int DoClientTCPRecv(TPEngine *);
int DoClientTCPSend(TPEngine *);
int DoClientUDPDuplex(TPEngine *);
int DoClientUDPRecv(TPEngine *);
int DoClientUDPSend(TPEngine *);
int DoNameLookup(TPEngine *);
int DoServerTCPSend(TPEngine *);
int DoServerTCPRecv(TPEngine *);
int DoServerUDPDuplex(TPEngine *);
int DoServerUDPRecv(TPEngine *);
int DoServerUDPSend(TPEngine *);
void FailSession(TPEngine *, int);
void GenerateUDPDataPacket(TPEngine *);
int RunClientContext(TPEngine *);		// Called by user
int RunServerContext(TPEngine *);		// Called by user
int SendHeloLine(TPEngine *);
int SendStatLine(TPEngine *);
int SendTCPTestData(TPEngine *);
int SendTestLine(TPEngine *);
int SendTestOKMessage(TPEngine *);
int SendUDPDataPacket(TPEngine *);
void SetSMState(TPEngine *, int);
void SetTimeLimit(TPEngine *, int);		// Called by user
int StartClientContext(TPEngine *);		// Called by user
int StartServerContext(TPEngine *);		// Called by user
void StopContext(TPEngine *);			// Called by user
int TimeLimitExceeded(TPEngine *);
int DoQueryMaster(TPEngine	*);


#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif

