/*
 * $Id: tptest3.cpp,v 1.6 2003/10/09 13:53:56 rlonn Exp $
 * $Source: /cvsroot-fuse/tptest/apps/windows/clients/gui/tptest3.cpp,v $
 *
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * tptest3.cpp - Win32 TPTEST Client app
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

// This program has been compiled in MSVC++ 6.0. Be sure to link in ws2_32.lib
// and compile it as a standard Win32 application
//
// This program creates an invisible main window used only as parent window
// to the dialog boxes that make up the user interface of the program. There 
// are two dialog boxes: IDD_STANDARD and IDD_AVANCERAT. IDD_AVANCERAT has
// more options and settings than IDD_STANDARD. 
// 
// The main work is done by the tptest() function which gets called repeatedly
// by WinMain() in its message loop. The variables "myState" and "substate" 
// determines what tptest() should do when it is called. 
//
// This code needs some cleaning up but it has not had high priority lately
// as the program mostly serves as an example of how to use the TPTEST engine.
// /prl@gatorhole.com 020912
// 



#define NO_QUAD_PRINTF

#define MAX_LOADSTRING 100

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <winuser.h>
#include <winbase.h>
#include <wingdi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <process.h>

#include "resource.h"

#include "tpengine.h"
#include "tptest.h"
#include "tpcommon.h"
#include "tpclient.h"
#include "tpio_win32.h"

extern char *TestText[];

// Global Variables:
HINSTANCE hInst;								// current instance
HWND g_hwndTT;
HHOOK g_hhk;
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// The title bar text
BOOL g_bIsVisible = FALSE;

TPEngine * engp;

WINDOWPLACEMENT windowplacement;
BOOL firstwindow;
//struct tagMOUSEMOVEPOINT mmp, oldmmp;
//MOUSEMOVEPOINT mmp, oldmmp;
BOOL Advanced;
BOOL WaitMsg = FALSE;
HWND hWnd, TextWindow, CurDlg, ProgressBar;
HBRUSH progress_brush;
POINT mousepos, oldmousepos;
INT64 lastmousemove, perf_freq;
time_t	FirstStartTime;
int now_displaying = 0;

int iter = 0;
int selectedMode = M_NONE;
int testRunning = 0;
int gotServerList = 0;
int Progress_X;
int myState = MYSTATE_IDLE;
int substate = 0;
char selectedMaster[MAX_SERVER_NAME];
int selectedMasterPort = 0;
int selectedServerPort = 0;
char selectedServer[MAX_SERVER_NAME];
char selectedServerAdr[MAX_SERVER_NAME];

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	Standard(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	AdvancedDlg(HWND, UINT, WPARAM, LPARAM);
//LRESULT CALLBACK	Valj(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	Help(HWND, UINT, WPARAM, LPARAM);

#ifndef ENGLISH
LRESULT CALLBACK	PULDlg(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	PULDlg2(HWND, UINT, WPARAM, LPARAM);
#endif

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

	LoadString(hInstance, IDC_TPTEST2, szWindowClass, MAX_LOADSTRING);

	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_TPTEST2);

	// Init TPTEST stuff - winsock, data structures etc.
	Initialize();

	firstwindow = true;
	if (!stricmp(lpCmdLine, "/a"))
		CreateDialog(hInst, (LPCTSTR)IDD_ADVANCED, hWnd, (DLGPROC)AdvancedDlg);
	else {
		CreateDialog(hInst, (LPCTSTR)IDD_STANDARD, hWnd, (DLGPROC)Standard);
	}

	if (QueryPerformanceFrequency((LARGE_INTEGER *)&perf_freq) != TRUE) {
		MessageBox(hWnd, "Unable to initialize performance counter", "Error", MB_OK);
		exit(1);
	}
	QueryPerformanceCounter((LARGE_INTEGER *)&lastmousemove);


	// Main message loop:
	while ( true )
	{
		iter++;
		if (WaitMsg)
			WaitMessage();
		while (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ))     
		{
			if( !GetMessage( &msg, NULL, 0, 0 ) )
				return (int)(msg.wParam);
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg); 
				DispatchMessage(&msg);
			}
			if (++iter % 16 == 0)
				tptest();	// Run the main tptest code	
		}
		tptest();		// Run the main tptest code				
	}

	return (int)(msg.wParam);
}



// Init Winsock etc.
void Initialize()
{
	OSVERSIONINFO osvi;
    char clientinfo[100];

	outtext = servers = NULL;
	progress_brush = CreateSolidBrush(0x00ff5555);
	strcpy(Info, TPTEST_INFO);
	selectedMode = CLM_AUTO;

	strcpy(selectedMaster, MASTERSERVER);
	selectedMasterPort = MASTERPORT;

	engp = CreateContext();
	if (engp == NULL) {
		MessageBox(hWnd, "Failed to initialize TPTEST engine", "Error", MB_OK);
		exit(1);
	}

    sprintf(clientinfo, "TPTEST %d.%d", MAJORVERSION, MINORVERSION);

	// Initialize the PRINTDLG members. 
 
	pd.lStructSize = sizeof(PRINTDLG); 
	pd.hDevMode = (HANDLE) NULL; 
	pd.hDevNames = (HANDLE) NULL; 
	pd.Flags = PD_RETURNDC; 
	pd.hwndOwner = hWnd; 
	pd.hDC = (HDC) NULL; 
	pd.nFromPage = 1; 
	pd.nToPage = 1; 
	pd.nMinPage = 0; 
	pd.nMaxPage = 0; 
	pd.nCopies = 1; 
	pd.hInstance = (HINSTANCE) NULL; 
	pd.lCustData = 0L; 
	pd.lpfnPrintHook = (LPPRINTHOOKPROC) NULL; 
	pd.lpfnSetupHook = (LPSETUPHOOKPROC) NULL; 
	pd.lpPrintTemplateName = (LPSTR) NULL; 
	pd.lpSetupTemplateName = (LPSTR)  NULL; 
	pd.hPrintTemplate = (HANDLE) NULL; 
	pd.hSetupTemplate = (HANDLE) NULL; 

/*
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_USEREX_CLASSES + ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icce);
*/

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx (&osvi);
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		switch (osvi.dwMajorVersion) {
		case 3:
		case 4:
			sprintf(engp->clientInfo, "%s Win NT ", clientinfo);
			// NT 3.51, NT 4
			break;
		case 5:
			if (osvi.dwMinorVersion < 1) 
				sprintf(engp->clientInfo, "%s Win 2000 ", clientinfo);
			else if (osvi.dwMinorVersion == 1)
				sprintf(engp->clientInfo, "%s Win XP ", clientinfo);
			else
				sprintf(engp->clientInfo, "%s Windows %d.%d ", clientinfo, osvi.dwMajorVersion,
					osvi.dwMinorVersion);
			break;
		default:
				sprintf(engp->clientInfo, "%s Windows %d.%d ", clientinfo, osvi.dwMajorVersion,
					osvi.dwMinorVersion);
		}
	}
	else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
		if (osvi.dwMinorVersion == 0) 
			sprintf(engp->clientInfo, "%s Win 95 ", clientinfo);
		else
			sprintf(engp->clientInfo, "%s Win 98 ", clientinfo);
	}
	strncat(engp->clientInfo, osvi.szCSDVersion, 79-strlen(engp->clientInfo));

}


void UpdateLoadResults()
{
	if (!Advanced) {
		double sendres, recvres;
		if (engp->bestTCPSendRate > 0.0 && engp->bestUDPSendRate > 0.0) {
			sendres = (((double)engp->bestTCPSendRate/(double)engp->bestUDPSendRate) * 100.0);
			if (sendres > 100.0) sendres = 100.0;
			sprintf(tmp, "%02.1f %%", sendres);
			SetDlgItemText(CurDlg, IDC_SENDLOAD, tmp);
		}
		if (engp->bestTCPRecvRate > 0.0 && engp->bestUDPRecvRate > 0.0) {
			recvres = (((double)engp->bestTCPRecvRate/(double)engp->bestUDPRecvRate) * 100.0);
			if (recvres > 100.0) recvres = 100.0;
			sprintf(tmp, "%02.1f %%", recvres);
			SetDlgItemText(CurDlg, IDC_RCVLOAD, tmp);
		}
	}
}

void UpdateBestRate(TPEngine * engp)
{
	if (Advanced)
		return;

	if (engp->bestTCPSendRate == 0.0)
		SetDlgItemText(CurDlg, IDC_TCPSEND, "");
	else
		SetDlgItemText(CurDlg, IDC_TCPSEND, 
		Int32ToString((int)(engp->bestTCPSendRate * 8.0)));

	if (engp->bestTCPRecvRate == 0.0)
		SetDlgItemText(CurDlg, IDC_TCPRCV, "");
	else
		SetDlgItemText(CurDlg, IDC_TCPRCV, 
		Int32ToString((int)(engp->bestTCPRecvRate * 8.0)));

	if (engp->bestUDPSendRate == 0.0)
		SetDlgItemText(CurDlg, IDC_UDPSEND, "");
	else
		SetDlgItemText(CurDlg, IDC_UDPSEND, 
		Int32ToString((int)(engp->bestUDPSendRate * 8.0)));

	if (engp->bestUDPRecvRate == 0.0)
		SetDlgItemText(CurDlg, IDC_UDPRCV, "");
	else
		SetDlgItemText(CurDlg, IDC_UDPRCV, 
		Int32ToString((int)(engp->bestUDPRecvRate * 8.0)));

}

