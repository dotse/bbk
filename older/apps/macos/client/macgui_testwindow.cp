/*
 *	
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * macgui_testwindow.cp - main test progress window routines
 *
 * Written by
 *  Torsten Alm <totte@3tag.com>
 *  Hans Green <hg@3tag.com>
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

#include <macwindows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "macgui_testwindow.h"
#include "tpio.h"
#include "tpio_mac.h"
#include "tpclient.h"

// Locals protos

extern "C" {
	static pascal OSStatus ControlFocusEventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData);
	static pascal OSStatus ControlChangedEventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData);
	static pascal OSStatus TestWindowEventHandler(EventHandlerCallRef /*myHandler*/, EventRef event, void* userData);
	static pascal void TestWindowIdleCallBack(EventLoopTimerRef Ignored, void* userData);
	static Boolean TooMuchText(ControlRef control, SInt16 charCode);
	static pascal SInt16 HostnameKeyFilter(ControlRef control, SInt16* keyCode, SInt16*	charCode, EventModifiers* modifiers);
	static pascal SInt16 NumericKeyFilter(ControlRef control, SInt16* keyCode, SInt16*	charCode, EventModifiers* modifiers);
	static pascal OSStatus AlertSheetEventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData);
	static pascal void TestWindowWorkCallBack(EventLoopTimerRef tref, void* userData);
	static pascal OSStatus ProgressSheetEventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData);
}

TestWindow	*gMainTW;				// For access from glue-routine

// Extras

#define TEST_ERROR			(-1)
#define TEST_WAITING		0
#define TEST_RUNNING		1
#define TEST_FINISHED		2

#define TP_MASTER_PORT		1640
#define DEFAULT_PKTS_SEC 	5
#define DEFAULT_PKT_SIZE 	1024
#define DEFAULT_BITRATE 	64000
#define START_TCP_BYTES		51200
#define DEFAULT_TCPBYTES	(START_TCP_BYTES * 2)
#define DEFAULT_TESTTIME	10

// For drawing stuff

#define CS_XP			5
#define CR_XP			190
#define CC_XP			380

#define DRBASE_X		20
#define DRBASE_Y		80
#define POS_AT_CURRENT	0x8000

#define DRMODE_MASK		0x07
#define DRMODE_BOLD		0x01

#define DRJUST_MASK		0x70
#define DRJUST_RIGHT	0x10

// --------------------------------------------------------
// TestWindow [constructor]
// --------------------------------------------------------
TestWindow::TestWindow(bool inIsX)
{
	OSStatus	status;
	int			winPos;
	
	gMainTW = (TestWindow *) this;
	
	// Initialize class members
	mEvhRef = NULL;
	mFocusHandler = NULL;
	mServerChangedHandler = NULL;
	mTestWinEvtHandler = NULL;
	mAlertSheetEvtHandler = NULL;
	mHostnameKeyFilterUPP = NULL;
	mNumericKeyFilterUPP = NULL;
	mTestWindowCBHandler = NULL;
	mTestWindowIdleHandler = NULL;
	mTestWindow = NULL;
	mErrorAlertSheet = NULL;
	mTestProgressSheet = NULL;
	mIsOK = false;
	showTick = NULL;
	dispBestTCPSendRate = 0.0;
	dispBestTCPRcvRate = 0.0;
	dispBestUDPSendRate = 0.0;
	dispBestUDPRcvRate = 0.0;
	counterWorld = 0;
	isReallyDoingTest = false;
	mIsAnimationMode = false;
	masterTried = false;
	mMasterServerContacted = false;
	mInRunMode = false;
	mInFlipMode = false;
	infoMsg[0] = 0;
	failInfo[0] = 0;
	
	// Setup Draw Rect
	::SetRect(&drawRect, DRBASE_X,  DRBASE_Y, DRBASE_X+540,  DRBASE_Y+105);
	
	// Get incoming
	mIsOnX = inIsX;
	
	status = Finalize();
	if (status == noErr) {
		winPos = MakeStatWindow((mIsOnX == true ? 1 : 0));			// Set up status window	
		status = CreateWindow(winPos);
		if (status == noErr) {
			mIsOK = true;
		}
	}	
}


// --------------------------------------------------------
// TestWindow [destructor]
// --------------------------------------------------------
TestWindow::~TestWindow()
{
	// Drop idle task
	if (mTestWindowIdleRef != NULL) {
		::RemoveEventLoopTimer(mTestWindowIdleRef);
		mTestWindowIdleRef = NULL;
	}
	
	if (mTestWindowIdleHandler != NULL) {
		::DisposeEventLoopTimerUPP(mTestWindowIdleHandler);
		mTestWindowIdleHandler = NULL;
	}
	
	// Tell TPTest engine to shutdown whatever it has active
	if (engp) {
		DeleteSessComm(engp);
	}
	TPOTShutDown();
}

// -----------------------------------------------------------------------------------
// Finalize
// -----------------------------------------------------------------------------------
OSStatus 
TestWindow::Finalize(void)
{
	OSStatus		err;
	char			*osType;
    UInt32			response;
	
	// Setup the TPTestEngine OTStyle
	err = TPOTSetup();					
	if (err != noErr) {
		return err;
	}
	
	// create engine
	engp = CreateContext();
	if (engp == 0) {
		return -1;
	}
	
	// Set default values in session and dialog fields
	InitSessComm(engp);
	engp->tcpBytes = START_TCP_BYTES;
	engp->bitsPerSecond = DEFAULT_BITRATE;
	engp->packetSize = 1000;
	engp->packetsPerSecond = 8;
	engp->sessionTime = 20;

	if (mIsOnX) {
		osType = "OSX";
	} else {
		osType = "OS9";
	    if (::Gestalt(gestaltSystemVersion, (SInt32 *) &response) == noErr) {
	    	if (response == 0x00860) osType = "OS8";
	    }
	}
	sprintf(engp->clientInfo, "%s %s", kClientType, osType);
	RecalculatePPSSZ(engp);
	
	// Done
	return err;
}

