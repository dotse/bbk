/*
 *	
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * macgui_statwin.c - status window routines
 *
 * Written by
 *  Hans Green <hg@3tag.com>
 *  Torsten Alm <totte@3tag.com>
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

#include "tpengine.h"
#include "tpio.h"
#include "tpclient.h"
#include "tpcommon.h"
#include "macgui_statwin.h"

#define MAX_PKT_SIZE	65535	// Max packet size for udp tests
#define MAX_ROWS		40		// Number of rows in status window
#define ROW_SIZE		120		// Max number of chars in each row

static pascal OSStatus StatWindowEventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData);
static void StatWindowUpdater(void);

static EventHandlerUPP	gStatWinEvtHandler;	
static WindowRef		gStatWindow;
static char				*rowTab[MAX_ROWS];
static int				rowsOK;
static int				rowCnt;

/* ------------------------------------------- StatWindowEventHandler ---- *\

	Event handler for status window
	
\* ----------------------------------------------------------------------- */

static pascal OSStatus StatWindowEventHandler(
	EventHandlerCallRef myHandler,
	EventRef	event,
	void		*userData)
{
    OSStatus	result = eventNotHandledErr;
	HICommand  	aCommand;
	
	myHandler, userData;			// Touch unused
	
	switch (GetEventClass(event)) {
		case kEventClassCommand:
			GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL,
					sizeof(HICommand), NULL, &aCommand);
			
			// What Command
			
			switch (aCommand.commandID) {			    	
			    
		    case 'test':
		    	PutIntoClipGlue();
		    	result = noErr;
		    	break;
		            
			default:
				break;
		    }
			break;

		case kEventClassWindow:
			switch (GetEventKind(event)) {
			
		    case kEventWindowClose:
		    		DisposeEventHandlerUPP(gStatWinEvtHandler);
		        DisposeWindow(gStatWindow);
		       	gStatWindow = NULL;
		        result = noErr;
		        break;

		    case kEventWindowDrawContent:
		    	StatWindowUpdater();
		    	break;
		    	
		    case kEventWindowBoundsChanging:
		    	DrawControls(gStatWindow);
		   		break;
		 
		 	default:
		 		break;
			}
		 
		default:
			break;
	}
	
	// Done
    return result;
}

/* --------------------------------------------------- MakeStatWindow ---- *\

	Create the status window
	
\* ----------------------------------------------------------------------- */

int MakeStatWindow(int isMacOSX)
{
	int					i;
	WindowPtr			myWindow;
	EventHandlerRef		ref;
	int					attrib;
	Rect					screenrect, winrect;
	GDHandle				myDevice;
	
	EventTypeSpec	evtlist[] = { {kEventClassWindow, kEventWindowClose},
								  {kEventClassWindow, kEventWindowDrawContent},
								  {kEventClassWindow, kEventWindowBoundsChanging},
								  {kEventClassCommand, kEventCommandProcess}
								   };

	if (rowsOK == 0) {		// Allocate content buffer at first call
		for (i = 0; i < MAX_ROWS ; i++) {
			rowTab[i] = calloc(1, ROW_SIZE+2);
		}
		rowsOK = 1;
	}
	rowCnt = 0;

	myDevice = GetMainDevice();
	GetAvailableWindowPositioningBounds( myDevice, &screenrect );

	SetRect(&winrect, screenrect.left + 10, 50, screenrect.left + 10 + 320, 470);
	myWindow = NewCWindow(nil, &winrect, "\pTP Status", true, zoomDocProc, (WindowPtr) -1, true, 0);
	
	if (myWindow != nil) {
		SetPort(GetWindowPort(myWindow));
		TextFont(kFontIDMonaco);
		TextSize(9);
		gStatWindow = myWindow;
	} else {
		DebugStr("\pNewWindow failed");
	}
	
	// Set Window Attributes for Carbon Events Handling and closebox
	attrib = kWindowStandardHandlerAttribute | kWindowCloseBoxAttribute;
	if (isMacOSX) attrib |= kWindowLiveResizeAttribute;
	(void) ChangeWindowAttributes(gStatWindow, attrib, NULL);

	// Set The correct Window background theme
	(void) SetThemeWindowBackground(gStatWindow, kThemeBrushDialogBackgroundActive, true);

	// Add EventHandler to the Window
	gStatWinEvtHandler = NewEventHandlerUPP(StatWindowEventHandler);
	InstallWindowEventHandler(gStatWindow, gStatWinEvtHandler, 4, evtlist, 0, &ref);

	// Show it
	ShowWindow(gStatWindow);
	DrawControls(gStatWindow);
	return winrect.right;
}