void ClearTestResults() {
	if (!Advanced)
	{
		SetDlgItemText(CurDlg, IDC_SENDLOAD, "");
		SetDlgItemText(CurDlg, IDC_RCVLOAD, "");
		SetDlgItemText(CurDlg, IDC_TCPSEND, "");
		SetDlgItemText(CurDlg, IDC_TCPRCV, "");
		SetDlgItemText(CurDlg, IDC_UDPSEND, "");
		SetDlgItemText(CurDlg, IDC_UDPRCV, "");
		SetDlgItemText(CurDlg, IDC_TCPBYTES, "");
		SetDlgItemText(CurDlg, IDC_UDPSPEED, "");
	}
}


int ServerSelected() {
	int ind, i;
	char str[200];
	ind = (int)SendDlgItemMessage(CurDlg, IDC_SERVERLIST, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
	if (ind == LB_ERR) 
		return 0;
	SendDlgItemMessage(CurDlg, IDC_SERVERLIST, LB_GETTEXT, (WPARAM)ind, (LPARAM)str);
	for (i = 0; i < engp->numServers; i++) {
		if (strcmp(str, engp->serverInfoList[i]) == 0) {
			strcpy(selectedServer, engp->serverInfoList[i]);
			strcpy(selectedServerAdr, engp->serverNameList[i]);
			selectedServerPort = engp->serverPortList[i];
			SetDlgItemText(CurDlg, IDC_SERVERNAME, engp->serverNameList[i]);
			SetDlgItemInt(CurDlg, IDC_SERVERPORT, engp->serverPortList[i], FALSE);
			return 1;
		}
	}
	return 0;
}

void UpdateServerList() {
	int i;
	if (engp->numServers > 0) {
		SendDlgItemMessage(CurDlg, IDC_SERVERLIST, LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
		for (i = 0; i < engp->numServers; i++) {
			SendDlgItemMessage(CurDlg, IDC_SERVERLIST, LB_ADDSTRING, 
				(WPARAM)0, (LPARAM)(engp->serverInfoList[i]));
		}
	}	
}


void UpdateTesting(TPEngine * engp) {
	if (Advanced) {
		switch (selectedMode)
		{
			case M_TCP_SEND:
			case M_UDP_SEND:
				SendDlgItemMessage(CurDlg, IDC_RADIO1, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM) 0);
				SendDlgItemMessage(CurDlg, IDC_RADIO2, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM) 0);
				SendDlgItemMessage(CurDlg, IDC_RADIO3, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM) 0);
				break;
			case M_TCP_RECV:
			case M_UDP_RECV:
				SendDlgItemMessage(CurDlg, IDC_RADIO2, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM) 0);
				SendDlgItemMessage(CurDlg, IDC_RADIO1, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM) 0);
				SendDlgItemMessage(CurDlg, IDC_RADIO3, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM) 0);
				break;
			case M_UDP_FDX:
				SendDlgItemMessage(CurDlg, IDC_RADIO3, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM) 0);
				SendDlgItemMessage(CurDlg, IDC_RADIO1, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM) 0);
				SendDlgItemMessage(CurDlg, IDC_RADIO2, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM) 0);
				break;
		}
		if (myState == MYSTATE_IDLE) {
			EnableInputWindows(TRUE);
			SetDlgItemText(CurDlg, IDC_TCPSTART, MSG_START1);
			SetDlgItemText(CurDlg, IDC_UDPSTART, MSG_START1);
		}
		else {
			EnableInputWindows(FALSE);
			SetDlgItemText(CurDlg, IDC_TCPSTART, MSG_CANCEL1);
			SetDlgItemText(CurDlg, IDC_UDPSTART, MSG_CANCEL1);
		}
		return;
	}

    SetDlgItemText(CurDlg, IDC_TCPBYTES, "");
	SetDlgItemText(CurDlg, IDC_UDPSPEED, "");
	SetDlgItemText(CurDlg, IDC_TCPINFO, "");
	SetDlgItemText(CurDlg, IDC_UDPINFO, "");

	SetDlgItemText(CurDlg, IDC_SERVERNAME, "");
	if (strlen(selectedServer) > 0)
		ServerSelected();

	if (myState != MYSTATE_IDLE) {

		SetDlgItemText(CurDlg, IDC_SERVERNAME, engp->hostName);

		switch (engp->tpMode) {
		case M_TCP_SEND:
			SetDlgItemText(CurDlg, IDC_TCPBYTES, UInt32ToString( (engp->tcpBytes) ));
			SetDlgItemText(CurDlg, IDC_TCPINFO, MSG_SENDING1);
			SetDlgItemText(CurDlg, IDC_DISPLAY, DISPLAYINFO1);
			break;
		case M_TCP_RECV:
			SetDlgItemText(CurDlg, IDC_TCPBYTES, UInt32ToString( (engp->tcpBytes) ));
			SetDlgItemText(CurDlg, IDC_TCPINFO, MSG_RECEIVING1);
			SetDlgItemText(CurDlg, IDC_DISPLAY, DISPLAYINFO2);
			break;
		case M_UDP_SEND:
			SetDlgItemText(CurDlg, IDC_UDPINFO, MSG_TESTSENDSPEED1);
			SetDlgItemText(CurDlg, IDC_UDPSPEED, Int32ToString( (INT)(engp->bitsPerSecond) ));
			SetDlgItemText(CurDlg, IDC_DISPLAY, DISPLAYINFO3);
			break;
		case M_UDP_RECV:
			SetDlgItemText(CurDlg, IDC_UDPINFO, MSG_TESTRCVSPEED1);
			SetDlgItemText(CurDlg, IDC_UDPSPEED, Int32ToString( (INT)(engp->bitsPerSecond) ));
			SetDlgItemText(CurDlg, IDC_DISPLAY, DISPLAYINFO4);
			break;
		case M_NAME_LOOKUP:
			{
				char str[200];
				sprintf(str, "%s ", MSG_LOOKUP1);
				strcat(str, engp->hostName);
				SetDlgItemText(CurDlg, IDC_DISPLAY, str);
			}
			break;
		case M_QUERY_MASTER:
			{
				char str[200];
				sprintf(str, "%s ", MSG_MQUERY);
				strcat(str, engp->hostName);
				SetDlgItemText(CurDlg, IDC_DISPLAY, str);
			}
			break;
				
		}
	}
	if (myState == MYSTATE_IDLE) {
		EnableInputWindows(TRUE);
		SetDlgItemText(CurDlg, IDC_STANDARDTEST, MSG_STANDARDTEST);
		SetDlgItemText(CurDlg, IDC_START, MSG_START1);
		SetDlgItemText(CurDlg, IDC_DISPLAY, DISPLAYINFO);

	}
	else {
		EnableInputWindows(FALSE);
		SetDlgItemText(CurDlg, IDC_START, MSG_CANCEL1);
		SetDlgItemText(CurDlg, IDC_STANDARDTEST, MSG_CANCEL2);
	}

	switch (selectedMode)
	{
		case CLM_AUTO:
			SendDlgItemMessage(CurDlg, IDC_RADIO1, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM) 0);
			SendDlgItemMessage(CurDlg, IDC_RADIO2, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM) 0);
			SendDlgItemMessage(CurDlg, IDC_RADIO3, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM) 0);
			break;
		case CLM_AUTO_SEND:
			SendDlgItemMessage(CurDlg, IDC_RADIO2, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM) 0);
			SendDlgItemMessage(CurDlg, IDC_RADIO1, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM) 0);
			SendDlgItemMessage(CurDlg, IDC_RADIO3, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM) 0);
			break;
		case CLM_AUTO_RECV:
			SendDlgItemMessage(CurDlg, IDC_RADIO3, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM) 0);
			SendDlgItemMessage(CurDlg, IDC_RADIO1, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM) 0);
			SendDlgItemMessage(CurDlg, IDC_RADIO2, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM) 0);
			break;
	}
}