// -----------------------------------------------------------------------------------
// CreateWindow
// -----------------------------------------------------------------------------------
OSStatus 
TestWindow::CreateWindow(
	int		hPos)
{
	ControlRef			dummyControl;
	ControlFontStyleRec	cfontrec;
	Rect				etbounds;
	Rect				screenRect, winRect;
	SInt16				baseline;
	unsigned long 		srvidx		= 0;
	GDHandle			myDevice;
	char				dummy[64];
	
	// Events to be handled by eventhandlers
	EventTypeSpec	evtlist[]	= {{kEventClassWindow, kEventWindowClose},
						 		 	{kEventClassWindow, kEventWindowDrawContent},
						 		 	{kEventClassWindow, kEventWindowGetClickActivation},
						 		 	{kEventClassWindow, kEventWindowBoundsChanging},
						 		 	{kEventClassCommand, kEventCommandProcess}};

	EventTypeSpec	cntrllist[] = {{kEventClassControl, kEventControlClick},
									{kEventClassKeyboard, kEventRawKeyDown}};
									
	EventTypeSpec	poplist[]	= {kEventClassControl, kEventControlHit};
	
	// Create the TestWindow from the resource
	mTestWindow = ::GetNewCWindow(kTestWindow, NULL, (WindowRef) -1);
	if (!mTestWindow) {
		::SysBeep(1);
		return -1;
	}

	// Static text font information
	cfontrec.font = kControlFontSmallSystemFont;
	cfontrec.flags = kControlUseFontMask+kControlUseFaceMask+kControlUseSizeMask+kControlUseJustMask;
	cfontrec.just = teJustLeft;

	// Set Window Attributes for Carbon Events Handling and close box
	(void) ::ChangeWindowAttributes(mTestWindow, kWindowStandardHandlerAttribute | kWindowCloseBoxAttribute, NULL);

	// Set The correct Window background theme
	(void) ::SetThemeWindowBackground(mTestWindow, kThemeBrushDialogBackgroundActive, true);

	// Add controlitems to the TestWindow
	// [Root control]
	::CreateRootControl(mTestWindow, &mRootControl);

	// [Start Button]
	mStartButtonRef = ::GetNewControl(kTestStartButton, mTestWindow);
	::SetControlCommandID(mStartButtonRef, kCommandRun);
  	::SetWindowDefaultButton(mTestWindow, mStartButtonRef);
	::EmbedControl(mStartButtonRef, mRootControl);
	
	// [Disclosure triangle]
	mFlipperControl = ::GetNewControl(kTestWindowFlipper, mTestWindow);
	::SetControlCommandID(mFlipperControl, kCommandFlip);
	::EmbedControl(mFlipperControl, mRootControl);
	
	// [Flipper infortext]
	mFlipperInfoControl = ::GetNewControl(kTestWindowFlipperInfo, mTestWindow);
	(void)::SetControlData(mFlipperInfoControl, 0, kControlStaticTextTextTag, strlen(kAdvancedModeText), kAdvancedModeText);
	(void)::SetControlData(mFlipperInfoControl, 0, kControlStaticTextStyleTag, sizeof(cfontrec), &cfontrec);	
	::EmbedControl(mFlipperInfoControl, mRootControl);
	
	// [Server Popup]
	mTestServerPopupRef = ::GetNewControl(kTestServerPopup, mTestWindow);
	::SetControlCommandID(mTestServerPopupRef, kServerFlip);
	
	// [Method Popup]
	mTestMethodPopupRef = ::GetNewControl(kTestMethodPopup, mTestWindow);
	
	// Good to set
	mAdvancedMode = false;
	
	//
	// Advanced mode stuff
	//
	
	// Create key filers & focus evthandlers
	mHostnameKeyFilterUPP	= NewControlKeyFilterUPP(HostnameKeyFilter);
	mNumericKeyFilterUPP	= NewControlKeyFilterUPP(NumericKeyFilter);
	mFocusHandler			= NewEventHandlerUPP(ControlFocusEventHandler);
	mServerChangedHandler	= NewEventHandlerUPP(ControlChangedEventHandler);

	// text label size
	cfontrec.font = kControlFontBigSystemFont;

	//
	// Type Popup Button
	//
	
	// [Type Popup]
	mTestTypePopup = ::GetNewControl(kTestWindowTypePop, mTestWindow);
	//::EmbedControl(mTestTypePopup, mTestAdvanced);
	::SetControlCommandID(mTestTypePopup, kTypePopup);

	// [grouping control]
	mTestAdvanced = ::GetNewControl(kTestWindowAdvGroup, mTestWindow);
	::EmbedControl(mTestAdvanced, mRootControl);
	
	// [tcp group control]
	mTestTCPGrp = ::GetNewControl(kTestWindowTCPGrp, mTestWindow);
	::EmbedControl(mTestTCPGrp, mTestAdvanced);
	
	// [udp group control]
	mTestUDPGrp = ::GetNewControl(kTestWindowUDPGrp, mTestWindow);
	::EmbedControl(mTestUDPGrp, mTestAdvanced);
	
	// [Server editfield]
	mServerEditField = ::GetNewControl(kTestWindowServerEF, mTestWindow);
	::EmbedControl(mServerEditField, mTestAdvanced);
	
	// Set max chars to accept
	::SetControlReference(mServerEditField, 63);
	
	// Set type of keyfilter
	(void) ::SetControlData(mServerEditField, 0, kControlKeyFilterTag, sizeof(mHostnameKeyFilterUPP), (Ptr)&mHostnameKeyFilterUPP);
	// Check controlsize
	if (::GetBestControlRect(mServerEditField, &etbounds, &baseline) == noErr) {
		::SetControlBounds(mServerEditField, &etbounds);
	}
	
	// [Server infotext]
	dummyControl = ::GetNewControl(kTestWindowServerST, mTestWindow);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextTextTag, strlen(kServerText), kServerText);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextStyleTag, sizeof(cfontrec), &cfontrec);	
	::EmbedControl(dummyControl, mTestAdvanced);

	// [Port editfield]
	mPortEditField = ::GetNewControl(kTestWindowPortEF, mTestWindow);
	EmbedControl(mPortEditField, mTestAdvanced);

	// Set max chars to accept
	::SetControlReference(mPortEditField, 5);

	// Set type of keyfilter
	(void) ::SetControlData(mPortEditField, 0, kControlKeyFilterTag, sizeof(mNumericKeyFilterUPP), (Ptr)&mNumericKeyFilterUPP);
	// Check controlsize
	if (::GetBestControlRect(mPortEditField, &etbounds, &baseline) == noErr) {
		::SetControlBounds(mPortEditField, &etbounds);
	}

	// [Port infotext]
	dummyControl = ::GetNewControl(kTestWindowPortST, mTestWindow);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextTextTag, strlen(kPortText), kPortText);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextStyleTag, sizeof(cfontrec), &cfontrec);	
	::EmbedControl(dummyControl, mTestAdvanced);

	// [time editfield]
	mTimeEditField = ::GetNewControl(kTestWindowTimeEF, mTestWindow);
	::EmbedControl(mTimeEditField, mTestAdvanced);

	// Set max chars to accept
	::SetControlReference(mTimeEditField, 2);

	// Set default data
	sprintf(dummy,"%ld", engp->sessionTime);
	(void)::SetControlData(mTimeEditField, 0, kControlEditTextTextTag, strlen(dummy), dummy);

	// Set type of keyfilter
	(void) ::SetControlData(mTimeEditField, 0, kControlKeyFilterTag, sizeof(mNumericKeyFilterUPP), (Ptr)&mNumericKeyFilterUPP);
	// Check controlsize
	if (::GetBestControlRect(mTimeEditField, &etbounds, &baseline) == noErr) {
		::SetControlBounds(mTimeEditField, &etbounds);
	}

	// [time infotext]
	dummyControl = ::GetNewControl(kTestWindowTimeST, mTestWindow);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextTextTag, strlen(kTestTimeText), kTestTimeText);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextStyleTag, sizeof(cfontrec), &cfontrec);	
	::EmbedControl(dummyControl, mTestAdvanced);
	
	
	//
	// TCP Fields
	//
	
	// [tcpbytes editfield]
	mTCPBytesField = ::GetNewControl(kTestWindowTCPEF, mTestWindow);
	::EmbedControl(mTCPBytesField, mTestTCPGrp);

	// Set max chars to accept
	::SetControlReference(mTCPBytesField, 10);

	// Set default data
	sprintf(dummy,"%ld", engp->tcpBytes);
	(void)::SetControlData(mTCPBytesField, 0, kControlEditTextTextTag, strlen(dummy), dummy);

	// Set type of keyfilter
	(void) ::SetControlData(mTCPBytesField, 0, kControlKeyFilterTag, sizeof(mNumericKeyFilterUPP), (Ptr)&mNumericKeyFilterUPP);
	// Check controlsize
	if (::GetBestControlRect(mTCPBytesField, &etbounds, &baseline) == noErr) {
		::SetControlBounds(mTCPBytesField, &etbounds);
	}

	// [tcp infortext]
	dummyControl = ::GetNewControl(kTestWindowTCPST, mTestWindow);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextTextTag, strlen(kTestTCPText), kTestTCPText);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextStyleTag, sizeof(cfontrec), &cfontrec);	
	::EmbedControl(dummyControl, mTestTCPGrp);

	
	//
	// UDP Fields
	//
	
	// [Datarate editfield]
	mDatarateEditField = ::GetNewControl(kTestWindowDataEF, mTestWindow);
	EmbedControl(mDatarateEditField, mTestUDPGrp);

	// Set max chars to accept
	::SetControlReference(mDatarateEditField, 10);

	// Set type of keyfilter
	(void) ::SetControlData(mDatarateEditField, 0, kControlKeyFilterTag, sizeof(mNumericKeyFilterUPP), (Ptr)&mNumericKeyFilterUPP);
	
	// Set default data
	sprintf(dummy,"%ld", engp->bitsPerSecond);
	(void)::SetControlData(mDatarateEditField, 0, kControlEditTextTextTag, strlen(dummy), dummy);

	// Check controlsize
	if (::GetBestControlRect(mDatarateEditField, &etbounds, &baseline) == noErr) {
		::SetControlBounds(mDatarateEditField, &etbounds);
	}

	// [datarate infortext]
	dummyControl = ::GetNewControl(kTestWindowDataST, mTestWindow);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextTextTag, strlen(kDatarateText), kDatarateText);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextStyleTag, sizeof(cfontrec), &cfontrec);	
	::EmbedControl(dummyControl, mTestUDPGrp);

	// [packetsize editfield]
	mPacksizeEditField = ::GetNewControl(kTestWindowPackEF, mTestWindow);
	::EmbedControl(mPacksizeEditField, mTestUDPGrp);

	// Set max chars to accept
	::SetControlReference(mPacksizeEditField, 5);

	// Set type of keyfilter
	(void) ::SetControlData(mPacksizeEditField, 0, kControlKeyFilterTag, sizeof(mNumericKeyFilterUPP), (Ptr)&mNumericKeyFilterUPP);

	// Set default data
	sprintf(dummy,"%ld", engp->packetSize);
	(void)::SetControlData(mPacksizeEditField, 0, kControlEditTextTextTag, strlen(dummy), dummy);
	

	// Check controlsize
	if (::GetBestControlRect(mPacksizeEditField, &etbounds, &baseline) == noErr) {
		::SetControlBounds(mPacksizeEditField, &etbounds);
	}

	// [packetsize infortext]
	dummyControl = ::GetNewControl(kTestWindowPackST, mTestWindow);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextTextTag, strlen(kPacksizeText), kPacksizeText);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextStyleTag, sizeof(cfontrec), &cfontrec);	
	::EmbedControl(dummyControl, mTestUDPGrp);


	// [packetrate editfield]
	mPacketrateEditField = ::GetNewControl(kTestWindowRateEF, mTestWindow);
	::EmbedControl(mPacketrateEditField, mTestUDPGrp);

	// Set max chars to accept
	::SetControlReference(mPacketrateEditField, 6);

	// Set default data
	sprintf(dummy,"%ld", engp->packetsPerSecond);
	(void)::SetControlData(mPacketrateEditField, 0, kControlEditTextTextTag, strlen(dummy), dummy);

	// Set type of keyfilter
	(void) ::SetControlData(mPacketrateEditField, 0, kControlKeyFilterTag, sizeof(mNumericKeyFilterUPP), (Ptr)&mNumericKeyFilterUPP);
	// Check controlsize
	if (::GetBestControlRect(mPacketrateEditField, &etbounds, &baseline) == noErr) {
		::SetControlBounds(mPacketrateEditField, &etbounds);
	}

	// [packetrate infortext]
	dummyControl = ::GetNewControl(kTestWindowRateST, mTestWindow);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextTextTag, strlen(kPackrateText), kPackrateText);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextStyleTag, sizeof(cfontrec), &cfontrec);	
	::EmbedControl(dummyControl, mTestUDPGrp);

	// Add EventHandler to the Window
	mTestWinEvtHandler = ::NewEventHandlerUPP(TestWindowEventHandler);
	(void) ::InstallWindowEventHandler(mTestWindow, mTestWinEvtHandler, sizeof(evtlist)/sizeof(EventTypeSpec), evtlist, this, NULL);

	// Install eventhandlers for the controls
	(void) ::InstallControlEventHandler(mPacketrateEditField, mFocusHandler, 2, cntrllist,  this, NULL);
	(void) ::InstallControlEventHandler(mPacksizeEditField, mFocusHandler, 2, cntrllist,  this, NULL);
	(void) ::InstallControlEventHandler(mDatarateEditField, mFocusHandler, 2, cntrllist,  this, NULL);
	(void) ::InstallControlEventHandler(mServerEditField, mFocusHandler, 2, cntrllist,  this, NULL);
	(void) ::InstallControlEventHandler(mPortEditField, mFocusHandler, 2, cntrllist,  this, NULL);
	(void) ::InstallControlEventHandler(mTimeEditField, mFocusHandler, 2, cntrllist,  this, NULL);
	(void) ::InstallControlEventHandler(mTestServerPopupRef, mServerChangedHandler, 1, poplist,  this, NULL);
	
	// Set popups 
	::SetControlValue(mTestServerPopupRef, 1);
	::SetControlValue(mTestMethodPopupRef, 3);
	::SetControlValue(mTestTypePopup, 1);
	
	// Start in TCP Mode
 	::HideControl(mTestUDPGrp);
	::DeactivateControl(mTestUDPGrp);
	::DeactivateControl(mTestAdvanced);
	::DeactivateControl(mTestServerPopupRef);
	mAdvancedTestMode=0;
	
	// Show current server and port in advanced mode
	SetupServerAndPort();

	// Start the idle timer
	StartIdleTimer();
	
	// Try putting it beside the status window but over it on small screens	
	myDevice = ::GetMainDevice();
	::GetAvailableWindowPositioningBounds(myDevice, &screenRect);
	::GetWindowBounds(mTestWindow, kWindowStructureRgn, &winRect);
	
	if (mIsOnX == false) {					// non OSX has borders
		hPos += 8;
		winRect.left -= 8;
		winRect.right += 8;
	}
	if (screenRect.right - (winRect.right - winRect.left) >= hPos + 5) {
		::MoveWindow(mTestWindow, hPos + 5, 50, 1);
	} else {
		::MoveWindow(mTestWindow, screenRect.right - (winRect.right - winRect.left) - 5, 50, 1);
	}

	// Show it
	::ShowWindow(mTestWindow);
	::DrawControls(mTestWindow);

	// Done
	return noErr;
}


// -----------------------------------------------------------------------------------
// StartIdleTimer - Start the nice idle task timer
// -----------------------------------------------------------------------------------
void
TestWindow::StartIdleTimer(void)
{
	// Install the Timer
	mTestWindowIdleHandler = ::NewEventLoopTimerUPP(TestWindowIdleCallBack);
	(void) ::InstallEventLoopTimer(
					::GetCurrentEventLoop(), 
					kEventDurationMillisecond * 200,
					kEventDurationMillisecond * 200,
					mTestWindowIdleHandler,
					this,
					&mTestWindowIdleRef);
}

// -----------------------------------------------------------------------------------
// StopIdleTimer - Stop the nice idle task timer
// -----------------------------------------------------------------------------------
void
TestWindow::StopIdleTimer(void)
{
	if (mTestWindowIdleRef) {
		::RemoveEventLoopTimer(mTestWindowIdleRef);
		mTestWindowIdleRef=NULL;
	}
	if (mTestWindowIdleHandler) {
		::DisposeEventLoopTimerUPP(mTestWindowIdleHandler);
		mTestWindowIdleHandler=NULL;
	}
}