/* ------------------------------------------------ StatWindowUpdater ---- *\

	Redraw status window contents
	
\* ----------------------------------------------------------------------- */

static void StatWindowUpdater(void)
{
	int			i;
	Rect		r;
	
	for (i = 0 ; i < rowCnt ; i++) {
		SetRect(&r, 10, (12 * i) + 2, 800, (12 * i) + 14);
		TETextBox(rowTab[i], strlen(rowTab[i]), &r, teJustLeft);
	}
}

/* --------------------------------------------------------- PrintMsg ---- *\

	Print message in status window

\* ----------------------------------------------------------------------- */

void PrintMsg(char *msg)
{
	Rect		tempRect;
	char		*dp;
	GrafPtr		oldPort;
	
	if (gStatWindow == 0) return;

	if (msg[0] == '\f') {
		GetPort(&oldPort);
		SetPort(GetWindowPort(gStatWindow));
		EraseRect(GetWindowPortBounds(gStatWindow, &tempRect));
		SetPort(oldPort);
		rowCnt = 0;
		return;
	}

	if (rowCnt >= MAX_ROWS) return;

	dp = rowTab[rowCnt];
	strncpy(dp, msg, ROW_SIZE);
	dp[ROW_SIZE-1] = 0;
	
	SetRect(&tempRect, 10, (12 * rowCnt) + 2, 800, (12 * rowCnt) + 14);
	InvalWindowRect(gStatWindow, &tempRect);
	rowCnt += 1;	
}

/* -------------------------------------------------- ClearStatWindow ---- *\

	Clear status window contents
	
\* ----------------------------------------------------------------------- */

void ClearStatWindow(void)
{
	PrintMsg("\f");
}

/* ----------------------------------------------------------- Report ---- *\

	Report message to status window and to log
	
\* ----------------------------------------------------------------------- */

void Report(char *str)
{
	TrcLog(1, "%s", str);
	PrintMsg(str);
}

/* ------------------------------------------------- ShowNewBestRates ---- *\

	Show best send and receive rates
	
\* ----------------------------------------------------------------------- */

void ShowNewBestRates(TPEngine *engp)
{
	char	tbuf[80];
	
	sprintf(tbuf, "BestTCPSendRate = %d", (int)(engp->bestTCPSendRate * 8.0)); Report(tbuf);
	sprintf(tbuf, "BestTCPRcvRate  = %d", (int)(engp->bestTCPRecvRate * 8.0)); Report(tbuf);
	sprintf(tbuf, "BestUDPSendRate = %d", (int)(engp->bestUDPSendRate * 8.0)); Report(tbuf);
	sprintf(tbuf, "BestUDPRcvRate  = %d", (int)(engp->bestUDPRecvRate * 8.0)); Report(tbuf);
}

char *myTestText[] =
{
  "None",
  "UDP Full Duplex",
  "UDP Transmit",
  "UDP Receive",
  "TCP Transmit",
  "TCP Receive",
  "Query Master",
};

/* --------------------------------------------------- Long64ToString ---- *\

	Convert a long long value to a string formatted as
	"64.25 Kbyte" or "10.16 Mbyte" or "103 byte".

\* ----------------------------------------------------------------------- */

char *Long64ToString( long long lVal )
{
	static char sBuf[ 256 ];

	if ( lVal >= 1024*1024 ) {
		sprintf( sBuf, "%.2f Mbyte", (double)(lVal) / ( 1024.0 * 1024.0 ) );
	      
	} else if ( lVal > 1024 ) {
		sprintf( sBuf, "%.2f Kbyte", (double)(lVal) / 1024.0 );
    
	} else {
		sprintf( sBuf, "%" LONG_LONG_PREFIX "d byte", lVal );
	}
	return( sBuf );
}

/* ------------------------------------------------------ GetTestName ---- *\

	Get decription of specified subtest
	
\* ----------------------------------------------------------------------- */