void EnableInputWindows(bool Enable) {
	HWND hwnd;
	hwnd = GetDlgItem(CurDlg, IDC_SERVERLIST);
	EnableWindow(hwnd, Enable);
	hwnd = GetDlgItem(CurDlg, IDC_RADIO1);
	EnableWindow(hwnd, Enable);
	hwnd = GetDlgItem(CurDlg, IDC_RADIO2);
	EnableWindow(hwnd, Enable);
	hwnd = GetDlgItem(CurDlg, IDC_RADIO3);
	EnableWindow(hwnd, Enable);
	if (Advanced) {
		hwnd = GetDlgItem(CurDlg, IDC_SERVERNAME);
		EnableWindow(hwnd, Enable);
		hwnd = GetDlgItem(CurDlg, IDC_SERVERPORT);
		EnableWindow(hwnd, Enable);
		hwnd = GetDlgItem(CurDlg, IDC_REFRESH);
		EnableWindow(hwnd, Enable);
		hwnd = GetDlgItem(CurDlg, IDC_MASTER);
		EnableWindow(hwnd, Enable);
		hwnd = GetDlgItem(CurDlg, IDC_MASTERPORT);
		EnableWindow(hwnd, Enable);
		hwnd = GetDlgItem(CurDlg, IDC_TIME);
		EnableWindow(hwnd, Enable);	
		hwnd = GetDlgItem(CurDlg, IDC_TCPBYTES);
		EnableWindow(hwnd, Enable);	
		hwnd = GetDlgItem(CurDlg, IDC_UDPPPS);
		EnableWindow(hwnd, Enable);	
		hwnd = GetDlgItem(CurDlg, IDC_UDPBITRATE);
		EnableWindow(hwnd, Enable);	
		hwnd = GetDlgItem(CurDlg, IDC_UDPSIZE);
		EnableWindow(hwnd, Enable);
	}
}



void DisplayCSH(LPPOINT pt) {
	static POINT oldpt = {0,0};
	int pos;
	if (Advanced)
		return;
	if (pt == NULL) {
		if (now_displaying != 0) {
			SetDlgItemText(CurDlg, IDC_DISPLAY, DISPLAYINFO);
			now_displaying = 0;
		}
		return;
	}
	if (pt->x == oldpt.x && pt->y == oldpt.y)
		return;
	oldpt.x = pt->x;
	oldpt.y = pt->y;
	for (pos = 0; CSHCoordinates[pos*5] != 0; pos++) {
		if (pt->x >= CSHCoordinates[pos*5] && pt->x <= CSHCoordinates[pos*5+1] &&
			pt->y >= CSHCoordinates[pos*5+2] && pt->y <= CSHCoordinates[pos*5+3])
		{
			if (CSHCoordinates[pos*5+4] != now_displaying) {
				now_displaying = CSHCoordinates[pos*5+4];
				SetDlgItemText(CurDlg, IDC_DISPLAY, CSHText[now_displaying]);
			}
			return;
		}
	}
	if (now_displaying != 0) {
		SetDlgItemText(CurDlg, IDC_DISPLAY, DISPLAYINFO);
		now_displaying = 0;
	}
}


int do_nlookup(TPEngine * engp) {
	WSInfo * w;
	w = (WSInfo *)(engp->ctrlRefp);
	if (w == 0) return -1;

	if (engp->state == CLSM_NAMELOOKUP) {
		Sleep(100);
		RunClientContext(engp);
		if (engp->state == CLSM_IDLE) {
			if (engp->numHostIP == 0) 
				return NOSUCHHOST;
		}
	}
	else {
		engp->tpMode = CLM_NAME_LOOKUP;
        SetTimeLimit(engp, 30);
		StartClientContext(engp);
		if (engp->state == CLSM_NAMELOOKUP) 
			UpdateTesting(engp);
		else
			return LOOKUPINITFAIL;
	}
	return 0;
}

int get_serverlist(TPEngine * engp) {
	if (engp->active != 0) {
		RunClientContext(engp);
		if (engp->state == CLSM_FAILED) {
			return QUERYMASTERFAIL;
		}
	}
	else {
		engp->tpMode = CLM_QUERY_MASTER;
        engp->sessionMaxTime = 30;
		StartClientContext(engp);
		if (engp->active == 0) 
			return QUERYMASTERINITFAIL;
	}
	return 0;
}

void DoError(char * str) {
	char str2[300], str3[30];
	sprintf(str2, "%s ", str);
	sprintf(str3, "Error %d", engp->failCode);
	switch (engp->failCode) {
	case TPER_TIMEOUT:
		strcat(str2, "- test timed out");
		break;
	case TPER_SERVERBUSY:
		strcat(str2, "- server busy. Try again later");
		break;
	case TPER_SERVERDENY:
		strcat(str2, "- server refused us. Try another server");
		break;
	case TPER_CONNECTFAIL:
		strcat(str2, "- server is not running. Try another server");
		break;
	case TPER_MASTERBUSY:
		strcat(str2, "- master server busy. Try again later");
		break;
	case TPER_MASTERDENIED:
		strcat(str2, "- master server refused us");
		break;
	case TPER_USERABORT:
		strcat(str2, "- user abort");
		break;
	case TPER_NATFAIL:
		strcat(str2, "- NATOPEN failed. Do you have a firewall?");
		break;
	case TPER_SRVABORT:
		strcat(str2, "- server aborted test");
		break;
	case TPER_CTRLCLOSED:
		strcat(str2, "- server closed control connection");
		break;
	default:
		strcat(str2, " (last msg: ");
		strcat(str2, engp->ctrlMessage);
		strcat(str2, ")");
		break;
	}
	substate = 0;
	myState = MYSTATE_IDLE;
	UpdateTesting(engp);
	MessageBox(CurDlg, str2, str3, MB_OK);
}