// -----------------------------------------------------------------------------------
// SetTestInfo - Show what we're doing
// -----------------------------------------------------------------------------------
void
TestWindow::SetTestInfo(
	char	*msgtext)
{
	(void)::SetControlData(mProgInfoText, 0, kControlStaticTextTextTag, strlen(msgtext), msgtext);
	::DrawOneControl(mProgInfoText);
}

// -----------------------------------------------------------------------------------
// HandleFocusChange - RecalculateValues on focus change via key
// -----------------------------------------------------------------------------------
void
TestWindow::HandleFocusChange(
	ControlRef inControl,
	ControlRef focusControl)
{
	if (focusControl == mDatarateEditField ||
		focusControl == mPacksizeEditField ||
		focusControl == mPacketrateEditField) {
		
		(void) HandleValueChanges();
		mCurrentFocusControl = inControl;
	}
}

// -----------------------------------------------------------------------------------
// HandleSetFocus - RecalculateValues on focus change via click
// -----------------------------------------------------------------------------------
void
TestWindow::HandleSetFocus(void)
{
	::GetKeyboardFocus(mTestWindow, &mCurrentFocusControl);
	if (mCurrentFocusControl == mDatarateEditField ||
		mCurrentFocusControl == mPacksizeEditField ||
		mCurrentFocusControl == mPacketrateEditField) {
		
		(void) HandleValueChanges();
	}
}


// -----------------------------------------------------------------------------------
// ExecuteTest -
// -----------------------------------------------------------------------------------
void
TestWindow::ExecuteTest(void)
{
	OSStatus 	status;
	long		actualSize;
	char		dummy[32];
	
	if (!mMasterServerContacted) {
		sprintf(failInfo, "Kunde ej koppla upp mot masterservern");
		UpdateTestValues();
		return;
	}

	// Setup The Engine record
	
	engp->wipeFlag = 1;
	strcpy(failInfo, "");
	engp->tpMode = M_NAME_LOOKUP;

	isReallyDoingTest = false;
	time(&testStartTime);
	time(&testEndTime);
	
	// Set the Action state
	
	selectedMode = M_NAME_LOOKUP;
	
	status = GetControlData(mServerEditField, 0, kControlEditTextTextTag, 63,  dummy, &actualSize);
	if (status == noErr) {
		dummy[actualSize]=0;
		strcpy(engp->hostName, dummy);
	}

	status = GetControlData(mPortEditField, 0, kControlEditTextTextTag, 63, dummy, &actualSize);
	if (status == noErr) {
		dummy[actualSize]=0;
		engp->hostCtrlPort = (int) atol(dummy);
	}

	if (mAdvancedMode) {
		SetupAdvancedTestMode();		
		status = GetControlData(mTimeEditField, 0, kControlEditTextTextTag, 2, dummy, &actualSize);
		if (status == noErr) {
			dummy[actualSize] = 0;
			engp->sessionTime = (unsigned long) atol(dummy);
		}
		
		status = GetControlData(mTCPBytesField, 0, kControlEditTextTextTag, 12, dummy, &actualSize);
		if (status == noErr) {
			dummy[actualSize]=0;
			engp->tcpBytes = (unsigned long) atol(dummy);
		}

		switch (mAdvancedTestMode) {
		
		case 1: wantedMode = CLM_TCP_SEND; break;
		case 2: wantedMode = CLM_TCP_RECV; break;
		case 3: wantedMode = CLM_UDP_SEND; break;
		case 4: wantedMode = CLM_UDP_RECV; break;
		case 5: wantedMode = CLM_UDP_FDX;  break;
			
		default:
				ShowErrorAlertSheet("Illegal mode selected");	
				wantedMode=-1;
				break;
		}
		
		if (wantedMode == -1) {
			return;
		}
	} else {		
		// Get server and port

		// Auto mode
		switch (GetControlValue(mTestMethodPopupRef)) {
		
		case 1:  wantedMode = CLM_AUTO_SEND; break;
		case 2:  wantedMode = CLM_AUTO_RECV;	 break;
		case 3:  wantedMode = CLM_AUTO;		 break;
			
		default:
				ShowErrorAlertSheet("Illegal mode selected");	
				wantedMode=-1;
				break;
		}
		
		if (wantedMode == -1) {
			return;
		}
		
		engp->tcpBytes = START_TCP_BYTES;
		engp->bitsPerSecond = DEFAULT_BITRATE;
		engp->packetSize = 1000;
		engp->packetsPerSecond = 8;
		engp->sessionTime = 20;
		RecalculatePPSSZ(engp);
	}

	sessActive = -1;
	currentHostIdx = 0;
	mInRunMode = true;
	TrcLog(1, "Starting lookup for %s", engp->hostName);
	StartClientContext(engp);
 				
	// Stop the idletickTimer
	StopIdleTimer();
	
	// Set Test Mode Running
	SetRunningTestMode();
	SetTestInfo("Resolvar TestServer namn");
	
	// Set the button text
	if (!mIsOnX) {
		::SetControlTitle(mStartButtonRef, kpCancelText);
	}

	// Install the Timer
	mTestWindowCBHandler = ::NewEventLoopTimerUPP(TestWindowWorkCallBack);
	(void) ::InstallEventLoopTimer(
					::GetCurrentEventLoop(), 
					kEventDurationMicrosecond * 100,
					kEventDurationMicrosecond * 100,
					mTestWindowCBHandler,
					this,
					&mTestWindowCBRef);
}

// -----------------------------------------------------------------------------------
// UpdateTestValues - Draws the testvalueset onto the screen
// -----------------------------------------------------------------------------------
void
TestWindow::UpdateTestValues(void)
{
	RGBColor	saveFore, saveBack;

	GetForeColor(&saveFore);
	GetBackColor(&saveBack);
	
	ShowCounters();

	RGBBackColor(&saveBack);
	RGBForeColor(&saveFore);	
}

// -----------------------------------------------------------------------------------
// ActivateWithSheet - Activates any attached sheet
// -----------------------------------------------------------------------------------
void
TestWindow::ActivateWithSheet(void)
{
	if (mTestProgressSheet) {
		BringToFront(mTestProgressSheet);
		SelectWindow(mTestProgressSheet);
	}
	else if (mErrorAlertSheet) {
		BringToFront(mErrorAlertSheet);
		SelectWindow(mErrorAlertSheet);
	}
}

// -----------------------------------------------------------------------------------
// SetupAdvancedTestMode - 
// -----------------------------------------------------------------------------------
void
TestWindow::SetupAdvancedTestMode(void)
{
	// Get the selected testmode
 	mAdvancedTestMode = ::GetControlValue(mTestTypePopup);
	
 	// Handle GUI settings for this mode
	if (mAdvancedTestMode < 3) {	
		::ClearKeyboardFocus(mTestWindow);
    	::HideControl(mTestUDPGrp);
    	::DeactivateControl(mTestUDPGrp);
		::ShowControl(mTestTCPGrp);
		::ActivateControl(mTestTCPGrp);
		mCurrentFocusControl = mTCPBytesField;
		::SetKeyboardFocus(mTestWindow,mCurrentFocusControl, kControlFocusNextPart); 
	}
	else {
		::ClearKeyboardFocus(mTestWindow);
   		::HideControl(mTestTCPGrp);
    	::DeactivateControl(mTestTCPGrp);
 		::ShowControl(mTestUDPGrp);
		::ActivateControl(mTestUDPGrp);
		mCurrentFocusControl = mDatarateEditField;
		::SetKeyboardFocus(mTestWindow,mCurrentFocusControl, kControlFocusNextPart); 
	}
}


// -----------------------------------------------------------------------------------
// HandleValueChanges - handles the special value fields
// -----------------------------------------------------------------------------------
bool
TestWindow::HandleValueChanges(void)
{
	char			dummy[32];
	long 			actualSize;
	OSStatus		status;
	DataRates		theRates;
	DataRates		*drp;
	unsigned long	user_datarate,  user_packetsize, user_packetrate;
	
	// Get the current values
	status = ::GetControlData(mDatarateEditField, 0, kControlEditTextTextTag, 12, dummy, &actualSize);
	if (status == noErr) {
		dummy[actualSize]=0;
		user_datarate = (unsigned long) atol(dummy);
	}
	status = ::GetControlData(mPacksizeEditField, 0, kControlEditTextTextTag, 12, dummy, &actualSize);
	if (status == noErr) {
		dummy[actualSize]=0;
		user_packetsize = (unsigned long) atol(dummy);
	}
	status = ::GetControlData(mPacketrateEditField, 0, kControlEditTextTextTag, 12, dummy, &actualSize);
	if (status == noErr) {
		dummy[actualSize]=0;
		user_packetrate = (unsigned long) atol(dummy);
	}
	status = ::GetControlData(mTimeEditField, 0, kControlEditTextTextTag, 2, dummy, &actualSize);
	if (status == noErr) {
		dummy[actualSize]=0;
		engp->sessionTime = (unsigned long) atol(dummy);
	}
	status = ::GetControlData(mTCPBytesField, 0, kControlEditTextTextTag, 12, dummy, &actualSize);
	if (status == noErr) {
		dummy[actualSize]=0;
		engp->tcpBytes = (unsigned long) atol(dummy);
	}
	
	// Recalculate the Stuff
	drp = &theRates;

	drp->newDataRate	= user_datarate;
	drp->oldDataRate	= engp->bitsPerSecond;
	
	drp->newPacketSize	= user_packetsize;
	drp->oldPacketSize	= engp->packetSize;
	
	drp->newPacketRate	= user_packetrate;
	drp->oldPacketRate	= engp->packetsPerSecond;

	// Do the validation
	ValidateRates(drp, engp);
	
	// process result
	engp->bitsPerSecond = drp->newDataRate;
	engp->packetSize = drp->newPacketSize;
	engp->packetsPerSecond = drp->newPacketRate;
	
	// showmethevalues
	sprintf(dummy, "%ld", drp->newDataRate);
	::SetControlData(mDatarateEditField, 0, kControlEditTextTextTag, strlen(dummy), dummy);
	::DrawOneControl(mDatarateEditField);
	
	sprintf(dummy, "%ld", drp->newPacketSize);
	::SetControlData(mPacksizeEditField, 0, kControlEditTextTextTag, strlen(dummy), dummy);
	::DrawOneControl(mPacksizeEditField);

	sprintf(dummy, "%ld", drp->newPacketRate);
	::SetControlData(mPacketrateEditField, 0, kControlEditTextTextTag,  strlen(dummy), dummy);
	::DrawOneControl(mPacketrateEditField);

	// If HG found an Error
	if (drp->errorFlag != 0) {

		// Show the Alert
		ShowErrorAlertSheet(drp->message);		
	}
	
	// done
	return (drp->errorFlag==0);	
}

