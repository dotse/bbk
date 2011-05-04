/*
 *	
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * macgui_testwindow.h - Header for Mac test progress window
 *
 * Written by
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

#ifndef __TESTWINDOW__H__
#define __TESTWINDOW__H__

// Includes
#include <Carbon.h>
#include "macgui_resourcedefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "tpengine.h"
#include "macgui_statwin.h"

#ifdef __cplusplus
}
#endif /* __cplusplus */

// The TestWindowClass
class TestWindow {
public:

		// Con & Destructor
									TestWindow(bool inIsX);
		virtual					~TestWindow();
		
		
		//  -- public methods --
		
		// actions
		void						HandleSetFocus(void);
		void						ExecuteTest(void);
		void						HandleFocusChange(ControlRef inControl, ControlRef focusControl);
		void						SetupAdvancedTestMode(void);
		void						FlipAdvancedMode(void);
		void						UpdateTestValues(void);
		void						ActivateWithSheet(void);
		void						CloseErrorSheet(void);
		void 						DoneMasterServerQuery(void);
		void						StartMasterServerQuery(void);
		void						EngineProcess(void);
		void 						NextSubTest(void);
		void						UserCancelledAction(void);
		void						SetupServerAndPort(void);
		void						ShowLoadResults(void);
		void						PutReportIntoClip(void);
		
		// getters and setters
		WindowRef				GetWindow(void) { return mTestWindow; }
		bool						InAnimationLoop(void) { return mIsAnimationMode; }
		int						GetAdvancedTestMode(void) { return mAdvancedTestMode; }
		bool						InRunMode(void) { return mInRunMode; }
		bool						IsFlipMode(void) { return mInFlipMode; }
		bool						IsOK(void) { return mIsOK; }
		bool						IsMasterTried(void) { return (mMasterServerContacted || masterTried) ; }
		
private:
		// members
		EventHandlerRef		mEvhRef;						// Reference to the installed eventhandler
		EventHandlerUPP		mFocusHandler;				// Reference to evthandler for control focus
		EventHandlerUPP		mServerChangedHandler;	// Reference to the eventhandler for the Server Popup
		EventHandlerUPP		mTestWinEvtHandler;		// Testwindow Event handler
		EventHandlerUPP		mAlertSheetEvtHandler;	// ErrorAlertSheet Event handler
		EventHandlerUPP		mSheetWinEvtHandler;		// EventHandler for the ProgressSheet (X only)
		
		ControlKeyFilterUPP	mHostnameKeyFilterUPP;	// UPP to the hostname keyfilter for EditText controls
		ControlKeyFilterUPP	mNumericKeyFilterUPP;	// UPP to the numeric keyfilter for EditText controls
		
		EventLoopTimerUPP		mTestWindowCBHandler;	// Timer handling repetetive tasks for the TestWindow
		EventLoopTimerRef		mTestWindowCBRef;			// Reference to the repetetive tasks timer

		EventLoopTimerUPP		mTestWindowIdleHandler;	// Timer handling cursor blinkings for the TestWindow on OS 8.6 & 9
		EventLoopTimerRef		mTestWindowIdleRef;		// Reference to the cursor blinkings for the TestWindow on OS 8.6 & 9

		WindowRef				mTestWindow;				// The window reference
		WindowRef				mErrorAlertSheet;			// The Alert error sheet reference
		WindowRef				mTestProgressSheet;		// The testprogress sheet reference (X only)
		