// Main test function called repeatedly by WinMain()
void tptest()
{
	int res, newmode;
	switch (myState) {
	case MYSTATE_GETSERVERLIST:
		switch (substate) {
		case 0:
			res = do_nlookup(engp);
			if (res == 0) {
				if (engp->state == CLSM_COMPLETE || engp->state == CLSM_FAILED) {
					if (engp->numHostIP > 0) {
						engp->hostIP.s_addr = engp->hostIPTab[0].s_addr;
						substate = 1;
					}
					else {
						DoError("Master server not found");
					}
				}
				break;
			}
			if (res == NOSUCHHOST)
				MessageBox(CurDlg, "Master server address wrong", "Error", MB_OK);
			if (res == LOOKUPINITFAIL)
				MessageBox(CurDlg, "Name lookup init failed", "Error", MB_OK);
			substate = 0;
			myState = MYSTATE_IDLE;
			UpdateTesting(engp);
			break;
		case 1:
			res = get_serverlist(engp);
			if (res == 0) {
				if (engp->active != 0) {
					break;
				}
				else {
					if (engp->numServers == 0) {
						if (gotServerList) {
						}
						else {
							DoError("Failed to get server list");
							break;
						}
					}
					else {
						UpdateServerList();
						ServerSelected();
						gotServerList = 1;
					}
				}
			}
			else {
				MessageBox(CurDlg, "Get serverlist failed", "Error", MB_OK);
			}
			substate = 0;
			myState = MYSTATE_IDLE;
			UpdateTesting(engp);
			break;
		}
		break;

	case MYSTATE_STDTEST:
		switch (substate) {
		case 0:
			res = do_nlookup(engp);
			if (res == 0) {
				if (engp->state == CLSM_COMPLETE || engp->state == CLSM_FAILED) {
					if (engp->numHostIP > 0) {
						engp->hostIP.s_addr = engp->hostIPTab[0].s_addr;
						engp->wipeFlag = 1;
						substate = 1;
					}
					else {
						DoError("Master server not found");
					}
				}
				break;
			}
			if (res == NOSUCHHOST)
				MessageBox(CurDlg, "Master server address wrong", "Error", MB_OK);
			if (res == LOOKUPINITFAIL)
				MessageBox(CurDlg, "Name lookup init failed", "Error", MB_OK);
			if (gotServerList != 0) substate = 2;
			else {
				substate = 0;
				myState = MYSTATE_IDLE;
			}
			break;
			// do master server name lookup
			// if fail and we have old list, go to 2
		case 1:
			res = get_serverlist(engp);
			if (res == 0) {
				if (engp->active != 0) {
					break;
				}
				else {
					if (engp->numServers == 0) {
						if (gotServerList) {
						}
						else {
							DoError("Failed to get server list");
							break;
						}
					}
					else {
						UpdateServerList();
						ServerSelected();
						gotServerList = 1;
					}
				}
			}
			else {
				DoError("Get serverlist failed");
				break;
			}
			strcpy(engp->hostName, engp->serverNameList[0]);
			engp->hostCtrlPort = engp->serverPortList[0];
			substate = 2;
			break;
			// get server list from master
		case 2:
			// Lookup first server's IP
			res = do_nlookup(engp);
			if (res == 0) {
				if (engp->active == 0) {
					if (engp->numHostIP > 0) {
						engp->hostIP.s_addr = engp->hostIPTab[0].s_addr;
						engp->tpMode = CLM_NONE;
						substate = 3;
					}
					else {
						DoError("Server not found");
					}
				}
				break;
			}
			if (res == NOSUCHHOST)
				MessageBox(CurDlg, "Server address wrong", "Error", MB_OK);
			if (res == LOOKUPINITFAIL)
				MessageBox(CurDlg, "Name lookup init failed", "Error", MB_OK);
			substate = 0;
			myState = MYSTATE_IDLE;
			break;
		case 3:
			newmode = AdvanceTest(engp, selectedMode, engp->tpMode, 0);
			UpdateTesting(engp);
			UpdateBestRate(engp);
			if (newmode != CLM_NONE) {
				engp->tpMode = newmode;
				if (StartClientContext(engp) != 0) {
					DoError("Startclientcontext failed");
				}
				else {
					InvalidateRect(ProgressBar, NULL, TRUE);
					Progress_X = 0;
					substate = 4;
				}
			}
			else {
				UpdateLoadResults();
				ReportStats();
				substate = 0;
				myState = MYSTATE_IDLE;
				MessageBox(CurDlg, "Test finished!", "Success", MB_OK);
			}
			UpdateTesting(engp);
			break;
		case 4:
			RunClientContext(engp);
			update_progressbar();
			if (engp->state == CLSM_FAILED) {
				DoError("Test failed");
			}
			else if (engp->state == CLSM_COMPLETE) {
				fill_progressbar();
				Sleep(300);
				substate = 3;
				UpdateTesting(engp);
			}
			break;
		}
		break;

	case MYSTATE_AUTOTEST:
		switch (substate) {
		case 0:
			// Lookup first server's IP
			res = do_nlookup(engp);
			if (res == 0) {
				if (engp->active == 0) {
					if (engp->numHostIP > 0) {
						engp->hostIP.s_addr = engp->hostIPTab[0].s_addr;
                        engp->hostCtrlPort = selectedServerPort;
						engp->tpMode = CLM_NONE;
						substate = 1;
					}
					else {
						DoError("Server not found");
					}
				}
				break;
			}
			if (res == NOSUCHHOST)
				MessageBox(CurDlg, "Server address wrong", "Error", MB_OK);
			if (res == LOOKUPINITFAIL)
				MessageBox(CurDlg, "Name lookup init failed", "Error", MB_OK);
			substate = 0;
			myState = MYSTATE_IDLE;
			break;
		case 1:
			newmode = AdvanceTest(engp, selectedMode, engp->tpMode, 0);
			UpdateTesting(engp);
			UpdateBestRate(engp);
			if (newmode != CLM_NONE) {
				engp->tpMode = newmode;
				if (StartClientContext(engp) != 0) {
					DoError("Startclientcontext failed");
				}
				else {
					InvalidateRect(ProgressBar, NULL, TRUE);
					Progress_X = 0;
					substate = 2;
				}
			}
			else {
				UpdateLoadResults();
				ReportStats();
				substate = 0;
				myState = MYSTATE_IDLE;
				MessageBox(CurDlg, "Test finished!", "Success", MB_OK);
			}
			UpdateTesting(engp);
			break;
		case 2:
			RunClientContext(engp);
			update_progressbar();
			if (engp->state == CLSM_FAILED) {
				DoError("Test failed");
			}
			else if (engp->state == CLSM_COMPLETE) {
				fill_progressbar();
				substate = 1;
			}
			break;
		}
		// run M_AUTO_xxxx test
		// set myState = MYSTATE_IDLE when finished
		break;

	case MYSTATE_SINGLETEST:
		switch (substate) {
		case 0:
			// Lookup server's IP
			res = do_nlookup(engp);
			if (res == 0) {
				if (engp->active == 0) {
					if (engp->numHostIP > 0) {
						engp->hostIP.s_addr = engp->hostIPTab[0].s_addr;
						engp->tpMode = CLM_NONE;
						substate = 1;
					}
					else {
						DoError("Server not found");
					}
				}
				break;
			}
			if (res == NOSUCHHOST)
				MessageBox(CurDlg, "Server address wrong", "Error", MB_OK);
			if (res == LOOKUPINITFAIL)
				MessageBox(CurDlg, "Name lookup init failed", "Error", MB_OK);
			substate = 0;
			myState = MYSTATE_IDLE;
			break;
		case 1:
			newmode = AdvanceTest(engp, selectedMode, engp->tpMode, 0);
			UpdateTesting(engp);
			UpdateBestRate(engp);
			if (newmode != CLM_NONE) {
				engp->tpMode = newmode;
				if (StartClientContext(engp) != 0) {
					DoError("Startclientcontext failed");
				}
				else {
					InvalidateRect(ProgressBar, NULL, TRUE);
					Progress_X = 0;
					substate = 2;
				}
			}
			else {
				UpdateLoadResults();
				ReportStats();
				substate = 0;
				myState = MYSTATE_IDLE;
				MessageBox(CurDlg, "Test finished!", "Success", MB_OK);
			}
			UpdateTesting(engp);
			break;
		case 2:
			RunClientContext(engp);
			update_progressbar();
			if (engp->state == CLSM_FAILED) {
				DoError("Test failed");
			}
			else if (engp->state == CLSM_COMPLETE) {
				fill_progressbar();
				substate = 1;
			}
			break;
		}
		// run M_AUTO_xxxx test
		// set myState = MYSTATE_IDLE when finished
		break;

	case MYSTATE_IDLE:
		/*
		int GetMouseMovePoints(
		UINT cbSize                // size of the MOUSEMOVEPOINT struct
		LPMOUSEMOVEPOINT lppt,     // pointer to current mouse move point
		LPMOUSEMOVEPOINT lpptBuf,  // buffer to store the points
		int nBufPoints,            // how many points the buffer can store
		DWORD resolution           // resolution of the points
		);

		GetMouseMovePoints(sizeof(mps), &oldmmp, &mmp, 1, GMMP_USE_DISPLAY_POINTS);
		if (mmp.x != oldmmp.x || mmp.y != oldmmp.y) {
			GetCursorPos(&mousepos);
			lastmousemove = time(NULL);
			DisplayCSH(NULL);
		}
		if (time(NULL) > lastmousemove+1)
			DisplayCSH(&mousepos);
		break;
		*/
		if (!Advanced)
		{
			INT64 ttmp;
			GetCursorPos(&mousepos);
			mousepos.x -= windowplacement.rcNormalPosition.left;
			mousepos.y -= windowplacement.rcNormalPosition.top;

			/*
			sprintf(tmp, "%d,%d", mousepos.x, mousepos.y);
			SetDlgItemText(CurDlg, IDC_DISPLAY, tmp);
			*/

			if (mousepos.x != oldmousepos.x || mousepos.y != oldmousepos.y) {
					oldmousepos.x = mousepos.x;
					oldmousepos.y = mousepos.y;
					QueryPerformanceCounter((LARGE_INTEGER *)&lastmousemove);
					DisplayCSH(NULL);
					WaitMsg = FALSE;
					return;
			}
			QueryPerformanceCounter((LARGE_INTEGER *)&ttmp);
			if (ttmp > (lastmousemove + ((perf_freq * CSHWAIT)/1000))) {
				DisplayCSH(&mousepos);
				WaitMsg = TRUE;
			}
		}
		else
			WaitMsg = TRUE;
		break;
	}
}


// Start a new test session. Init variables, sockets, etc
// Connect to server and exchange session information
int InitSession(TPEngine * engp)
{

		// Clear the progress bar area
		Progress_X = 0;
		InvalidateRect(ProgressBar, NULL, TRUE);

		ClearTextWindow();
		return 0;
}



void fill_progressbar()
{
	RECT rect;
	HDC hdc;
	rect.bottom = 15;
	rect.left = 1;
	rect.right = 575;
	rect.top = 1;
	hdc = GetDC(ProgressBar);
	FillRect(hdc, &rect, progress_brush);
	ReleaseDC(ProgressBar, hdc);
}


// Move progress bar
void update_progressbar()
{
	int new_x = 0;
	switch (engp->tpMode) {
		case M_UDP_SEND:
			if (engp->nPackets > 0)
				new_x = (engp->curSend * 580) / engp->nPackets;
			break;
		case M_UDP_RECV:
		case M_UDP_FDX:
			if (engp->nPackets > 0)
				new_x = (engp->stats.PktsRecvd * 580) / engp->nPackets;
			break;
		case M_TCP_SEND:
			if (engp->tcpBytes > 0)
				new_x = (int)((engp->stats.BytesSent * 580) / engp->tcpBytes);
			break;
		case M_TCP_RECV:
			if (engp->tcpBytes > 0)
                new_x = (int)((engp->stats.BytesRecvd * 580) / engp->tcpBytes);
			break;
	}
	if (new_x > 580)
		new_x = 580;
	// Avoid updating it for every sent packet if we send a lot of packets per second
	if (new_x > Progress_X)
	{
		RECT rect;
		HDC hdc;
		rect.bottom = 15;
		rect.left = 1;
		rect.right = Progress_X = new_x;
		rect.top = 1;
		hdc = GetDC(ProgressBar);
		FillRect(hdc, &rect, progress_brush);
		ReleaseDC(ProgressBar, hdc);
	}
}