// -----------------------------------------------------------------------------------
// ShowErrorAlertSheet - Show a sheet with an error message in it
// -----------------------------------------------------------------------------------
void 
TestWindow::ShowErrorAlertSheet(char* theErrorMessage)
{
	// Locals
	Rect				boundsRect = {0, 0, 100, 508};
	ControlRef			OKButtonRef, rootControl, dummyControl;	
	EventTypeSpec		evtlist[] =  {kEventClassCommand, kEventCommandProcess};
	ControlFontStyleRec	cfontrec;

	// Create the ProgressSheet direcly [Resourcerer cant setup a sheet window yet]
	mErrorAlertSheet = ::NewCWindow(NULL,&boundsRect, kpTestIsOn, false, kWindowSheetProc, (WindowRef) 0, false, NULL);
	::SysBeep(1);
	if (!mErrorAlertSheet) {
		return;
	}

	// Set Window Attributes for Carbon Events Handling and closebox
	(void) ::ChangeWindowAttributes(mErrorAlertSheet, kWindowStandardHandlerAttribute, NULL);

	// Set The correct Window background theme
	(void) ::SetThemeWindowBackground(mErrorAlertSheet, kThemeBrushDialogBackgroundActive, true);
	
	// Add controlitems to the Error Alert
	// [root Control]
	::CreateRootControl(mErrorAlertSheet, &rootControl);
	
	// [OK Button]
	OKButtonRef = ::GetNewControl(kAlertSheetOKButton, mErrorAlertSheet);
	::SetControlCommandID(OKButtonRef, kCommandClose);
	::SetWindowDefaultButton(mErrorAlertSheet, OKButtonRef);
	::EmbedControl(OKButtonRef, rootControl);
			

	// Static tect font information
	cfontrec.font = kControlFontBigSystemFont;
	cfontrec.flags = kControlUseFontMask+kControlUseFaceMask+kControlUseSizeMask+kControlUseJustMask;
	cfontrec.just = teJustCenter;

	// [ Text box ]
	dummyControl = ::GetNewControl(kAlertSheetOKText, mErrorAlertSheet);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextTextTag, strlen(theErrorMessage), theErrorMessage);
	(void)::SetControlData(dummyControl, 0, kControlStaticTextStyleTag, sizeof(cfontrec), &cfontrec);	
	::EmbedControl(dummyControl, rootControl);

	// Add EventHandler to the Window
	mAlertSheetEvtHandler = ::NewEventHandlerUPP(AlertSheetEventHandler);
   ::InstallWindowEventHandler(mErrorAlertSheet, mAlertSheetEvtHandler, 1, evtlist, this, NULL);

	// Show it
	::ShowSheetWindow(mErrorAlertSheet, mTestWindow);
	::DrawControls(mErrorAlertSheet);
	
}

// -----------------------------------------------------------------------------------
// CloseErrorSheet - Close the Error Sheet
// -----------------------------------------------------------------------------------
void 
TestWindow::CloseErrorSheet(void)
{
	// Sanity
	if (!mErrorAlertSheet) {
		return;
	}
	
	// Hide the sheet, nice effect
	::HideSheetWindow(mErrorAlertSheet);
	
	// Dispose the EventHandler UPP
	if (mAlertSheetEvtHandler) {
 		::DisposeEventHandlerUPP(mAlertSheetEvtHandler);
 		mAlertSheetEvtHandler=NULL;
 	}
 	
 	// OK, Drop the window
   ::ReleaseWindow(mErrorAlertSheet);
   mErrorAlertSheet=NULL;
   
}

// -----------------------------------------------------------------------------------
// FlipAdvancedMode - Advanced mode or not
// -----------------------------------------------------------------------------------
void 
TestWindow::FlipAdvancedMode(void)
{
	Rect	bounds;
	
	// Get current bounds
	::GetWindowBounds(mTestWindow, kWindowStructureRgn, &bounds);
	
	// if going to advanced mode
	if (!mAdvancedMode) {
		mAdvancedMode=true;
		bounds.bottom += kFlipResize;
		(void)::SetControlData(mFlipperInfoControl, 0, kControlStaticTextTextTag, strlen(kNormalModeText), kNormalModeText);
		::DrawOneControl(mFlipperInfoControl);
		
		// Enable controls
		::ShowControl(mTestAdvanced);
		::ActivateControl(mTestAdvanced);
		::DeactivateControl(mTestMethodPopupRef);
		Sys86Fix();						// Compensate for MacOS 8.6 control manager bugs
		
		// Set the control focus
		mCurrentFocusControl = mServerEditField;
		::SetKeyboardFocus(mTestWindow, mCurrentFocusControl, kControlFocusNextPart); 
		
		// Do the transition
		if (mIsOnX) {
			mInFlipMode=true;
			::TransitionWindow(mTestWindow, kWindowSlideTransitionEffect, kWindowResizeTransitionAction, &bounds);
			mInFlipMode=false;
		}
		else {
			// This is due to bugs in TransitionWindow in CarbonLib 
			::SetWindowBounds(mTestWindow, kWindowStructureRgn, &bounds);
		}
		::ConstrainWindowToScreen(mTestWindow, kWindowStructureRgn, kWindowConstrainStandardOptions, NULL, NULL);
	}
	else {
		mAdvancedMode=false;
		bounds.bottom -= kFlipResize;
		(void)::SetControlData(mFlipperInfoControl, 0, kControlStaticTextTextTag, strlen(kAdvancedModeText), kAdvancedModeText);
		::DrawOneControl(mFlipperInfoControl);

		// Do the transition
		if (mIsOnX) {
			mInFlipMode=true;
			::TransitionWindow(mTestWindow, kWindowSlideTransitionEffect, kWindowResizeTransitionAction, &bounds);
			mInFlipMode=false;
		}
		else {
			// This is due to bugs in TransitionWindow in CarbonLib 
			::SetWindowBounds(mTestWindow, kWindowStructureRgn, &bounds);
		}
		::ConstrainWindowToScreen(mTestWindow, kWindowStructureRgn, kWindowConstrainStandardOptions, NULL, NULL);
		
		// Disable controls
		::HideControl(mTestAdvanced);
		::DeactivateControl(mTestAdvanced);
		::ActivateControl(mTestMethodPopupRef);
		if (mCurrentFocusControl!=NULL) {
			::SetKeyboardFocus(mTestWindow,mCurrentFocusControl, kControlFocusNoPart); 
			mCurrentFocusControl=NULL;
		}
	}
}

// -----------------------------------------------------------------------------------
// Sys86Fix
// This workaround is due to a bug in control layering in OS 8.6 that doesnt work bette
// even if we have a better CarbonLib.
// -----------------------------------------------------------------------------------
void 
TestWindow::Sys86Fix(void)
{ 
    // Locals
    UInt32 response;

    if (::Gestalt(gestaltSystemVersion, (SInt32 *) &response) == noErr) {
    	if ( response == 0x00860) {
    		::ShowControl(mTestUDPGrp);
			::ActivateControl(mTestUDPGrp);
			::DrawControls(mTestWindow);
			::HideControl(mTestUDPGrp);
			::DeactivateControl(mTestUDPGrp);
			SetupAdvancedTestMode();
    	}
    }
}

// -----------------------------------------------------------------------------------
// StartMasterServerQuery - Query Masterserver
// -----------------------------------------------------------------------------------
void 
TestWindow::StartMasterServerQuery(void)
{
	// Setup The Engine record
	strcpy(engp->hostName, kDefaultMaster);
	
	engp->wipeFlag = 1;
	strcpy(failInfo, "");
	engp->tpMode = M_NAME_LOOKUP;
	
	TrcLog(1, "Starting lookup for %s", engp->hostName);
	StartClientContext(engp);
 			
	// Set the Action state
	selectedMode = M_NAME_LOOKUP;
	wantedMode = M_QUERY_MASTER;
	sessActive = -1;
	currentHostIdx = 0;
	isReallyDoingTest = false;
	masterTried = true;
	
	// Stop the idletick Timer
	StopIdleTimer();
	
	// Set Test Mode Running
	SetRunningTestMode();
	SetTestInfo("Letar efter masterserver");
	
	// Set the button text
	if (!mIsOnX) {
		::SetControlTitle(mStartButtonRef, kpCancelText);
	}

	// Install the Timer
	mTestWindowCBHandler = ::NewEventLoopTimerUPP(TestWindowWorkCallBack);
	(void) ::InstallEventLoopTimer(
					::GetCurrentEventLoop(), 
					kEventDurationMicrosecond * 100,
					kEventDurationMicrosecond * 100,
					mTestWindowCBHandler,
					this,
					&mTestWindowCBRef);
}


// -----------------------------------------------------------------------------------
// DoneMasterServerQuery - Populate Master Server List
// -----------------------------------------------------------------------------------
void 
TestWindow::DoneMasterServerQuery(void)
{	
	MenuRef		popMenu;
	char		*cp;
	Str255		pMenuLine;
	int			srvridx		= 0;

	// Force the only server until the world gets bigger
	//ClearServerList(engp);
	//AddServerToList(engp, "hostname=194.213.69.155;port=1640;proto=TCP;info=Ragnar, 256 kb/s");
	//AddServerToList(engp, "hostname=213.88.212.151;port=1634;proto=TCP;info=Doggie, 256 kb/s");
	
	// Add the entries
	if (::GetControlData(mTestServerPopupRef, 0, kControlPopupButtonMenuRefTag, sizeof(MenuRef), &popMenu, NULL) == noErr) {
		
		// Loop and get all servers and ports
		for (int idx = 1 ; idx <= engp->numServers ; idx++) {
		
			// Get serverinfo at the index
			cp = engp->serverInfoList[idx - 1];
			c2pstrcpy(pMenuLine, cp);
			::AppendMenuItemText(popMenu, pMenuLine);
			srvridx++;
			
		}
		
		// make menuline
		::SetControlMaximum(mTestServerPopupRef, srvridx);
		::SetControlValue(mTestServerPopupRef, 1);
		
		// OK, we found an Error
		if (srvridx == 0) {
			::HideSheetWindow(mTestProgressSheet);
		 	::DisposeEventHandlerUPP(mSheetWinEvtHandler);
			mSheetWinEvtHandler=NULL;
			::ReleaseWindow(mTestProgressSheet);
			mTestProgressSheet=NULL;
			ShowErrorAlertSheet(kMasterServerError);
			
			// Disable some controls
			::DeactivateControl(mFlipperControl);
			::DeactivateControl(mFlipperInfoControl);
			::DeactivateControl(mStartButtonRef);
			
		}
		else {
			mMasterServerContacted=true;
			StartIdleTimer();
			::ActivateControl(mTestServerPopupRef);
			::ActivateControl(mFlipperControl);
			::ActivateControl(mFlipperInfoControl);
			::SetControlValue(mTestServerPopupRef, 1);
			SetupServerAndPort();
			
			// Reset the button text
			if (!mIsOnX) {
				::SetControlTitle(mStartButtonRef, kpStartText);
			}
		}
	}
}	

// -----------------------------------------------------------------------------------
// SetupServerAndPort - Setup the Server and Port Edit fields
// -----------------------------------------------------------------------------------
void 
TestWindow::SetupServerAndPort(void)
{
	char		*server;
	char		port[16];
	int			idx;
	
	// Get The Control value for the Server popup menu
	
	idx = GetControlValue(mTestServerPopupRef);
	if (idx > engp->numServers) return;			// Buggy value
	
	// Get Selected Server and port
	
	server = engp->serverNameList[idx-1];	
	sprintf(port, "%d",	engp->serverPortList[idx-1]);

	(void)::SetControlData(mServerEditField, 0, kControlEditTextTextTag, strlen(server), server);
	(void)::SetControlData(mPortEditField, 0, kControlEditTextTextTag, strlen(port), port);

	::DrawOneControl(mServerEditField);
	::DrawOneControl(mPortEditField);
}	