static char *GetTestName(int testType)
{
	switch (testType) {
	case CLM_AUTO:			return "Auto";
	case CLM_AUTO_SEND:		return "Auto sŠndning";
	case CLM_AUTO_RECV:		return "Auto mottagning";
	case CLM_TCP_SEND:		return "TCP sŠndning";
	case CLM_TCP_RECV:		return "TCP mottagning";
	case CLM_UDP_SEND:		return "UDP sŠndning";
	case CLM_UDP_RECV:		return "UDP mottagning";
	case CLM_UDP_FDX:		return "UDP full duplex";
	case M_QUERY_MASTER:	return "Serverlistning";
	}
	return "??";
}

/* -------------------------------------------------- BuildAutoValues ---- *\

	Create result info for automatic test modes
	
\* ----------------------------------------------------------------------- */

static void BuildAutoValues(TPEngine *engp, int testType, char *dp)
{
	double		sendRes;
	
	if (testType == CLM_AUTO || testType == CLM_AUTO_SEND) {
		
		dp += sprintf(dp, "SŠndning:\r\r");			
		dp += sprintf(dp, "Hšgsta hastighet TCP: ");
		if (engp->bestTCPSendRate) {
			dp += sprintf(dp, "%s\r", Int32ToString((int)(engp->bestTCPSendRate * 8.0)));
		} else {
			dp += sprintf(dp, "ej testat\r");
		}
		dp += sprintf(dp, "Hšgsta hastighet UDP: ");
		if (engp->bestUDPSendRate) {
			dp += sprintf(dp, "%s\r", Int32ToString((int)(engp->bestUDPSendRate * 8.0)));
		} else {
			dp += sprintf(dp, "ej testat\r");
		}

		if (engp->bestTCPSendRate > 0.0 && engp->bestUDPSendRate > 0.0) {
			sendRes = ((engp->bestTCPSendRate/engp->bestUDPSendRate) * 100.0);
			if (sendRes > 100.0) sendRes = 100.0;
			dp += sprintf(dp, "TillgŠnglig bandbredd TCP/UDP: %02.1f %%\r", sendRes);
		}
	}
	if (testType == CLM_AUTO || testType == CLM_AUTO_RECV) {
		
		if (testType == CLM_AUTO) dp += sprintf(dp, "\r");			
		dp += sprintf(dp, "Mottagning:\r\r");			
		dp += sprintf(dp, "Hšgsta hastighet TCP: ");
		if (engp->bestTCPRecvRate) {
			dp += sprintf(dp, "%s\r", Int32ToString((int)(engp->bestTCPRecvRate * 8.0)));
		} else {
			dp += sprintf(dp, "ej testat\r");
		}
		dp += sprintf(dp, "Hšgsta hastighet UDP: ");
		if (engp->bestUDPRecvRate) {
			dp += sprintf(dp, "%s\r", Int32ToString((int)(engp->bestUDPRecvRate * 8.0)));
		} else {
			dp += sprintf(dp, "ej testat\r");
		}

		if (engp->bestTCPRecvRate > 0.0 && engp->bestUDPRecvRate > 0.0) {
			sendRes = ((engp->bestTCPRecvRate/engp->bestUDPRecvRate) * 100.0);
			if (sendRes > 100.0) sendRes = 100.0;
			dp += sprintf(dp, "TillgŠnglig bandbredd TCP/UDP: %02.1f %%\r", sendRes);
		}
	}
}

/* ---------------------------------------------- BuildAdvancedValues ---- *\

	Create result info for advanced test modes	

\* ----------------------------------------------------------------------- */