// Store a single line in the outtext linked list and update the text
// window so the new line is shown
void Report(char *s)
{
	static int lines = 0;
	struct outtext_struct *o;
	int len;
	if (s == NULL)
		return;
	len = (int)strlen(s);
	if (len > 97)
		len = 97;
	if (outtext == NULL) {
		outtext = (struct outtext_struct *)malloc(sizeof(struct outtext_struct));
		if (outtext == NULL) {
	  		MessageBox(CurDlg, "malloc() failed, out of memory!", TPERROR, MB_OK);
			exit(1);
		}
		outtext->next = NULL;
		lines = 1;
		o = outtext;
	}
	else {
		o = outtext;
		while (o->next != NULL)
			o = o->next;
		o->next = (struct outtext_struct *)malloc(sizeof(struct outtext_struct));
		if (o->next == NULL) {
	  		MessageBox(CurDlg, "malloc() failed, out of memory!", TPERROR, MB_OK);
			exit(1);
		}
		o = o->next;
		o->next = NULL;
		lines++;
	}
	sprintf(o->line, "%s\r\n", s);
	if (lines > 100) {
		o = outtext;
		outtext = o->next;
		free(o);
		lines--;
	}
#ifdef REPORT
	if (Advanced)
		UpdateTextWindow();
#endif
}

// Concatenate all the lines in the outtext linked list and output
// them into the text window.
void UpdateTextWindow()
{
	char tmp[11000];
	struct outtext_struct *o = outtext;
	memset(tmp, 0, 11000);
	while (o != NULL)
	{
		strcat(tmp, o->line);
		o = o->next;
	}
	SendMessage(TextWindow, WM_SETTEXT, (WPARAM)0, (LPARAM)tmp);
}

// Clears the text window and frees the linked list holding the text lines
//
void ClearTextWindow()
{
	struct outtext_struct *o, *o2;
	if ((o = outtext) != NULL)
	{
		while ((o2 = o->next) != NULL)
		{
			free(o);
			o = o2;
		}
		free(o);
		outtext = NULL;
	}
#ifdef REPORT
		SendMessage(TextWindow, WM_SETTEXT, (WPARAM)0, (LPARAM)"");
#endif
}

// A lot of the old TPTEST report-statistics code, with a few changes
//
void ReportStats()
{
	char str[200], str2[200];
	  int Hours, Minutes, Seconds, msSend, msRcv;
  double BytesPerSecondSend, BytesPerSecondRcv;
  static double STBytesPerSecond = 0;
  time_t stopTime;
  char tBuf1[ 256 ];
  struct tm *tmPnt;

  time(&stopTime);
	ClearTextWindow();

  msSend = ( engp->stats.StopSend.tv_sec - engp->stats.StartSend.tv_sec ) * 1000;
  msSend += ( engp->stats.StopSend.tv_usec - engp->stats.StartSend.tv_usec ) / 1000;
  if( msSend != 0 )
    BytesPerSecondSend = ( (double)(engp->stats.BytesSent) * 1000.0 )
							/ (double)(msSend);
  else
    BytesPerSecondSend = 0.0;

  msRcv = ( engp->stats.StopRecv.tv_sec - engp->stats.StartRecv.tv_sec ) * 1000;
  msRcv += ( engp->stats.StopRecv.tv_usec - engp->stats.StartRecv.tv_usec ) / 1000;
  if( msRcv != 0 )
    BytesPerSecondRcv = ( (double)(engp->stats.BytesRecvd) * 1000.0 )
							/ (double)(msRcv);
  else
    BytesPerSecondRcv = 0.0;

	sprintf(str, "Throughput test results:");
	Report(str);
	Report("");

	switch (engp->tpMode) {
		case M_TCP_SEND:
		case M_UDP_SEND:
		case M_UDP_FDX:
			if (gethostname( tBuf1, sizeof tBuf1 ) == SOCKET_ERROR)
				sprintf(str, "Source machine :         %s",
					inet_ntoa(engp->myLocalAddress));
			else
				sprintf(str, "Source machine :         %s (%s)", tBuf1,
					inet_ntoa(engp->myLocalAddress));
			sprintf(str2, "Destination machine :    %s (%s)",
					engp->hostName, 
					inet_ntoa( *((struct in_addr *)&engp->hostIP) ) );
			break;
		case M_TCP_RECV:
		case M_UDP_RECV:
			if (gethostname( tBuf1, sizeof tBuf1 ) == SOCKET_ERROR)
				sprintf(str2, "Destination machine :    %s",
					inet_ntoa(engp->myLocalAddress));
			else
				sprintf(str2, "Destination machine :    %s (%s)", tBuf1,
					inet_ntoa(engp->myLocalAddress));
			sprintf(str, "Source machine :         %s (%s)",
					engp->hostName, 
					inet_ntoa( *((struct in_addr *)&engp->hostIP) ) );
			break;
		default:
			sprintf(str, "Source machine :         Unknown");
			sprintf(str2, "Destination machine :    Unknown");
	}
	Report(str);
	Report(str2);

	switch (selectedMode) {
	case CLM_AUTO:
		sprintf(str, "Type of test :           Send & Receive");
		break;
	case CLM_AUTO_SEND:
		sprintf(str, "Type of test :           Send");
		break;
	case CLM_AUTO_RECV:
		sprintf(str, "Type of test :           Receive");
		break;
	}
	Report(str);

#ifdef USE_GMTIME
  tmPnt = gmtime( (time_t *)(&engp->startTime.tv_sec) );
  sprintf(str, "Test started :           %04d-%02d-%02d %02d:%02d:%02d.%03ld GMT",
	  tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
	  tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec,
	  engp->startTime.tv_usec / 1000L );
  Report(str);
  tmPnt = gmtime( (time_t *)(&stopTime.tv_sec) );
  sprintf(str, "Test ended :             %04d-%02d-%02d %02d:%02d:%02d.%03ld GMT",
	  tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
	  tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec,
	  stopTime.tv_usec / 1000L );
  Report(str);
#else
  tmPnt = localtime( (time_t *)(&engp->startTime) );
  sprintf(str, "Test started :           %04d-%02d-%02d %02d:%02d:%02d",
	  tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
	  tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec );
  Report(str);
  tmPnt = localtime( (time_t *)(&stopTime) );
  sprintf(str, "Test ended :             %04d-%02d-%02d %02d:%02d:%02d",
	  tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
	  tmPnt->tm_hour, tmPnt->tm_min, tmPnt->tm_sec );
  Report(str);
#endif

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

  switch (selectedMode) {
	  case M_UDP_RECV:
	  case M_UDP_SEND:
	  case M_UDP_FDX:
		  sprintf(str, "  Packets sent :         %d", engp->stats.PktsSent );
		  Report(str);
		  sprintf(str, "  Unable to send :       %d", engp->stats.PktsUnSent );
		  Report(str);
		  break;
  }

#ifdef NO_QUAD_PRINTF
  sprintf(str, "  Bytes sent :           %.0f  (%s)",
	  (double)(engp->stats.BytesSent), UInt32ToString( (UINT32) engp->stats.BytesSent ) );
#else
  sprintf(str, "  Bytes sent :           %qd  (%s)",
	  engp->stats.BytesSent, UInt32ToString( engp->stats.BytesSent ) );
#endif
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
	  case M_UDP_SEND:
	  case M_UDP_RECV:
	  case M_UDP_FDX:
		sprintf(str, "  Packets received :     %d", engp->stats.PktsRecvd );
		Report(str);
		sprintf(str, "  Out-of-order packets : %d", engp->stats.ooCount );
		Report(str);
		break;
  }

#ifdef NO_QUAD_PRINTF
  sprintf(str, "  Bytes received :       %.0f  (%s)",
	  (double)(engp->stats.BytesRecvd), UInt32ToString( (UINT32)engp->stats.BytesRecvd ) );
#else
  sprintf(str, "  Bytes received :       %d  (%s)",
	  engp->stats.BytesRecvd, UInt32ToString( (UINT32)engp->stats.BytesRcvd ) );