// -----------------------------------------------------------------------------------
// NextSubTest - Handle sub test switches
// -----------------------------------------------------------------------------------
void 
TestWindow::NextSubTest(void)
{	
	int					subResult;

	// Check if we're in a completed state
	if (engp->state == CLSM_COMPLETE) {
		subResult = 0;
		
		// Depending on what we we're doing, we might step to a next mode
		if (selectedMode == M_NAME_LOOKUP) {
			
			// What did we intend to do here
			time(&testStartTime);				// Lookup not part of test
			selectedMode = wantedMode;
			engp->tpMode = selectedMode;
			engp->hostIP = engp->hostIPTab[0];	// Use first address
			
			if (selectedMode == M_QUERY_MASTER) {		// We want to get server list
				SetTestInfo("Kontaktar MasterServer");
				TrcLog(1, "Kontaktar MasterServer");
				
				// Check if we have an address to test for
				if (currentHostIdx <= (engp->numHostIP -1)) {

					// Setup The Enginerecord
					engp->wipeFlag = 1;
					strcpy(failInfo, "");
					engp->hostIP = engp->hostIPTab[currentHostIdx];
					engp->hostCtrlPort = TP_MASTER_PORT;
					
					StartClientContext(engp);
					sessActive = -1;
					currentHostIdx++;
				}
				else {
					engp->failCode = -1;
				}
			}
			else {
				SetTestInfo("Utfšr test");
				
				// Check if we have an address to test for
				if (currentHostIdx <= (engp->numHostIP -1)) {

					// Setup The Engine record
					engp->wipeFlag = 0;
					strcpy(failInfo, "");
					engp->hostIP = engp->hostIPTab[currentHostIdx];
					isReallyDoingTest = true;
					testPhase = 1;
					strcpy(failInfo, "");
					engp->bestTCPSendRate = engp->bestTCPRecvRate = 0;
					engp->bestUDPSendRate = engp->bestUDPRecvRate = 0;
					dispBestTCPSendRate = dispBestTCPRcvRate = 0;
					dispBestUDPSendRate = dispBestUDPRcvRate = 0;

					engp->nPackets = engp->sessionTime * engp->packetsPerSecond;
					engp->sessionMaxTime = engp->sessionTime * 3;
					currentMode = AdvanceTest(engp, selectedMode, CLM_NONE, TEST_RUNNING);

					engp->tpMode = currentMode;
					TrcLog(1, "Starting real test - submode %d", engp->tpMode);
					StartClientContext(engp);
					sessActive = -1;
					prevMode = currentMode;
				}
				else {
					engp->failCode = -1;
				}
			}
		}
		
		// Did we do a master query
		
		else if (selectedMode == M_QUERY_MASTER) {
			time(&testEndTime);					// Remember test end time
			DoneMasterServerQuery();
			currentMode = CLM_NONE;
			TrcLog(1, "Server list fetch complete");
		}
		
		// We were running a real test
		
		else {	
			ReportStats(engp);						// Show some info
			engp->sessionMaxTime = engp->sessionTime * 3;
			currentMode = AdvanceTest(engp, selectedMode, currentMode, subResult);
			
			if (currentMode == CLM_NONE) {			// We are free
				UpdateTestValues();
				ShowLoadResults();					// Show resulting ratios
				time(&testEndTime);					// Remember test end time
				
			} else {								// Run a new subtest
				UpdateTestValues();					// Show result panel
				TrcLog(1, "\nStarting session...");
			
				// For special phase counting during auto sessions
				
				if (currentMode != prevMode) testPhase = 0;
				prevMode = currentMode;
				testPhase += 1;
				engp->tpMode = currentMode;
				StartClientContext(engp);
			}
		}
		
	} else {			// Subtest failed
		subResult = TEST_ERROR;
		PrintMsg("Error in test");
	}
	sessActive = engp->active;
}

// -----------------------------------------------------------------------------------
// ContinueProgressBarFader - Fade the progress bar
// -----------------------------------------------------------------------------------
void 
TestWindow::ContinueProgressBarFader(void)
{	
	GWorldPtr			myWorld;
	GDHandle			myDevice;
	PixMapHandle		pxmap;
	PixPatHandle		ppat;
	RGBColor			fadeColor	= {0xffff, 0xffff, 0xffff};
	RGBColor			whiteColor	= {0xffff, 0xffff, 0xffff};
	RGBColor			blackColor 	= {0, 0, 0};
	CGrafPtr			op;

	if (mIsAnimationMode) {
		TrcLog(1, "Animationstep = %ld", mAnimStep);
		::GetPort(&op);
		::SetPort(::GetWindowPort(mTestWindow));
		
		// Save the world
		::GetGWorld(&myWorld, &myDevice);

		// Put Window background color into the resultWorld
		pxmap = ::GetGWorldPixMap(mResultWorld);
		
		if (::LockPixels(pxmap)) {
			::SetGWorld(mResultWorld, (GDHandle) NULL);
			(void) ::SetThemeBackground(kThemeBrushDialogBackgroundActive,16, true);
			::EraseRect(&mBarRect);
			::UnlockPixels(pxmap);
		}

		// Save the world
		pxmap = ::GetGWorldPixMap(mMaskWorld);
		
		if (::LockPixels(pxmap)) {
			::SetGWorld(mMaskWorld, (GDHandle) NULL);
			fadeColor.blue = fadeColor.green = fadeColor.red = mAnimStep;
			ppat = NewPixPat();
			// Could we make an empty pix pat handle?
			if (ppat) {
				::MakeRGBPat(ppat, &fadeColor);
				::EraseRect(&mBarRect);
				::FillCRect(&mBarRect, ppat);
				::DisposePixPat(ppat);
			}
			::UnlockPixels(pxmap);
		}

		::SetGWorld(myWorld, myDevice);
		
		// For Copybits
		::RGBForeColor(&blackColor);
		::RGBBackColor(&whiteColor);
		
		// make a faded progressbar image
		::CopyMask(::GetPortBitMapForCopyBits(mBarWorld),
			  		::GetPortBitMapForCopyBits(mMaskWorld),
			  		::GetPortBitMapForCopyBits(mResultWorld),
			  		&mBarRect, &mBarRect, &mBarRect);

		// place the faded progressbar image on screen
		::CopyBits(::GetPortBitMapForCopyBits(mResultWorld),
					::GetPortBitMapForCopyBits(::GetWindowPort(mTestWindow)),
					&mBarRect, &mAnimRect, srcCopy, NULL);

		// increase fadestep
		mAnimStep += 2500; 	// found this a nice fadestep
			
		// Done - found this to be a good endpoint in fading
		if (mAnimStep > 65000) {
			// Dispose GWorlds
			if (mBarWorld) {
				::DisposeGWorld(mBarWorld);
				mBarWorld = NULL;
			}
			if (mMaskWorld) {
				::DisposeGWorld(mMaskWorld);
				mMaskWorld = NULL;
			}
			if (mResultWorld) {
				::DisposeGWorld(mResultWorld);
				mResultWorld = NULL;
			}
			::RemoveEventLoopTimer(mTestWindowCBRef);
			DisposeEventLoopTimerUPP(mTestWindowCBHandler);
			mTestWindowCBRef = NULL;
			mTestWindowCBHandler = NULL;
			::DisposeControl(mProgressBarRef);
			mProgressBarRef = NULL;
			mInRunMode = false;
			::SetControlTitle(mStartButtonRef, kpStartText);
			mIsAnimationMode = false;
			::ActivateControl(mFlipperControl);
			::ActivateControl(mFlipperInfoControl);
			::SetPort(::GetWindowPort(mTestWindow));
			// Enable the popup menu
			::ActivateControl(mTestAdvanced);
			::DrawOneControl(mTestAdvanced);
			(void) ::SetThemeWindowBackground(mTestWindow, kThemeBrushDialogBackgroundActive, false);
			::EraseRect(&mAnimRect);
			StartIdleTimer();
			::SetPort(op);
			return;
		}
		
		// restore port
		::SetPort(op);
		::SetEventLoopTimerNextFireTime(mTestWindowCBRef,(kEventDurationMillisecond*30));
		
	}
}

// -----------------------------------------------------------------------------------
// EngineProcess - Process the and run the TPEngine
// -----------------------------------------------------------------------------------
void 
TestWindow::EngineProcess(void)
{	
	char		*cp;
	int			code;
	
	if (mIsAnimationMode) {			// We're in animation mode
		ContinueProgressBarFader();
	}
	else {							// Run the context
			
		RunClientContext(engp);
		if (!mIsOnX) {
			::IdleControls(mTestWindow);
		}
		
		// Update counters if we're not running master server query or lookup mode
		if ((selectedMode != M_NAME_LOOKUP) && (selectedMode != M_QUERY_MASTER)) {
			// Update the counters each .5 seconds
			if (TickCount()-showTick > 30) {
				UpdateTestValues();
				showTick = TickCount();
			}
		}
		
		// Do new subtest if necessary 
		if (engp->active != sessActive) {
			NextSubTest();
		}
			
		// Log to logwindow
		if (infoMsg[0]) {
			PrintMsg(infoMsg);
			infoMsg[0] = 0;
		}
		
		// Check if the test is done
		if (!engp->active) {		
			// Stop it
			StopContext(engp);
			mInRunMode = false;
			
			// Did we fail out here
			if (engp->failCode) {
				if (engp->failCode == TPER_BADPORTS) {
					code = CheckTestReply(engp);				// Look at TEST reply
					if (code >= 400 && code < 500) engp->failCode = TPER_SERVERBUSY;
					if (code >= 500 && code < 600) engp->failCode = TPER_SERVERDENY;
				}
				switch (engp->failCode) {
					case TPER_MASTERDENIED:	cp = "Masterservern avvisade hŠmtning av serverlista"; break;
					case TPER_MASTERBUSY:	cp = "Masterservern šverbelastad. Fšrsšk senare"; break;
					case TPER_BADWELCOME:	cp = "Felaktigt svar frŒn testservern"; break;
					case TPER_SERVERDENY:	cp = "Testservern avvisade testbegŠran. Fšrsšk senare"; break;
					case TPER_SERVERBUSY:	cp = "Testservern šverbelastad. Fšrsšk senare"; break;
					case TPER_TIMEOUT:		cp = "Timeout pŒ svar frŒn servern"; break;
					case TPER_USERABORT:	cp = "Stoppad av anvŠndaren"; break;
					case TPER_CONNECTFAIL:  if (wantedMode == M_QUERY_MASTER) {
												cp = "Kunde ej koppla upp mot masterservern";
											} else {
												cp = "Kunde ej koppla upp mot testservern";
											}
											break;
					default:				cp = "Kommunikationsfel"; break;
				}
				sprintf(failInfo, "Testen avbruten - %s (%d)", cp, engp->failCode);
				TrcLog(1, "%s", failInfo);
			}
			UpdateTestValues();
			engp->tpMode = wantedMode;
			if (wantedMode != M_QUERY_MASTER) SendStatMessage(engp);
			TrcClose();
			
			if (!mMasterServerContacted) ::DeactivateControl(mStartButtonRef);

			// OSX Cleanup
			if (mIsOnX) {
				::RemoveEventLoopTimer(mTestWindowCBRef);
				::DisposeEventLoopTimerUPP(mTestWindowCBHandler);
				mTestWindowCBRef = NULL;
				mTestWindowCBHandler = NULL;
				if (mTestProgressSheet) {
					::HideSheetWindow(mTestProgressSheet);
				 	::DisposeEventHandlerUPP(mSheetWinEvtHandler);
					mSheetWinEvtHandler = NULL;
					::ReleaseWindow(mTestProgressSheet);
					mTestProgressSheet = NULL;
				}
	
				// Enable the popup menu
				::ActivateControl(mTestAdvanced);
				::DrawOneControl(mTestAdvanced);
				StartIdleTimer();
			}
			
			// OS 9 Cleanup
			else {
				if (selectedMode == M_QUERY_MASTER) {
					TrcLog(1, "EngineProcess: cleanup OS9 run");
					::RemoveEventLoopTimer(mTestWindowCBRef);
					::DisposeEventLoopTimerUPP(mTestWindowCBHandler);
					mTestWindowCBRef = NULL;
					::HideControl(mProgressBarRef);
					::DisposeControl(mProgressBarRef);
					mProgressBarRef = NULL;
					mTestWindowCBHandler = NULL;
					::HideControl(mProgressBarRef);
					::SetControlTitle(mStartButtonRef, kpStartText);
					mIsAnimationMode = false;
					::ActivateControl(mFlipperControl);
					::ActivateControl(mFlipperInfoControl);
					// Enable the popup menu
					::ActivateControl(mTestAdvanced);
					::DrawOneControl(mTestAdvanced);
					StartIdleTimer();
				}
				else {
					TrcLog(1, "EngineProcess: start OS9 animation");
					if (SetupProgressBarFader()) {
						mIsAnimationMode=true;
						::SetEventLoopTimerNextFireTime(mTestWindowCBRef,(kEventDurationMillisecond*30));
					}
				}	
			}
		}
		else {
			::SetEventLoopTimerNextFireTime(mTestWindowCBRef,(kEventDurationMicrosecond*100));
		}
	}
}