static void BuildAdvancedValues(TPEngine *engp, int testType, char *dp)
{
	int			throughput;
	LONG_LONG	recvtime;
	struct tm	*tmPnt;
	
	if (testType == CLM_TCP_SEND || testType == CLM_TCP_RECV) {
		dp += sprintf(dp, "TCP Bytes:    %d\r", engp->tcpBytes);
	} else {
		dp += sprintf(dp, "Antal paket:  %d\r", engp->nPackets);
		dp += sprintf(dp, "Paketstorlek: %d\r", engp->packetSize);
	}

	tmPnt = localtime((time_t *)(&engp->stats.StartSend.tv_sec));
	dp += sprintf(dp, "SŠndning startad:    %04d-%02d-%02d %02d:%02d:%02d\r",
			tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
			tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec);

	tmPnt = localtime((time_t *)(&engp->stats.StopSend.tv_sec));
	dp += sprintf(dp, "SŠndning avslutad:   %04d-%02d-%02d %02d:%02d:%02d\r",
			tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
			tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec);

	tmPnt = localtime((time_t *)(&engp->stats.StartRecv.tv_sec));
	dp += sprintf(dp, "Mottagning startad:  %04d-%02d-%02d %02d:%02d:%02d\r",
			tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
			tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec);

	tmPnt = localtime((time_t *)(&engp->stats.StopRecv.tv_sec));
	dp += sprintf(dp, "Mottagning avslutad: %04d-%02d-%02d %02d:%02d:%02d\r",
			tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
			tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec);

	if (testType == CLM_UDP_SEND || testType == CLM_UDP_RECV || testType == CLM_UDP_FDX) {
		dp += sprintf(dp, "Paketresultat:\r");
		dp += sprintf(dp, "SŠnda:       %d\r", engp->stats.PktsSent);
		dp += sprintf(dp, "Mottagna :   %d\r", engp->stats.PktsRecvd);
		dp += sprintf(dp, "Tappade:     %d\r", engp->stats.PktsSent - engp->stats.PktsRecvd);
		dp += sprintf(dp, "OsŠnda:      %d\r", engp->stats.PktsUnSent);
		if (testType == CLM_UDP_FDX) {
			if (engp->stats.nRoundtrips > 0) {
				dp += sprintf(dp, "Max roundtrip: %0.3fms\r", (double)engp->stats.MaxRoundtrip / 1000.0);
				dp += sprintf(dp, "Min roundtrip: %0.3fms\r", (double)engp->stats.MinRoundtrip / 1000.0);
				dp += sprintf(dp, "Med roundtrip: %0.3fms\r", 
					((double)engp->stats.TotalRoundtrip / (double)engp->stats.nRoundtrips) / 1000.0);
			}
		}
	}
	dp += sprintf(dp, "Bytes skickade: %" LONG_LONG_PREFIX "d\r", engp->stats.BytesSent);
	dp += sprintf(dp, "Bytes mottagna: %" LONG_LONG_PREFIX "d\r", engp->stats.BytesRecvd);
	recvtime = (LONG_LONG)engp->stats.StopRecv.tv_sec * (LONG_LONG)1000000 + 
					(LONG_LONG)engp->stats.StopRecv.tv_usec;
	recvtime -= ((LONG_LONG)engp->stats.StartRecv.tv_sec * (LONG_LONG)1000000 + 
					(LONG_LONG)engp->stats.StartRecv.tv_usec);
	if (recvtime > 0) 
		throughput = (int)((double)(engp->stats.BytesRecvd * 8)/((double)recvtime / 1000000.0));
	else
		throughput = 0;
	dp += sprintf(dp, "Bandbredd: %d bps (%s)\r", (int)throughput, Int32ToString((int)throughput));
}

/* ----------------------------------------------------- ReportToClip ---- *\

	Put test report into clipboard
	
\* ----------------------------------------------------------------------- */

void ReportToClip(
	TPEngine		*engp,
	int				testType,
	time_t			testStartTime,
	time_t			testStopTime)
{
	int			i;
	char		*dp;
	struct tm	*tmPnt;
	ScrapRef	scrap;
	char		*clipBuf	= 0;
	
	clipBuf = (char*) malloc(8000);
	if (clipBuf == 0) return;
	
	dp = clipBuf;
	dp += sprintf(dp, "Testresultat:\r");
	dp += sprintf(dp, "-------------\r");
	dp += sprintf(dp, "\r");
	dp += sprintf(dp, "Server: %s:%d\r", engp->hostName, engp->hostCtrlPort);
	dp += sprintf(dp, "Test:   %d (%s)\r", engp->tpMode, GetTestName(testType));
	dp += sprintf(dp, "Tid :   %d s  Maxtid:   %d s\r",
			engp->sessionTime, engp->sessionMaxTime);

	tmPnt = localtime((time_t *)(&testStartTime));
	dp += sprintf(dp, "Test startad : %04d-%02d-%02d %02d:%02d:%02d\r",
			tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
			tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec);

	tmPnt = localtime((time_t *)(&testStopTime));
	dp += sprintf(dp, "Test avslutad: %04d-%02d-%02d %02d:%02d:%02d\r",
			tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
			tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec);
	
	dp += sprintf(dp, "\r");
	
	switch (testType) {
	
	case CLM_AUTO:
	case CLM_AUTO_SEND:
	case CLM_AUTO_RECV:
		BuildAutoValues(engp, testType, dp);
		break;
		
	case CLM_TCP_SEND:
	case CLM_TCP_RECV:
	case CLM_UDP_SEND:
	case CLM_UDP_RECV:
	case CLM_UDP_FDX:
		BuildAdvancedValues(engp, testType, dp);
		break;

	case M_QUERY_MASTER:
		dp += sprintf(dp, "Testserverlista:\r\r");
		for (i = 0; i < engp->numServers; i++) {
			dp += sprintf(dp, "%02d:  \"%s\" (%s:%d)\r",
				i, engp->serverInfoList[i], engp->serverNameList[i], engp->serverPortList[i]);
		}
		break;
	}

	// Wipe old scrap contents and insert the generated result
	
	ClearCurrentScrap();
	if (GetCurrentScrap(&scrap) == noErr) {
		PutScrapFlavor(scrap, kScrapFlavorTypeText, kScrapFlavorMaskNone, strlen(clipBuf), (void*) clipBuf);
	}
	free(clipBuf);
}