#endif
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
		if( engp->stats.PktsSent != 0 ) {
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
		}
		else {
			sprintf(str, "  Packets lost on net :  %d", engp->stats.PktsSent - engp->stats.PktsRecvd );
			Report(str);
			sprintf(str, "  Unable to send :       %d", engp->stats.PktsUnSent );
			Report(str);
			sprintf(str, "  Total packets lost :   %d",
				( engp->stats.PktsSent - engp->stats.PktsRecvd ) + engp->stats.PktsUnSent );
			Report(str);
		}
  }

  if( engp->tpMode == M_UDP_FDX ) {
	if( engp->stats.nRoundtrips != 0 ) {
		sprintf(str, "\r\nRoundtrip statistics" );
		Report(str);
		sprintf(str, "  Min roundtrip delay :  %d.%03d ms",
			engp->stats.MinRoundtrip / 1000, engp->stats.MinRoundtrip % 1000 );
		Report(str);
		sprintf(str, "  Max roundtrip delay :  %d.%03d ms",
			engp->stats.MaxRoundtrip / 1000, engp->stats.MaxRoundtrip % 1000 );
		Report(str);
#ifdef NO_QUAD_PRINTF
		sprintf(str, "  Avg roundtrip delay :  %.0f.%03.0f ms",
			(double)(( engp->stats.TotalRoundtrip / engp->stats.nRoundtrips ) / 1000),
			(double)(( engp->stats.TotalRoundtrip / engp->stats.nRoundtrips ) % 1000) );
#else
		sprintf(str, "  Avg roundtrip delay :  %qd.%03qd ms",
			( engp->stats.TotalRoundtrip / engp->stats.nRoundtrips ) / 1000,
			( engp->stats.TotalRoundtrip / engp->stats.nRoundtrips ) % 1000 );
#endif
		Report(str);
	}
  }
  Report("");
}



// Remove CRLF from the end of a string
void stripcrlf(char *s)
{
	char *p;
	p = s + (strlen(s) - 1);
	if (*p == 13 ||  *p == 10)
	{
		*p = '\0';
		stripcrlf(s);
	}
}



LRESULT CALLBACK AdvancedDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId;

	switch (message)
	{
	case WM_INITDIALOG:
		Advanced = TRUE;
		if (firstwindow)
		{
			GetWindowPlacement(hDlg, &windowplacement);
			windowplacement.showCmd = SW_SHOWNORMAL;
			firstwindow = false;
		}
		TextWindow = GetDlgItem(hDlg, IDC_DISPLAY);
        ClearTextWindow();
		UpdateTextWindow();
		CurDlg = hDlg;
		ProgressBar = GetDlgItem(hDlg, IDC_PROGRESS);

		engp->bitsPerSecond = DEFAULT_BITRATE;
		engp->tcpBytes = DEFAULT_TCPBYTES;
		engp->sessionTime = DEFAULT_TESTTIME;
		RecalculatePPSSZ(engp);

		// Set text in edit boxes
		SetDlgItemText(hDlg, IDC_MASTER, SelectedMaster);
		SetDlgItemInt(hDlg, IDC_MASTERPORT, SelectedMasterPort, FALSE);
		if (selectedServerPort != 0) {
			SetDlgItemText(hDlg, IDC_SERVERNAME, selectedServerAdr);
			SetDlgItemInt(hDlg, IDC_SERVERPORT, selectedServerPort, FALSE);
		}
		selectedMode = M_TCP_RECV;
		UpdateTesting(engp);
		SetDlgItemInt(hDlg, IDC_TIME, engp->sessionTime, FALSE);
		SetDlgItemInt(hDlg, IDC_TCPBYTES, engp->tcpBytes, TRUE);
		SetDlgItemInt(hDlg, IDC_UDPPPS, engp->packetsPerSecond, FALSE);
		SetDlgItemInt(hDlg, IDC_UDPSIZE, engp->packetSize, FALSE);
		SetDlgItemInt(hDlg, IDC_UDPBITRATE, engp->bitsPerSecond, FALSE);
		UpdateServerList();
		SetWindowPlacement(hDlg, &windowplacement);

		WaitMsg = TRUE;
		return TRUE;
	case WM_COMMAND:
		WaitMsg = FALSE;
		wmId = LOWORD(wParam);
		switch (wmId)
		{
			case IDM_ABOUT:
			   DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			   break;
			case IDM_INFO:
				MessageBox(hDlg, TPTEST_INFO, "TPTEST Information", MB_OK);
				break;
			case IDM_HJALP:
				DialogBox(hInst, (LPCTSTR)IDD_DIALOG5, hWnd, (DLGPROC)Help);
				break;
			case IDM_EXIT:
			case IDCANCEL:
			   DestroyWindow(hWnd);
			   DeleteContext(engp);
			   exit(0);
			case IDM_PRINTREPORT:
				// Display the PRINT dialog box. 
				PrintReport(); 
				return TRUE;
			case IDC_RADIO1: 
				selectedMode = M_TCP_SEND;
				UpdateTesting(engp);
				return TRUE;
			case IDC_RADIO2:
				selectedMode = M_TCP_RECV;
				UpdateTesting(engp);
				return TRUE;
			case IDC_RADIO3:
				selectedMode = M_UDP_FDX;
				UpdateTesting(engp);
				return TRUE;
			case IDC_TCPSTART:
				if (myState == MYSTATE_IDLE) {
					unsigned int x = 0;
					ClearTextWindow();
					memset(engp->hostName, 0, MAX_SERVER_NAME);
					GetDlgItemText(hDlg, IDC_SERVERNAME, engp->hostName, MAX_SERVER_NAME);
					if (strlen(engp->hostName) < 1) {
						MessageBox(hDlg, "No server selected!", TPERROR, MB_OK);
						return TRUE;
					}
					engp->hostCtrlPort = GetDlgItemInt(hDlg, IDC_SERVERPORT, NULL, FALSE);
					if (engp->hostCtrlPort == 0) {
						MessageBox(hDlg, "Invalid server port selected!", TPERROR, MB_OK);
						return TRUE;
					}
					x = GetDlgItemInt(hDlg, IDC_TIME, NULL, FALSE);
					if (x == 0) {
						MessageBox(hDlg, "You must supply a test time!", TPERROR, MB_OK);
						return TRUE;
					}
					engp->sessionMaxTime = x;
					switch (selectedMode) {
						case M_UDP_FDX:
							MessageBox(hDlg, "Only Send and Receive modes are available with TCP", 
								TPERROR, MB_OK);
							return TRUE;
						case M_UDP_RECV:
							selectedMode = M_TCP_RECV;
							break;
						case M_UDP_SEND:
							selectedMode = M_TCP_SEND;
							break;
					}
					x = 0;
					x = GetDlgItemInt(hDlg, IDC_TCPBYTES, NULL, FALSE);
					if (x == 0) {
						MessageBox(hDlg, "You must enter # of bytes to transmit!", TPERROR, MB_OK);
						return TRUE;
					}
					engp->tcpBytes = x;
					engp->wipeFlag = 1;
					myState = MYSTATE_SINGLETEST;
					substate = 0;
				}
				else {
					StopContext(engp);
					myState = MYSTATE_IDLE;
					substate = 0;
					UpdateTesting(engp);
					MessageBox(CurDlg, "Test interrupted!", "Info", MB_OK);
				}
				return TRUE;
			case IDC_UDPSTART:
				if (myState == MYSTATE_IDLE) {
					unsigned int x = 0;
					ClearTextWindow();
					memset(engp->hostName, 0, TEXTBUF);
					GetDlgItemText(hDlg, IDC_SERVERNAME, engp->hostName, 99);
					if (strlen(engp->hostName) < 1) {
						MessageBox(hDlg, "No server selected!", TPERROR, MB_OK);
						return TRUE;
					}
					engp->hostCtrlPort = GetDlgItemInt(hDlg, IDC_SERVERPORT, NULL, FALSE);
					if (engp->hostCtrlPort == 0) {
						MessageBox(hDlg, "Invalid server port selected!", TPERROR, MB_OK);
						return TRUE;
					}
					if (strlen(engp->hostName) == 0) {
						MessageBox(hDlg, "No server selected!", TPERROR, MB_OK);
						return TRUE;
					}
					x = GetDlgItemInt(hDlg, IDC_TIME, NULL, FALSE);
					if (x == 0) {
						MessageBox(hDlg, "You must supply a test time!", TPERROR, MB_OK);
						return TRUE;
					}
					engp->sessionTime = x;
					engp->sessionMaxTime = x * 2;
					switch (selectedMode) {
						case M_TCP_RECV:
							selectedMode = M_UDP_RECV;
							break;
						case M_TCP_SEND:
							selectedMode = M_UDP_SEND;
							break;
					}
					engp->packetsPerSecond = GetDlgItemInt(hDlg, IDC_UDPPPS, NULL, FALSE);
					engp->packetSize = GetDlgItemInt(hDlg, IDC_UDPSIZE, NULL, FALSE);
					engp->nPackets = engp->sessionTime * engp->packetsPerSecond;
					engp->wipeFlag = 1;
					myState = MYSTATE_SINGLETEST;
					substate = 0;
				}
				else {
					StopContext(engp);
					myState = MYSTATE_IDLE;
					substate = 0;
					UpdateTesting(engp);
					MessageBox(CurDlg, "Test interrupted!", "Info", MB_OK);	
				}
				return TRUE;
			case IDC_UDPPPS:
				if (HIWORD(wParam) == EN_KILLFOCUS)
				{
					UINT32 newpps = (int)GetDlgItemInt(hDlg, IDC_UDPPPS, NULL, FALSE);
					if (newpps < 1) {
						MessageBox(CurDlg, "Minimum packets per second is 1, sorry", "FYI", MB_OK);
						SetDlgItemInt(hDlg, IDC_UDPPPS, engp->packetsPerSecond, FALSE);
					}
					else {
						if (newpps != engp->packetsPerSecond) {
							engp->packetsPerSecond = newpps;
							engp->bitsPerSecond = newpps * engp->packetSize * 8;
							SetDlgItemInt(hDlg, IDC_UDPBITRATE, engp->bitsPerSecond, FALSE);
						}
					}
					return TRUE;
				}
				break;
			case IDC_TIME:
				if (HIWORD(wParam) == EN_KILLFOCUS)
				{
					UINT32 newtime = (int)GetDlgItemInt(hDlg, IDC_TIME, NULL, FALSE);
					if (newtime < 1) {
						MessageBox(CurDlg, "Minimum test time is 1 second, sorry", "FYI", MB_OK);
						SetDlgItemInt(hDlg, IDC_TIME, engp->sessionTime, FALSE);
					}
					else {
						if (newtime != engp->sessionTime) {
							engp->sessionTime = newtime;
							engp->nPackets = engp->packetsPerSecond * engp->sessionTime;
						}
					}
					return TRUE;
				}
				break;
			case IDC_UDPSIZE:
				if (HIWORD(wParam) == EN_KILLFOCUS)
				{
					UINT32 newsize = (int)GetDlgItemInt(hDlg, IDC_UDPSIZE, NULL, FALSE);
					if (newsize < MIN_PKT_SIZE) {
						sprintf(tmp, "Minimum packet size is %d, sorry", MIN_PKT_SIZE);
						MessageBox(CurDlg, tmp, "FYI", MB_OK);
						SetDlgItemInt(hDlg, IDC_UDPSIZE, engp->packetSize, FALSE);
					}
					else if (newsize > MAX_PKT_SIZE) {
						sprintf(tmp, "Max packet size is %d, sorry", MAX_PKT_SIZE);
						MessageBox(CurDlg, tmp, "FYI", MB_OK);
						SetDlgItemInt(hDlg, IDC_UDPSIZE, engp->packetSize, FALSE);
					}
					else {
						if (newsize != engp->packetSize) {
							engp->packetSize = newsize;
							engp->bitsPerSecond = newsize * engp->packetsPerSecond * 8;
							SetDlgItemInt(hDlg, IDC_UDPBITRATE, engp->bitsPerSecond, FALSE);
						}
					}
					return TRUE;
				}
				break;
			case IDC_UDPBITRATE:
				if (HIWORD(wParam) == EN_KILLFOCUS)
				{
					UINT32 newrate = (int)GetDlgItemInt(hDlg, IDC_UDPBITRATE, NULL, FALSE);
					if (newrate < (MIN_PKT_SIZE * 8)) {
						sprintf(tmp, "Minimum bitrate is %d, sorry", MIN_PKT_SIZE * 8);
						MessageBox(CurDlg, tmp, "FYI", MB_OK);
						SetDlgItemInt(hDlg, IDC_UDPBITRATE, engp->bitsPerSecond, FALSE);
					}
					else {
						if (newrate != engp->bitsPerSecond) {
							engp->bitsPerSecond = newrate;
							RecalculatePPSSZ(engp);
							engp->bitsPerSecond = engp->packetSize * engp->packetsPerSecond * 8;
							SetDlgItemInt(hDlg, IDC_UDPBITRATE, engp->bitsPerSecond, FALSE);
							SetDlgItemInt(hDlg, IDC_UDPSIZE, engp->packetSize, FALSE);
							SetDlgItemInt(hDlg, IDC_UDPPPS, engp->packetsPerSecond, FALSE);
						}
					}
					return TRUE;
				}
				break;
			case IDC_SERVERLIST:
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					ServerSelected();
					return TRUE;
				}
				break;
			case IDC_MASTER:
				if (HIWORD(wParam) == EN_KILLFOCUS)
				{
					memset(SelectedMaster, 0, 100);
					GetDlgItemText(CurDlg, IDC_MASTER, SelectedMaster, 99);
					return TRUE;
				}
				break;
			case IDC_MASTERPORT:
				if (HIWORD(wParam) == EN_KILLFOCUS)
				{
					SelectedMasterPort = GetDlgItemInt(CurDlg, IDC_MASTERPORT, NULL, FALSE);
					return TRUE;
				}
				break;
			case IDC_REFRESH:
				gotServerList = 0;
				engp->wipeFlag = 1;
				engp->hostCtrlPort = GetDlgItemInt(CurDlg, IDC_MASTERPORT, NULL, FALSE);
				GetDlgItemText(CurDlg, IDC_MASTER, engp->hostName, MAX_SERVER_NAME-1);
				if (strlen(engp->hostName) < 1) {
					MessageBox(CurDlg, "No master server selected!", "Error", MB_OK);
					break;
				}
				ClearServerList(engp);
				myState = MYSTATE_GETSERVERLIST;
				substate = 0;
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	}
	return FALSE;
}
		