// -----------------------------------------------------------------------------------
// SetupProgressBarFader - Setup the nice fading progressbar
// -----------------------------------------------------------------------------------
bool 
TestWindow::SetupProgressBarFader(void)
{
	GWorldPtr		myWorld;
	GDHandle		myDevice;
	QDErr			qdErr1, qdErr2;
	PicHandle		bar = NULL;
	PixMapHandle	pxmap;
	RGBColor		whiteColor = {0xffff, 0xffff, 0xffff};
	RGBColor		blackColor = {0, 0, 0};
	CGrafPtr		op;

	TrcLog(1, "SetupProgressBarFader() called");
 
 	// Porting
 	
	::GetPort(&op);
	::SetPort(::GetWindowPort(mTestWindow));
	::DrawOneControl(mProgressBarRef);
	
	// Setup fade animation
	
	mAnimStep = 0;
	
	// OK, setup the animationrect and drop the Control
	
	::GetControlBounds(mProgressBarRef, &mAnimRect);

	// Get the Rect
	mBarRect = mAnimRect;
	::OffsetRect(&mBarRect, -mBarRect.left, -mBarRect.top);
	
	// Get current settings
	::GetGWorld(&myWorld, &myDevice);
	
	// Create the port
	qdErr1 = ::NewGWorld(&mBarWorld, 16, &mBarRect, NULL, myDevice, 0); 

	// Could we
	if (qdErr1 == noErr) {
		pxmap = ::GetGWorldPixMap(mBarWorld);
		if (::LockPixels(pxmap)) {
			::SetGWorld(mBarWorld, (GDHandle) NULL);
			::EraseRect(&mBarRect);
			::UnlockPixels(pxmap);
			
			// restore world in focus
			::SetGWorld(myWorld, myDevice);
		}
	}
	
	// Copy to it
	::RGBForeColor(&blackColor);
	::RGBBackColor(&whiteColor);
	::CopyBits(::GetPortBitMapForCopyBits(::GetWindowPort(mTestWindow)),
				 ::GetPortBitMapForCopyBits(mBarWorld),
	 			 &mAnimRect, &mBarRect, srcCopy, NULL);
	
	// Create the port
	qdErr2 = ::NewGWorld(&mMaskWorld, 16, &mBarRect, NULL, myDevice, 0); 
	if (qdErr2 == noErr) {
		qdErr2 = ::NewGWorld(&mResultWorld, 16, &mBarRect, NULL, myDevice, 0); 
	}

	// Hide the progressbar
	::HideControl(mProgressBarRef);
	
	// Should we just go instead?
	if (qdErr1 != noErr || qdErr2 != noErr) {
		::SetPort(::GetWindowPort(mTestWindow));
		::RemoveEventLoopTimer(mTestWindowCBRef);
		::DisposeControl(mProgressBarRef);
		mProgressBarRef=NULL;
		::DisposeEventLoopTimerUPP(mTestWindowCBHandler);
		mTestWindowCBRef=NULL;
		mTestWindowCBHandler=NULL;
		mInRunMode=false;
		::ActivateControl(mFlipperControl);
		::ActivateControl(mFlipperInfoControl);
		::SetControlTitle(mStartButtonRef, kpStartText);
		// Enable the popup menu
		::ActivateControl(mTestAdvanced);
		::DrawOneControl(mTestAdvanced);
		TrcLog(1, "SetupProgressBarFader() failed");
		::SetPort(op);
		return false;
	}
	
	// Restore the port
	::SetPort(op);

	// Done ok
	TrcLog(1, "SetupProgressBarFader() ok");
	return true;

}

// -----------------------------------------------------------------------------------
// ShowCounters - Draw the test process data
// -----------------------------------------------------------------------------------
void 
TestWindow::ShowCounters(void)
{
	int				newCount	= 0;
	int				endCount	= 0;
	Rect			tmpRect;
	GWorldPtr		myWorld;
	GrafPtr			oldPort;
	GDHandle		myDevice;
	QDErr			qdErr;
	RGBColor		saveFore, saveBack;
	RGBColor		whiteColor	= {0xffff, 0xffff, 0xffff};
	RGBColor		blackColor	= {0, 0, 0};
	RGBColor		frmColor	= {0x8000, 0x8000, 0x8000};
	RGBColor		errColor	= {0xffff, 0x1000, 0x0000};
	RGBColor		okColor		= {0x0000, 0x0000, 0x0000};
	double			sendres, recvres;
	char			*modep, *totp;
	char			curBuf[80];
	char			endBuf[80];
	char			typeBuf[80];
	char			infoBuf[200];
	char			hostBuf[200];
	PixMapHandle	pxmap;

	if (mTestWindow == 0) goto done;

	// Save the world
	
	GetGWorld(&myWorld, &myDevice);
	GetForeColor(&saveFore);
	GetBackColor(&saveBack);
	GetPort(&oldPort);

	if (counterWorld == 0) {
	
		// Create offscreen port to create non-flickering counter updates
		
		counterRect = drawRect;
		OffsetRect(&counterRect, -counterRect.left, -counterRect.top);
		qdErr = NewGWorld(&counterWorld, 16, &counterRect, NULL, myDevice, 0);
		if (qdErr != 0) goto done;
		SetGWorld(counterWorld, 0);
		SetPort(counterWorld);
		
		// Set background aligned to actual pane position
		
		ApplyThemeBackground(kThemeBackgroundTabPane, &drawRect, kThemeStateActive, 16, true);
	}

	// Set up strings to be displayed
	
	PutOctets(infoBuf, &engp->hostIP);
	sprintf(hostBuf, "%s [%s]", engp->hostName, infoBuf);

	modep = "";
	totp = "";
	if (mInRunMode) {
		if (engp->tpMode == M_TCP_RECV) {
			strcpy(typeBuf, "Detta pass (bytes)");
			sprintf(infoBuf, "TCP-mottagning pass %d (%s)", testPhase, Long64ToString(engp->tcpBytes));
			modep = "Mottaget:";
			NumForm(curBuf, 12, (int) engp->stats.BytesRecvd);
			endCount = engp->tcpBytes;
		} else if (engp->tpMode == M_TCP_SEND) {
			strcpy(typeBuf, "Detta pass (bytes)");
			modep = "Skickat:";
			sprintf(infoBuf, "TCP-sŠndning pass %d (%s)", testPhase, Long64ToString(engp->tcpBytes));
			NumForm(curBuf, 12, (int) engp->stats.BytesSent);
			endCount = engp->tcpBytes;
		} else if (engp->tpMode == M_UDP_RECV) {
			strcpy(typeBuf, "Detta pass (paket)");
			modep = "Mottaget:";
			sprintf(infoBuf, "UDP-mottagning pass %d (%s/s)", testPhase, Long64ToString(engp->bitsPerSecond));
			NumForm(curBuf, 12, (int) engp->stats.PktsRecvd);
			endCount = engp->nPackets;
		} else {
			strcpy(typeBuf, "Detta pass (paket)");
			modep = "Skickat:";
			sprintf(infoBuf, "UDP-sŠndning pass %d (%s/s)", testPhase, Long64ToString(engp->bitsPerSecond));
			NumForm(curBuf, 12, (int) engp->stats.PktsSent);
			endCount = engp->nPackets;
		}
		totp = "Av totalt:";
		NumForm(endBuf, 12, endCount);
	} else {
		if (failInfo[0]) {
			strcpy(infoBuf, failInfo);
		} else {
			if (engp->startTime) {
				sprintf(infoBuf, "Testen avslutad");
			} else {
				sprintf(infoBuf, "Ingen test utfšrd");
			}
		}
		strcpy(typeBuf, "");
		strcpy(curBuf, "");
		strcpy(endBuf, "");
	}

	// Tune in to our offscreen world
	
	SetGWorld(counterWorld, 0);
	SetPort(counterWorld);

	// Grab the PixMap
	
	pxmap = ::GetGWorldPixMap(counterWorld);

	// Only draw in a locked pixmap, or we WILL trash memory

	if (::LockPixels(pxmap)) {
		
		tmpRect = counterRect;
		EraseRect(&tmpRect);
		RGBForeColor(&frmColor);
		PenSize(2,2);
		FrameRect(&tmpRect);
		tmpRect.top = tmpRect.bottom - 23;
		FrameRect(&tmpRect);
		PenSize(1,1);

		// Display the values in their correct positions
		// ONLY do this if we're actually doing a test
		
		if (isReallyDoingTest) {
			RGBForeColor(&okColor);

			SetDrawBase(5, 18);
			DrawInfoText(0, 0, 80, DRMODE_BOLD, "Server:");
			DrawInfoText(POS_AT_CURRENT, 0, 400, DRMODE_BOLD, hostBuf);

			// Display counters
			
			SetDrawBase(CC_XP, 35);
			DrawInfoText(0, 0, 120, 1, typeBuf);
			DrawInfoText(0, 15, 60, 0, modep);
			DrawInfoText(POS_AT_CURRENT, 15, 80, DRMODE_BOLD+DRJUST_RIGHT, curBuf);
			DrawInfoText(0, 30, 60, 0, totp);
			DrawInfoText(POS_AT_CURRENT, 30, 80, DRMODE_BOLD+DRJUST_RIGHT, endBuf);

			// Show best Send rates so far
			
			SetDrawBase(CS_XP, 35);
			DrawInfoText(0, 0, 190, 1, "SŠndning");
			
			strcpy(curBuf, "ej tillgŠngligt");
			if (engp->bestTCPSendRate) sprintf(curBuf, "%s", Int32ToString((int)(engp->bestTCPSendRate * 8.0)));
			DrawInfoText(0, 15, 70, 0, "BŠsta TCP: ");
			DrawInfoText(POS_AT_CURRENT, 15, 90, DRMODE_BOLD+DRJUST_RIGHT, curBuf);

			strcpy(curBuf, "ej tillgŠngligt");
			if (engp->bestUDPSendRate) sprintf(curBuf, "%s", Int32ToString((int)(engp->bestUDPSendRate * 8.0)));
			DrawInfoText(0, 30, 70, 0, "BŠsta UDP: ");
			DrawInfoText(POS_AT_CURRENT, 30, 90, DRMODE_BOLD+DRJUST_RIGHT, curBuf);

			// Show best Receive rates so far
			
			SetDrawBase(CR_XP, 35);
			DrawInfoText(0, 0, 190, 1, "Mottagning");
			
			strcpy(curBuf, "ej tillgŠngligt");
			if (engp->bestTCPRecvRate) sprintf(curBuf, "%s", Int32ToString((int)(engp->bestTCPRecvRate * 8.0)));
			DrawInfoText(0, 15, 70, 0, "BŠsta TCP: ");
			DrawInfoText(POS_AT_CURRENT, 15, 90, DRMODE_BOLD+DRJUST_RIGHT, curBuf);
			
			strcpy(curBuf, "ej tillgŠngligt");
			if (engp->bestUDPRecvRate) sprintf(curBuf, "%s", Int32ToString((int)(engp->bestUDPRecvRate * 8.0)));
			DrawInfoText(0, 30, 70, 0, "BŠsta UDP: ");
			DrawInfoText(POS_AT_CURRENT, 30, 90, DRMODE_BOLD+DRJUST_RIGHT, curBuf);

			// Show available bandwidths in percent

			SetDrawBase(5, 80);
			if ((dispBestTCPSendRate > 0.0 && dispBestUDPSendRate > 0.0)
			   || (dispBestTCPRcvRate > 0.0 && dispBestUDPRcvRate > 0.0)) {
			   
				DrawInfoText(0, 0, 190, 0, "TillgŠnglig bandbredd TCP/UDP:");
				if (dispBestTCPSendRate > 0.0 && dispBestUDPSendRate > 0.0) {
					sendres = ((dispBestTCPSendRate/dispBestUDPSendRate) * 100.0);
					if (sendres > 100.0) sendres = 100.0;
					sprintf(curBuf, "%02.1f %%", sendres);
					modep = "SŠndning:";
					DrawInfoText(POS_AT_CURRENT, 0, 80, 0, modep);
					DrawInfoText(POS_AT_CURRENT, 0, 80, 1, curBuf);
				}
			
				if (dispBestTCPRcvRate > 0.0 && dispBestUDPRcvRate > 0.0) {
					recvres = ((dispBestTCPRcvRate/dispBestUDPRcvRate) * 100.0);
					if (recvres > 100.0) recvres = 100.0;
					sprintf(curBuf, "%02.1f %%", recvres);
					modep = "Mottagning:";
					DrawInfoText(POS_AT_CURRENT, 0, 90, 0, modep);
					DrawInfoText(POS_AT_CURRENT, 0, 80, 1, curBuf);
				}
			}			
		}	

		// Show message at bottom
		
		SetDrawBase(5, 102);
		if (failInfo[0]) RGBForeColor(&errColor);		// Errors in RED
		DrawInfoText(0, 0, 400, 1, infoBuf);
		RGBForeColor(&okColor);					// Back to BLACK

		// Unlock them again
		::UnlockPixels(pxmap);
		
	}
	// Now copy our offscreen stuff onto our main window
	
	SetPort(GetWindowPort(mTestWindow));

	RGBForeColor(&blackColor);		// Restore before doing CopyBits
	RGBBackColor(&whiteColor);
	CopyBits(GetPortBitMapForCopyBits(counterWorld),
			  	 GetPortBitMapForCopyBits(GetWindowPort(mTestWindow)),
			  	 &counterRect, &drawRect, srcCopy, NULL);

	// Restore graphics and return
	
	SetPort(oldPort);
	SetGWorld(myWorld, myDevice);
		
done:
	TrcClose();
}