/* ------------------------------------------------------ ReportStats ---- *\

	Show test result in status window
	
\* ----------------------------------------------------------------------- */

void ReportStats(struct TPEngine *engp)
{
	int			Hours, Minutes, Seconds, msSend, msRcv;
	double		BytesPerSecondSend, BytesPerSecondRcv;
	char		str[200], str2[200];
	char		tBuf1[ 256 ];
	struct		tm *tmPnt;
	struct timeval StopTime;
	static double STBytesPerSecond = 0;
	
	gettimeofday( &StopTime, NULL );
	ClearStatWindow();

	msSend = ( engp->stats.StopSend.tv_sec - engp->stats.StartSend.tv_sec ) * 1000;
	msSend += ( engp->stats.StopSend.tv_usec - engp->stats.StartSend.tv_usec ) / 1000;
  	if ( msSend != 0 ) {
    	BytesPerSecondSend = ( (double)(engp->stats.BytesSent) * 1000.0 ) / (double)(msSend);
 	} else {
    	BytesPerSecondSend = 0.0;
	}
	
	msRcv = ( engp->stats.StopRecv.tv_sec - engp->stats.StartRecv.tv_sec ) * 1000;
	msRcv += ( engp->stats.StopRecv.tv_usec - engp->stats.StartRecv.tv_usec ) / 1000;
	
  	if ( msRcv != 0 ) {
		BytesPerSecondRcv = ( (double)(engp->stats.BytesRecvd) * 1000.0 ) / (double)(msRcv);
	} else {
		BytesPerSecondRcv = 0.0;
	}
	sprintf(str, "Throughput test results:");
	Report(str);

	str[0] = str2[0] = 0;
	switch (engp->tpMode) {
	
	case M_TCP_SEND:
	case M_UDP_SEND:
	case M_UDP_FDX:
		tBuf1;		// touch
		sprintf(str2, "Destination machine :    %s",
				engp->hostName);
		break;
		
	case M_TCP_RECV:
	case M_UDP_RECV:
		sprintf(str, "Source machine :         %s",
				engp->hostName);
		break;
	
	default:
		sprintf(str, "Source machine :         Unknown");
		sprintf(str2, "Destination machine :    Unknown");
		break;
	}
	Report(str);
	Report(str2);

	if (engp->tpMode <= 6) {
		sprintf(str, "Type of test :           %s", myTestText[ engp->tpMode ]);
		Report(str);
	}

	tmPnt = localtime( &engp->startTime );
	sprintf(str, "Test started :           %04d-%02d-%02d %02d:%02d:%02d",
			tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
			tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec);
	Report(str);
	
	tmPnt = localtime( (time_t *)(&StopTime.tv_sec) );
	sprintf(str, "Test ended :             %04d-%02d-%02d %02d:%02d:%02d",
			tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
			tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec);
	Report(str);

	Hours = msSend / 3600000;
	msSend = msSend % 3600000;
	Minutes = msSend / 60000;
	msSend = msSend % 60000;
	Seconds = msSend / 1000;
	msSend = msSend % 1000;

	Report("");
	sprintf(str, "Send statistics" );
	Report(str);
	sprintf(str, "  Send time :            %02d:%02d:%02d.%03d",
			Hours, Minutes, Seconds, msSend );
	Report(str);

	switch (engp->tpMode) {
	case M_TCP_RECV:
	case M_UDP_SEND:
	case M_UDP_FDX:
		sprintf(str, "  Packets sent :         %d", engp->stats.PktsSent );
		Report(str);
		sprintf(str, "  Unable to send :       %d", engp->stats.PktsUnSent );
		Report(str);
		break;
	}

	sprintf(str, "  Bytes sent :           %" LONG_LONG_PREFIX "d  (%s)",
			engp->stats.BytesSent, Long64ToString( engp->stats.BytesSent ) );
	Report(str);
	
	sprintf(str, "  Bits/second sent :     %d  (%s)",
			(int)(BytesPerSecondSend * 8.0),
			Int32ToString( (int)(BytesPerSecondSend * 8.0) ) );
	Report(str);

	Hours = msRcv / 3600000;
	msRcv = msRcv % 3600000;
	Minutes = msRcv / 60000;
	msRcv = msRcv % 60000;
	Seconds = msRcv / 1000;
	msRcv = msRcv % 1000;
	Report("");
	
	sprintf(str, "Receive statistics" );
	Report(str);
	
	sprintf(str, "  Receive time :         %02d:%02d:%02d.%03d",
			Hours, Minutes, Seconds, msRcv );
	Report(str);

	switch (engp->tpMode) {
	case M_TCP_RECV:
	case M_UDP_SEND:
	case M_UDP_FDX:
		sprintf(str, "  Packets received :     %d", engp->stats.PktsRecvd );
		Report(str);
		break;
	}

	sprintf(str, "  Bytes received :       %" LONG_LONG_PREFIX "d  (%s)",
			engp->stats.BytesRecvd, Long64ToString( engp->stats.BytesRecvd ) );
	Report(str);

	sprintf(str, "  Bits/second received : %d  (%s)",
			(int)(BytesPerSecondRcv * 8.0),
			Int32ToString( (int)(BytesPerSecondRcv * 8.0) ) );
	Report(str);

	switch (engp->tpMode) {
	case M_UDP_RECV:
	case M_UDP_SEND:
	case M_UDP_FDX:
		Report("");
		sprintf(str, "Lost packets" );
		Report(str);
		if ( engp->stats.PktsSent != 0 ) {
			sprintf(str, "  Packets lost on net :  %d (%.2f%%)",
					engp->stats.PktsSent - engp->stats.PktsRecvd,
					( (double)(engp->stats.PktsSent - engp->stats.PktsRecvd) * 100.0 )
					/ (double)(engp->stats.PktsSent + engp->stats.PktsUnSent) );
			Report(str);
			sprintf(str, "  Unable to send :       %d (%.2f%%)", engp->stats.PktsUnSent,
					( (double)(engp->stats.PktsUnSent) * 100.0 )
					/ (double)(engp->stats.PktsSent + engp->stats.PktsUnSent) );
			Report(str);
			sprintf(str, "  Total packets lost :   %d (%.2f%%)",
					( engp->stats.PktsSent - engp->stats.PktsRecvd ) + engp->stats.PktsUnSent,
					( (double)(( engp->stats.PktsSent - engp->stats.PktsRecvd )
					+ engp->stats.PktsUnSent) * 100.0 )
					/ (double)(engp->stats.PktsSent + engp->stats.PktsUnSent) );
			Report(str);
		} else {
			sprintf(str, "  Packets lost on net :  %d", engp->stats.PktsSent - engp->stats.PktsRecvd );
			Report(str);
			sprintf(str, "  Unable to send :       %d", engp->stats.PktsUnSent );
			Report(str);
			sprintf(str, "  Total packets lost :   %d",
					( engp->stats.PktsSent - engp->stats.PktsRecvd ) + engp->stats.PktsUnSent );
			Report(str);
		}
	}

	if (engp->tpMode == M_UDP_FDX) {
		if (engp->stats.nRoundtrips != 0) {
			Report("");
			sprintf(str, "Roundtrip statistics");
			Report(str);
			sprintf(str, "  Min roundtrip delay :  %d.%03d ms",
					engp->stats.MinRoundtrip / 1000, engp->stats.MinRoundtrip % 1000 );
			Report(str);
			sprintf(str, "  Max roundtrip delay :  %d.%03d ms",
					(int)(engp->stats.MaxRoundtrip / 1000, engp->stats.MaxRoundtrip % 1000));
			Report(str);
			sprintf(str, "  Avg roundtrip delay :  %d.%03d ms",
					(int)((engp->stats.TotalRoundtrip / engp->stats.nRoundtrips) / 1000),
					(int)((engp->stats.TotalRoundtrip / engp->stats.nRoundtrips) % 1000));
			Report(str);
		}
	}
	Report("");
}