		ControlRef				mRootControl;				// The Rootcontrol in the control hierachy
		ControlRef				mProgressBarRef;			// The Progress sheet progress bar
		ControlRef				mTestServerPopupRef;		// The Server popup ref
		ControlRef				mTestMethodPopupRef;		// The Method popop ref
		ControlRef				mStartButtonRef;			// Start button
		ControlRef				mFlipperControl;			// page flipper (disclosure triangle)
		ControlRef				mFlipperInfoControl;		// Page flipper info
		ControlRef				mServerEditField;			// Advanced mode Server Edit field
		ControlRef				mPortEditField;			// Advanced mode Port Edit field
		ControlRef				mDatarateEditField;		// Advanced mode datarate Edit field
		ControlRef				mPacksizeEditField;		// Advanced mode packet size Edit field
		ControlRef				mPacketrateEditField;	// Advanced mode packet rate Edit field
		ControlRef				mTimeEditField;			// Advanced mode test time Edit field
		ControlRef				mTCPBytesField;			// Advanced mode test tcp bytes Edit field
		ControlRef				mTestAdvanced;				// Advanced mode group control
		ControlRef				mTestTypePopup;			// Advanced Test type popup button
		ControlRef				mTestTCPGrp;				// Advanced mode TCP group
		ControlRef				mTestUDPGrp;				// Advanced mode UDP group
		ControlRef				mCurrentFocusControl;	// Ptr to the textfontrol in focus
		ControlRef				mProgInfoText;				// Tells user what we're currently doing
		
		// For fade animation
		long						mAnimStep;					// Fade animationsteps
		GWorldPtr				mBarWorld;					// The Progressbar
		GWorldPtr				mMaskWorld;					// Fade mask
		GWorldPtr				mResultWorld;				// Fade result
		Rect						mAnimRect;					// Rect for animations
		Rect						mBarRect;					// gWorldrect
		
		// Data for the real TPEngine
		TPEngine					*engp;						// The TPTest Engine data Ptr
		int						selectedMode;				// The selected mode
		int						currentMode;				// The current submode
		int						wantedMode;					// The Mode we want after the Name lookup;
		bool						masterTried;				// We have tried to contact the master server
		bool						mMasterServerContacted;	// Master server has been contacted successfully
		bool						mInRunMode;					// We are running now
		bool						mAdvancedMode;				// In advanced Mode
		bool						mInFlipMode;				// Doing window trasition
		int						mAdvancedTestMode;		// Which testmode is selected
		int						sessActive;					// Active session id
		long						showTick;					// The show ticks for status update
		int						currentHostIdx;			// Index for HostIPs looked up, and used.
		bool						isReallyDoingTest;		// Are testing or just namelookupping ;-)
		time_t					testStartTime;		
		time_t					testEndTime;
		int						testPhase;
		int						prevMode;
		char						failInfo[200];
		char						infoMsg[200];
		
		// Other good to have stuff
		bool						mIsOnX;						// Running on OSX		
		bool						mIsOK;						// Did setup correctly
		bool						mIsAnimationMode;			// The OS9 animationloop running

		// For Drawing 
		int						adjustDone,drawX, drawY, baseX, baseY; // Its drawing stuff, dont bother
		Rect						drawRect;					// Where to draw in the testwindow
		double 					dispBestTCPSendRate;		// Display value
		double 					dispBestTCPRcvRate;		//		-"-
		double 					dispBestUDPSendRate;		// 	-"-
		double 					dispBestUDPRcvRate;		// 	-"-
		Rect						counterRect;				// Bounds for offscreen world
		GWorldPtr				counterWorld;				// Ptr to our offscreen world
		
		// methods
		OSStatus					Finalize(void);	
		OSStatus					CreateWindow(int hPos);
		bool						HandleValueChanges(void);
		void 						ShowErrorAlertSheet(char* theErrorMessage);
		void						ShowCounters(void);
		void						DrawInfoText(int xpos, int ypos, int width, int mode, char *msg);
		void						SetDrawPos(int x,int y);
		void						SetDrawBase(int x, int y);
		void						NumForm(char* destp,int numPos,int value);
		void						SetRunningTestMode(void);
		void						SetTestInfo(char* msgtext);
		void						StartIdleTimer(void);
		void						StopIdleTimer(void);
		bool						SetupProgressBarFader(void);
		void 						ContinueProgressBarFader(void);
		void						Sys86Fix(void);
};

void 	ExternalShowLoadResults();
#endif
		
		
		
		