// -----------------------------------------------------------------------------------
// DrawInfoText - Draw text at position
// -----------------------------------------------------------------------------------
void 
TestWindow::DrawInfoText(
	int		xpos,
	int		ypos,
	int		width,
	int		mode,
	char	*msg)
{
	Rect		tmpRect;	
	CFStringRef	cfsRef;
	int			theFont, theJust;
	
	if (xpos != POS_AT_CURRENT) drawX = xpos;
	if (ypos != POS_AT_CURRENT) drawY = ypos;
	SetRect(&tmpRect, baseX + drawX, baseY + drawY - 15, baseX + drawX + width, baseY + drawY);
	TextMode(srcCopy);
	
	cfsRef = CFStringCreateWithCString(kCFAllocatorDefault, msg, kCFStringEncodingMacRoman);
	EraseRect(&tmpRect);
	theFont = kThemeSmallSystemFont;
	theJust = teJustLeft;
	if ((mode & DRMODE_MASK) != 0) theFont = kThemeSmallEmphasizedSystemFont;
	if ((mode & DRJUST_MASK) != 0) theJust = teJustRight;
	DrawThemeTextBox(cfsRef, theFont, kThemeStateActive, false, &tmpRect, theJust, (void*) NULL);
	CFRelease(cfsRef);

	drawX += width;
}

// -----------------------------------------------------------------------------------
// SetDrawBase - Set draw base position
// -----------------------------------------------------------------------------------
void 
TestWindow::SetDrawBase(
	int		x,
	int		y)
{
	baseX = x;
	baseY = y;
}

// -----------------------------------------------------------------------------------
// SetDrawPos - Set draw position
// -----------------------------------------------------------------------------------
void 
TestWindow::SetDrawPos(
	int		x,
	int		y)
{
	if (x != POS_AT_CURRENT) drawX = x;
	if (y != POS_AT_CURRENT) drawY = y;
}

// -----------------------------------------------------------------------------------
// NumForm - Format number as "10 233 333"
// -----------------------------------------------------------------------------------
void 
TestWindow::NumForm(
	char	*destp,
	int		numPos,
	int		value)
{
	int			i, len;
	char		*sp, *dp;
	char		tbuf[20];
	
	memset(destp, ' ', numPos);
	destp[numPos] = 0;
	sprintf(tbuf, "%d", value);
	len = strlen(tbuf);
	sp = tbuf + len - 1;
	dp = destp + numPos - 1;
	
	for (i = 0 ; len > 0 ; i++) {
		if ((i & 3) == 3) {
			*dp-- = ' ';
			continue;
		}
		*dp-- = *sp--;
		len -= 1;
	}
}

// -----------------------------------------------------------------------------------
// PutReportIntoClip - Puts the test report into the clipboard
// -----------------------------------------------------------------------------------
void TestWindow::PutReportIntoClip(void)
{
	ReportToClip(engp, wantedMode, testStartTime, testEndTime);

	return;
}


// -----------------------------------------------------------------------------------
// ShowLoadResults - 
// -----------------------------------------------------------------------------------
void 
TestWindow::ShowLoadResults(void)
{
	char	tbuf[200];
	
	dispBestTCPSendRate = engp->bestTCPSendRate;
	dispBestTCPRcvRate = engp->bestTCPRecvRate;
	dispBestUDPSendRate = engp->bestUDPSendRate;
	dispBestUDPRcvRate = engp->bestUDPRecvRate;
	
	if (mAdvancedMode) {
		double sendres, recvres;
		if (engp->bestTCPSendRate > 0.0 && engp->bestUDPSendRate > 0.0) {
			sendres = (((double)engp->bestTCPSendRate/(double)engp->bestUDPSendRate) * 100.0);
			if (sendres > 100.0) sendres = 100.0;
			sprintf(tbuf, "Available bandwidth TCP/UDP, send:    %02.1f %%", sendres);
			Report(tbuf);
		}
		if (engp->bestTCPRecvRate > 0.0 && engp->bestUDPRecvRate > 0.0) {
			recvres = (((double)engp->bestTCPRecvRate/(double)engp->bestUDPRecvRate) * 100.0);
			if (recvres > 100.0) recvres = 100.0;
			sprintf(tbuf, "Available bandwidth TCP/UDP, receive: %02.1f %%", recvres);
			Report(tbuf);
		}
	}

}


// -----------------------------------------------------------------------------------
// UserCancelledAction - User hit the cancel button
// -----------------------------------------------------------------------------------
void 
TestWindow::UserCancelledAction(void)
{
	// Stop it
	if (engp) {
		StopContext(engp);
		time(&testEndTime);					// Remember test end time
	}	
}

// -----------------------------------------------------------------------------------
// SetRunningTestMode - Set in the running mode, different on X and Classic w. carbon
// -----------------------------------------------------------------------------------
void 
TestWindow::SetRunningTestMode(void)
{
	Rect			boundsRect = { 0,0,72,528 };
	ControlRef		CancelButtonRef, rootControl;	
	Boolean			indefinite = true;
	EventTypeSpec	evtlist[] = {{kEventClassWindow, kEventWindowClose},
								 {kEventClassCommand, kEventCommandProcess}};

	// Disable the popup menu
	::DeactivateControl(mTestAdvanced);
	::DrawOneControl(mTestAdvanced);
	
	// Check if we are on X
	if (mIsOnX) {	
	
		// Create the ProgressSheet direcly [Resourcerer cant setup a sheet window yet]
		mTestProgressSheet = ::NewCWindow(NULL,&boundsRect, kpTestIsOn, false, kWindowSheetProc, (WindowRef) 0, false, NULL);
		if (!mTestProgressSheet) {
			// Fix: failed alert here
			::SysBeep(1);
			return;
		}
				
		// Set Window Attributes for Carbon Events Handling and closebox
		(void) ::ChangeWindowAttributes(mTestProgressSheet, kWindowStandardHandlerAttribute, NULL);

		// Set The correct Window background theme
		(void) ::SetThemeWindowBackground(mTestProgressSheet, kThemeBrushDialogBackgroundActive, true);
		
		// Add controlitems to the TestWindow
		// [Root control]
		::CreateRootControl(mTestProgressSheet, &rootControl);
		
		// Add controlitems to the Progress sheet
		// [Cancel Button]
		CancelButtonRef = ::GetNewControl(kSheetCancelButton, mTestProgressSheet);
		::SetControlCommandID(CancelButtonRef, kCommandClose);
		::SetWindowCancelButton(mTestProgressSheet, CancelButtonRef);
		::EmbedControl(CancelButtonRef, rootControl);
			
		// [ProgressBar]
		mProgressBarRef = ::GetNewControl(kSheetProgressBar, mTestProgressSheet);
		::SetControlData(mProgressBarRef,
						 kControlNoPart,
						 kControlProgressBarIndeterminateTag,
						 sizeof (indefinite),
						 (Ptr)&indefinite);
		::EmbedControl(mProgressBarRef, rootControl);
		
		
		// [infotext]
		mProgInfoText = ::GetNewControl(kSheetInfoText, mTestProgressSheet);
		::EmbedControl(mProgInfoText, rootControl);

		
		// Add EventHandler to the Window
		mSheetWinEvtHandler = ::NewEventHandlerUPP(ProgressSheetEventHandler);
		::InstallWindowEventHandler(mTestProgressSheet, mSheetWinEvtHandler, 2, evtlist, this, NULL);

		// Show it
		::ShowSheetWindow(mTestProgressSheet, mTestWindow);
		::DrawControls(mTestProgressSheet);
		
	}
	else {
		mInRunMode=true;
		mProgressBarRef = ::GetNewControl(kTestWindowProgressBar, mTestWindow);
		::SetControlData(mProgressBarRef,
						 	kControlNoPart,
				 			kControlProgressBarIndeterminateTag,
				 			sizeof (indefinite),
				 			(Ptr)&indefinite);
		::EmbedControl(mProgressBarRef, mRootControl);
		::SetControlTitle(mStartButtonRef, kpCancelText);
		::DeactivateControl(mFlipperControl);
		::DeactivateControl(mFlipperInfoControl);
		::DrawOneControl(mProgressBarRef);
	}
}