/* ---------------------------------------------------- ValidateRates ---- *\

	Validate user specified data rates and packet sizes
	
\* ----------------------------------------------------------------------- */

void ValidateRates(DataRates *drp, TPEngine *engp)
{
	drp->errorFlag = 0;
	drp->message[0] = 0;
	if (drp->newPacketRate != drp->oldPacketRate) {
		if (drp->newPacketRate < 1) {
			sprintf(drp->message, "Minimum packets per second is 1, sorry");
			drp->newPacketRate = drp->oldPacketRate;
			drp->errorFlag = 1;
		} else {
			drp->oldPacketRate = drp->newPacketRate;
			engp->packetsPerSecond = drp->newPacketRate;
			engp->bitsPerSecond = drp->newPacketRate * engp->packetSize * 8;
			drp->newDataRate = drp->oldDataRate = engp->bitsPerSecond;
		}
	}
	if (drp->newPacketSize != drp->oldPacketSize) {
		if (drp->newPacketSize < MIN_PKT_SIZE) {
			sprintf(drp->message, "Minimum packet size is %d, sorry", MIN_PKT_SIZE);
			drp->newPacketSize = drp->oldPacketSize;
			drp->errorFlag = 1;
		} else if (drp->newPacketSize > MAX_PKT_SIZE) {
			sprintf(drp->message, "Max packet size is %d, sorry", MAX_PKT_SIZE);
			drp->newPacketSize = drp->oldPacketSize;
			drp->errorFlag = 1;
		} else {
			drp->oldPacketSize = drp->newPacketSize;
			engp->packetSize = drp->newPacketSize;
			engp->bitsPerSecond = drp->newPacketSize * engp->packetsPerSecond * 8;
			drp->newDataRate = drp->oldDataRate = engp->bitsPerSecond;
		}
	}
	if (drp->newDataRate != drp->oldDataRate) {
		if (drp->newDataRate < (MIN_PKT_SIZE * 8)) {
			sprintf(drp->message, "Minimum bitrate is %d, sorry", MIN_PKT_SIZE * 8);
			drp->newDataRate = drp->oldDataRate;
			drp->errorFlag = 1;
		} else {
			drp->oldDataRate = drp->newDataRate;
			engp->bitsPerSecond = drp->newDataRate;
			RecalculatePPSSZ(engp);
			engp->bitsPerSecond = engp->packetSize * engp->packetsPerSecond * 8;
			drp->oldDataRate = engp->bitsPerSecond;
			drp->oldPacketSize = engp->packetSize;
			drp->oldPacketRate = engp->packetsPerSecond;
		}
	}
}