// Message handler for main dialog box window
LRESULT CALLBACK Standard(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId;

	switch (message)
	{
		case WM_NCHITTEST:
			WaitMsg = FALSE;
			break;
		case WM_INITDIALOG:
			if (firstwindow)
			{
				GetWindowPlacement(hDlg, &windowplacement);
				firstwindow = false;
			}
			windowplacement.showCmd = SW_SHOWNORMAL;
			Advanced = FALSE;
			CurDlg = hDlg;
			selectedMode = CLM_AUTO;
			ProgressBar = GetDlgItem(hDlg, IDC_PROGRESS2);

			TextWindow = GetDlgItem(hDlg, IDC_DISPLAY);

			UpdateTesting(engp);

			SetDlgItemText(CurDlg, IDC_DISPLAY, DISPLAYINFO);

			SetWindowPlacement(hDlg, &windowplacement);

			// Set things up to get initial server list
			strcpy(engp->hostName, MASTERSERVER);
			engp->hostCtrlPort = MASTERPORT;
			myState = MYSTATE_GETSERVERLIST;
		
			return TRUE;

		case WM_COMMAND:
			wmId = LOWORD(wParam);
			WaitMsg = FALSE;
			switch (wmId)
			{
				case IDM_ABOUT:
				   DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				   break;
				case IDM_INFO:
					MessageBox(hDlg, TPTEST_INFO, "TPTEST Information", MB_OK);
					break;
				case IDM_HJALP:
					DialogBox(hInst, (LPCTSTR)IDD_DIALOG5, hWnd, (DLGPROC)Help);
					break;
				case IDM_EXIT:
				   DestroyWindow(hWnd);
				   DeleteContext(engp);
				   exit(0);
				case IDM_ADVANCED:
					DestroyWindow(hDlg);
					CreateDialog(hInst, (LPCTSTR)IDD_ADVANCED, hWnd, (DLGPROC)AdvancedDlg);
					return TRUE;
				case IDM_PRINTREPORT:
					// Display the PRINT dialog box. 
					PrintReport(); 
					return TRUE;
				case IDC_STANDARDTEST:
					if (myState == MYSTATE_IDLE) {
						ClearBestRates(engp);
						time(&FirstStartTime);
						ClearTestResults();
						strcpy(engp->hostName, MASTERSERVER);
						engp->hostCtrlPort = MASTERPORT;
						substate = 0;
						selectedMode = CLM_AUTO;
						myState = MYSTATE_STDTEST;
					}
					else {
						myState = MYSTATE_IDLE;
						StopContext(engp);
						UpdateTesting(engp);
						MessageBox(CurDlg, "Test interrupted!", "Info", MB_OK);
					}
					return TRUE;
				case IDC_START:
					if (myState == MYSTATE_IDLE) {
						if (ServerSelected() == 0) {
							MessageBox(CurDlg, "Invalid or no server selected", "Error", MB_OK);
							myState = MYSTATE_IDLE;
							substate = 0;
						}
						else {
							strcpy(engp->hostName, selectedServerAdr);
							engp->hostCtrlPort = selectedServerPort;
							ClearBestRates(engp);
							time(&FirstStartTime);
							ClearTestResults();
							substate = 0;
							UpdateTesting(engp);
							myState = MYSTATE_AUTOTEST;
						}
					}
					else {
						myState = MYSTATE_IDLE;
						StopContext(engp);
						UpdateTesting(engp);
						MessageBox(CurDlg, "Test interrupted!", "Info", MB_OK);
					}
					return TRUE;
				case IDC_SERVERLIST:
					if (HIWORD(wParam) == LBN_SELCHANGE)
						ServerSelected();
					return TRUE;
				case IDCANCEL:
					DestroyWindow(hWnd);
					DeleteContext(engp);
					exit(0);
				case IDC_RADIO1:
					selectedMode = CLM_AUTO;
					return TRUE;
				case IDC_RADIO2:
					selectedMode = CLM_AUTO_SEND;
					return TRUE;
				case IDC_RADIO3:
					selectedMode = CLM_AUTO_RECV;
					return TRUE;
				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_MOVE:
			GetWindowPlacement(hDlg, &windowplacement);
			return TRUE;
	}
    return FALSE;
}




// Message handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				SetDlgItemText(hDlg, IDC_EDIT1, ABOUTTEXT);
				SetDlgItemText(hDlg, IDC_VERSION, engp->clientInfo);
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}