// ***********************************************************************************
//
//								NON Class methods and callbacks
//
// ***********************************************************************************


// -----------------------------------------------------------------------------------
// ControlFocusEventHandler - Event handler for control focus changed event
// -----------------------------------------------------------------------------------
static pascal OSStatus ControlFocusEventHandler(
	EventHandlerCallRef myHandler,
	EventRef	event,
	void		*userData)
{
	ControlRef	control, cfocus;
	OSStatus	status;
	char		key;
	TestWindow	*tw = (TestWindow*) userData;
	
	myHandler;				// Touch
	
	switch(::GetEventClass(event)) {

	case kEventClassControl:
		status = ::GetEventParameter(event, kEventParamDirectObject, typeControlRef, NULL, sizeof(control), NULL, &control);
		if (status == noErr) {
			::GetKeyboardFocus(tw->GetWindow(), &cfocus);
			if (cfocus != control) {
				tw->HandleFocusChange(control, cfocus);
			}
		}
		break;
	
	case kEventClassKeyboard:
		status = ::GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(key), NULL, &key);
		if (status == noErr && key == 0x09) {
			tw->HandleSetFocus();
		}
		break;
	}
		
	// Done
	return eventNotHandledErr;
}

// -----------------------------------------------------------------------------------
// ControlChangedEventHandler - Event handler for control value changed for the Server Popup
// -----------------------------------------------------------------------------------
static pascal OSStatus ControlChangedEventHandler(
	EventHandlerCallRef myHandler,
	EventRef 	event,
	void		*userData)
{
	TestWindow	*tw = (TestWindow*) userData;

	myHandler, event;		// Touch unused
	
	// Show it
	tw->SetupServerAndPort();
		
	// Done - just pass it on
	return eventNotHandledErr;
}


// -----------------------------------------------------------------------------------
// TooMuchText - Checks max text len for the passed editfield
// -----------------------------------------------------------------------------------
Boolean TooMuchText(
	ControlRef	control,
	SInt16		charCode)
{
	char		usertext[512];
	Size		actsize;
	bool		refuseKey = false;
	SInt32		ref = 0, len = 0, sellen = 0;
	ControlEditTextSelectionRec		selrec;
	
	// Check for backspace
	if (charCode != 8) {
		// Get the length of the text in the current control
		ref=(SInt32) GetControlReference(control);
		GetControlData(control, 0, kControlEditTextTextTag, 511, &usertext, &len);
		GetControlData(control, 0, kControlEditTextSelectionTag, sizeof(ControlEditTextSelectionRec), &selrec, &actsize);
		sellen = (selrec.selEnd - selrec.selStart);
		
		// Check the length
		if ((len - sellen)  >= ref)  {
			refuseKey = true;
		}
		
		// Done
	}
	
	// Done
	return refuseKey;
}


// -----------------------------------------------------------------------------------
// HostnameKeyFilter - Hostname Filter for keyboard Events for EditText
// -----------------------------------------------------------------------------------
pascal SInt16 HostnameKeyFilter(
	ControlRef			control,
	SInt16*				keyCode,
	SInt16*				charCode,
	EventModifiers* 	modifiers)
{
	keyCode, modifiers;		// Touch unused
	
	// Check max size
	if (TooMuchText(control, *charCode)) {
		SysBeep(1);
		return kControlKeyFilterBlockKey;
	}
	
	// Normal keys
	if ((char)*charCode >= '0' && (char)*charCode <='9') {
		return kControlKeyFilterPassKey;
	}
	if ((char)*charCode >= 'A' && (char)*charCode <='Z') {
		return kControlKeyFilterPassKey;
	}
	if ((char)*charCode >= 'a' && (char)*charCode <='z'){
		return kControlKeyFilterPassKey;
	}

	switch (*charCode) {
		case '-':
		case '.':
		case '_':
		case 0x1c:	// kLeftArrow
		case 0x1d:	// kRightArrow
		case 0x08:	// kBackspace
			return kControlKeyFilterPassKey;
			
		default:	
			SysBeep(1);
			break;	
	}	

	// Invalid key
	return kControlKeyFilterBlockKey;
}


// -----------------------------------------------------------------------------------
// NumericKeyFilter - Numeric Filter for keyboard Events for EditText
// -----------------------------------------------------------------------------------
pascal SInt16 NumericKeyFilter(
	ControlRef			control,
	SInt16*				keyCode,
	SInt16*				charCode,
	EventModifiers* 	modifiers)
{	
	keyCode, modifiers;		// touch unused
	
	// Check max size
	if (TooMuchText(control, *charCode)){
		SysBeep(1);
		return kControlKeyFilterBlockKey;
	}

	// Normal keys
	if ((char)*charCode >= '0' && (char)*charCode <='9') {
		return kControlKeyFilterPassKey;
	}

	switch (*charCode) {
		case 0x1c: 		// kLeftArrow
		case 0x1d:		// kRightArrow
		case 0x08:		// kBackspace
			return kControlKeyFilterPassKey;
			
		default:	
			SysBeep(1);
			break;	
	}	

	// Invalid key
	return kControlKeyFilterBlockKey;
}

// -----------------------------------------------------------------------------------
// TestWindowEventHandler - Event handler for the Testwindow
// -----------------------------------------------------------------------------------
static pascal OSStatus TestWindowEventHandler(
	EventHandlerCallRef myHandler,
	EventRef	event,
	void		*userData)
{
	// Locals
	OSStatus		result = eventNotHandledErr;
	HICommand  		aCommand;
	TestWindow		*tw = (TestWindow*) userData;
	
	myHandler;			// Touch unused
	
	switch (GetEventClass(event)) {

	case kEventClassCommand:
	
		switch (GetEventKind(event)) {
			
		case kEventCommandProcess:
		
			// Get the Event Parameter
			GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
		    
			// What Command
			switch (aCommand.commandID) {
			
			case kCommandRun:
				if (tw->InAnimationLoop()) {
					break;
				}
				
				switch(tw->InRunMode()) {
						case false:
							tw->ExecuteTest();
							break;
							
						case true:
							tw->UserCancelledAction();
							break;
				}
				result = noErr;
				break;
		      
		    case kCommandFlip:
		    	tw->FlipAdvancedMode();
				result = noErr;
				break;
		    
		    case kTypePopup:
		    	tw->SetupAdvancedTestMode();
				result = noErr;
				break;
		    	
		    case kServerFlip:
		    	tw->SetupServerAndPort();
				result = noErr;
				break;
		    	
		    case 'test':
		    	tw->PutReportIntoClip();
		    	result = noErr;
		    	break;
		            
		    default:
				break;
			}
		}
		break;


	case kEventCommandUpdateStatus:
		// Get the Event Parameter
		GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	    
	   // Check Command id
		switch (aCommand.commandID) {
        case 'test':
			if (tw->GetWindow() != 0) {
				EnableMenuItem(aCommand.menu.menuRef, aCommand.menu.menuItemIndex);
			}
			else {
				DisableMenuItem(aCommand.menu.menuRef, aCommand.menu.menuItemIndex);
			}
        	result = noErr;
        	break;
        					        
		default:
			break;
		}
		break;
			                

	case kEventClassWindow:
		switch(	GetEventKind(event)) {
		
	    case kEventWindowClose:
			::QuitApplicationEventLoop();
        	result = noErr;
        	break;

	    case kEventWindowDrawContent:
    		::DrawControls(tw->GetWindow());
    		tw->UpdateTestValues();
        	result = noErr;
    		break;
	    	
	    case kEventWindowBoundsChanging:
    		if (tw->IsFlipMode()) {
    			::DrawControls(tw->GetWindow());
    		}
	   		break;
	 
	    case kEventWindowGetClickActivation:
	 		tw->ActivateWithSheet();
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


// -----------------------------------------------------------------------------------
// ProgressSheetEventHandler - Event handler for the Progresssheet
// -----------------------------------------------------------------------------------
static pascal OSStatus ProgressSheetEventHandler(
	EventHandlerCallRef myHandler,
	EventRef	event,
	void		*userData)
{
	// Locals
	OSStatus	result = eventNotHandledErr;
	HICommand  	aCommand;
	TestWindow	*tw = (TestWindow*) userData;

	myHandler;		// Touch unused
	
	switch(::GetEventClass(event)) {
	
	case kEventClassCommand:
		if (::GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand) == noErr) {
			switch (aCommand.commandID) {
			case kCommandClose:
				tw->UserCancelledAction();
	        	result = noErr;
	        	break;
		        
			default:
		      	break;
		    }
		}
		break;
		
	case kEventClassWindow:
		switch(::GetEventKind(event)) {
	   	case kEventWindowClose:
			tw->UserCancelledAction();
			result = noErr;
			break;
				
		default:
			break;
	    }
	    break;
	 
	default:
		break;
	}
	
	// Done
    return result;
}

// -----------------------------------------------------------------------------------
// TestWindowIdleCallBack - Called from the Carbon Timer, to handle widget update on CarbonLib
// -----------------------------------------------------------------------------------
pascal void TestWindowIdleCallBack(
	EventLoopTimerRef unused,
	void		*userData)
{
	TestWindow	*tw = (TestWindow*) userData;
	
	unused;		// Touch unused
	
	// If we have a testwindow
	if (tw->GetWindow()) {
		// update cursors
		IdleControls(tw->GetWindow());
		if (!tw->IsMasterTried()) {
			tw->StartMasterServerQuery();
		}		
	}
}

// -----------------------------------------------------------------------------------
// TestWindowWorkCallBack - Called from the Carbon Timer, to handle TPEngine tickle time
// -----------------------------------------------------------------------------------
static pascal void TestWindowWorkCallBack(
	EventLoopTimerRef unused,
	void		*userData)
{
	TestWindow	*tw = (TestWindow*) userData;
	
	unused;		// Touch unused
	
	// Do the actual Stuff here
	tw->EngineProcess();
}

// -----------------------------------------------------------------------------------
// AlertSheetEventHandler - Event handler for the ErrorAlertSheet
// -----------------------------------------------------------------------------------
static pascal OSStatus AlertSheetEventHandler(
	EventHandlerCallRef myHandler,
	EventRef	event,
	void		*userData)
{
	OSStatus	result = eventNotHandledErr;
	HICommand  	aCommand;
	TestWindow	*tw = (TestWindow*) userData;

	myHandler;		// Touch unused
	
	switch (GetEventClass(event)) {

	case kEventClassCommand:
		if (::GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand) == noErr) {

			switch (aCommand.commandID) {

			case kCommandClose:
				tw->CloseErrorSheet();
		      result = noErr;
		      break;
		        
		  	default:
		      break;
		    }
		}
		break;
				 
	 default:
	 	break;
	}
	
	// Done
    return result;
}

extern "C" {

// -----------------------------------------------------------------------------------
// PutIntoClipGlue - Glue routine to be able to call PutReportIntoClip from plain C world
// -----------------------------------------------------------------------------------

int PutIntoClipGlue(void)
{
	if (gMainTW != 0) gMainTW->PutReportIntoClip();
	return 7;
}

}