#define isspace(x) ((x) == ' ' || (x) == '\t')
#define isalnum(x) (((x | 0x20) >= 'A' && (x | 0x20) <= 'Z') || ((x) >= '0' && (x) <= '9'))

static void IsoToMac(
	char	*datap)
{
	unsigned char	*ucp = (unsigned char *) datap;
	int		c;
	
	while (*ucp) {
		c = *ucp;
		if      (c == 0xC5) c = 0x81;
		else if (c == 0xC4) c = 0x80;
		else if (c == 0xD6) c = 0x85;
		else if (c == 0xDC) c = 0x86;
		else if (c == 0xC9) c = 0x83;
		else if (c == 0xE5) c = 0x8C;
		else if (c == 0xE4) c = 0x8A;
		else if (c == 0xF6) c = 0x9A;
		else if (c == 0xFC) c = 0x9F;
		else if (c == 0xE9) c = 0x8E;
		*ucp = c;
		ucp += 1;
	}
}
char *PutOctets(
	char		*destp,
	struct in_addr *ip)
{
	sprintf(destp, "%d.%d.%d.%d", (ip->s_addr >> 24) & 255, (ip->s_addr >> 16) & 255,
		(ip->s_addr >> 8) & 255, ip->s_addr & 255);
	return destp + strlen(destp);
}