// Message handler for help box.
LRESULT CALLBACK Help(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				EnableWindow(CurDlg, FALSE);
				SetDlgItemText(hDlg, IDC_EDIT1, TPTEST_HELP);
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EnableWindow(CurDlg, TRUE);
				EndDialog(hDlg, 0);
				return TRUE;
			}
			break;
	}
    return FALSE;
}






//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_TPTEST2);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)IDC_TPTEST2;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{


   hInst = hInstance; // Store instance handle in our global variable


   // Create invisible main window. We don't actually use this
   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 0, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return FALSE;

	// Init WINDOWPLACEMENT struct
	windowplacement.length = sizeof(WINDOWPLACEMENT);
	windowplacement.flags = 0;
	windowplacement.showCmd = SW_SHOWNORMAL;
	windowplacement.ptMaxPosition.x = windowplacement.ptMaxPosition.y = 0L; 
	windowplacement.ptMinPosition.x = windowplacement.ptMinPosition.y = 0L;
	windowplacement.rcNormalPosition.bottom = 0;
	windowplacement.rcNormalPosition.left = 0;
	windowplacement.rcNormalPosition.right = 0;
	windowplacement.rcNormalPosition.top = 0;

/*  
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
*/

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId;
	switch (message) 
	{
		case WM_COMMAND:
			// Parse the menu selections:
			wmId = LOWORD(wParam);
			switch (wmId)
			{
				case IDM_EXIT:
				   DestroyWindow(hWnd);
				   DeleteContext(engp);
				   exit(0);
				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}


void PrintLine(char * line, int lineno, bool center) {
	SIZE szMetric;
	static int xLeft;
	int cWidthPels, yTop ;

	GetTextExtentPoint32(pd.hDC, line, (int)strlen(line), &szMetric); 

	if (center) {
		cWidthPels = GetDeviceCaps(pd.hDC, HORZRES); 
		xLeft = ((cWidthPels / 2) - (szMetric.cx / 2)); 
	}

    yTop = (szMetric.cy * (3 + lineno)); 
 
    // Print the path and filename for the bitmap, centered at the top 
    // of the page. 
 
    TextOut(pd.hDC, xLeft, yTop, line, (int)strlen(line)); 
}
 



void PrintReport() {
	int nError, lineno = 0;
	float fLogPelsX1, fLogPelsX2, fLogPelsY1, fLogPelsY2, fScaleX, fScaleY;
	char tmpline[100];
	struct outtext_struct * o;
	struct tm * tmPnt;

	o = outtext;
	if (o->line == NULL)
		return;

	// Initialize the members of a DOCINFO structure.
	DOCINFO di;
 
	PrintDlg(&pd);

    di.cbSize = sizeof(DOCINFO); 
    di.lpszDocName = "Bitmap Printing Test"; 
    di.lpszOutput = (LPTSTR) NULL; 
    di.lpszDatatype = (LPTSTR) NULL; 
    di.fwType = 0; 
 
    // Begin a print job by calling the StartDoc function. 
 
    nError = StartDoc(pd.hDC, &di); 
    if (nError == SP_ERROR) 
    { 
		MessageBox(CurDlg, "Print error", TPERROR, MB_OK);
		return;
    } 
 
    // Inform the driver that the application is about to begin 
    // sending data. 
 
    nError = StartPage(pd.hDC); 
    if (nError <= 0) 
    { 
		MessageBox(CurDlg, "Print error", TPERROR, MB_OK);
		return;
    } 
 
    // Retrieve the number of pixels-per-logical-inch in the 
    // horizontal and vertical directions for the display upon which 
    // the bitmap was created. 
 
    fLogPelsX1 = (float) GetDeviceCaps(pd.hDC, LOGPIXELSX); 
    fLogPelsY1 = (float) GetDeviceCaps(pd.hDC, LOGPIXELSY); 
 
    // Retrieve the number of pixels-per-logical-inch in the 
    // horizontal and vertical directions for the printer upon which 
    // the bitmap will be printed. 
 
    fLogPelsX2 = (float) GetDeviceCaps(pd.hDC, LOGPIXELSX); 
    fLogPelsY2 = (float) GetDeviceCaps(pd.hDC, LOGPIXELSY); 
 
    // Determine the scaling factors required to print the bitmap and 
    // retain its original proportions. 
 
    if (fLogPelsX1 > fLogPelsX2) 
        fScaleX = (fLogPelsX1 / fLogPelsX2); 
    else fScaleX = (fLogPelsX2 / fLogPelsX1); 
 
    if (fLogPelsY1 > fLogPelsY2) 
        fScaleY = (fLogPelsY1 / fLogPelsY2); 
    else fScaleY = (fLogPelsY2 / fLogPelsY1); 
 
	PrintLine("TPTEST Session Report", lineno, TRUE);
	lineno += 2;
	
	if (Advanced) {
		unsigned int longest = 0;
		while (o->line != NULL) {
			if (strlen(o->line) > longest)
				longest = (unsigned int)strlen(o->line);
			o = o->next;
		}
		o = outtext;
		memset(tmpline, 0, 100);
		while (longest--)
			tmpline[longest] = ' ';
		PrintLine(tmpline, lineno++, TRUE);
		while (o->line != NULL) {
			memset(tmpline, 0, 100);
			strncpy(tmpline, o->line, 99);
			stripcrlf(tmpline);
			PrintLine(tmpline, lineno++, FALSE);
			o = o->next;
		}
	}
	else {

		lineno++;

#ifdef USE_GMTIME
		tmPnt = gmtime( (time_t *)(&FirstStartTime) );
		sprintf(tmpline, "Test Started :             %04d-%02d-%02d %02d:%02d",
			tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
			tmPnt->tm_hour, tmPnt->tm_min);
#else
		tmPnt = localtime( (time_t *)(&FirstStartTime) );
		sprintf(tmpline, "Test Started :             %04d-%02d-%02d %02d:%02d",
			tmPnt->tm_year + 1900, tmPnt->tm_mon + 1, tmPnt->tm_mday,
			tmPnt->tm_hour, tmPnt->tm_min);
#endif
		PrintLine(tmpline, lineno++, TRUE);

		sprintf(tmpline, "Test Server :           %s", engp->hostName);
		PrintLine(tmpline, lineno, FALSE);

		lineno += 2;

		sprintf(tmpline, "Best UDP Send Rate :    %s", Int32ToString((int)(engp->bestUDPSendRate * 8.0)));
		PrintLine(tmpline, lineno++, FALSE);

		sprintf(tmpline, "Best UDP Receive Rate : %s", Int32ToString((int)(engp->bestUDPRecvRate * 8.0)));
		PrintLine(tmpline, lineno++, FALSE);

		sprintf(tmpline, "Best TCP Send Rate :    %s", Int32ToString((int)(engp->bestTCPSendRate * 8.0)));
		PrintLine(tmpline, lineno++, FALSE);

		sprintf(tmpline, "Best TCP Receive Rate : %s", Int32ToString((int)(engp->bestTCPRecvRate * 8.0)));
		PrintLine(tmpline, lineno++, FALSE);

		lineno++;

		if (engp->bestTCPSendRate > 0.0 && engp->bestUDPSendRate > 0.0) {
			if (engp->bestTCPSendRate > engp->bestUDPSendRate)
				sprintf(tmpline, "Send Efficiency :       100.0%%");
			else
				sprintf(tmpline, "Send Efficiency :       %d%%", (int)((engp->bestTCPSendRate/engp->bestUDPSendRate)*100));
			PrintLine(tmpline, lineno++, FALSE);
		}
		if (engp->bestTCPRecvRate > 0.0 && engp->bestUDPRecvRate > 0.0) {
			if (engp->bestTCPRecvRate > engp->bestUDPRecvRate)
				sprintf(tmpline, "Receive Efficiency :    100.0%%");
			else
				sprintf(tmpline, "Receive Efficiency :    %d%%", (int)((engp->bestTCPRecvRate/engp->bestUDPRecvRate)*100));
			PrintLine(tmpline, lineno++, FALSE);
		}

	}

    nError = EndPage(pd.hDC); 
	if (nError <= 0) 
        goto Error; 
 
    // Inform the driver that document has ended. 
 
    nError = EndDoc(pd.hDC); 

Error: 
 
    DeleteDC(pd.hDC); 
}

