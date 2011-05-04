/*
 *	
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * macgui_main.h - Header for main Mac application class
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

#ifndef __MACGUI_MAIN_H__
#define __MACGUI_MAIN_H__

// Includes
#include <Carbon.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// C style headerfiles included
#ifdef _cplusplus
extern "C" {
#endif
	#include "tpengine.h"
	#include "macgui_statwin.h"
#ifdef _cplusplus
}
#endif

// used classes
class TestWindow;
class AboutWindow;

// The Application class
class TPTestCarbon {
public:
			// Con and Destructor
										TPTestCarbon();
			virtual					~TPTestCarbon();

		
			// action
			OSStatus					Launch(void);				// Do application "boot strap sequence"
			void						DoAboutBox(void);			// Show the aboutbox
			void						MakeWindowShowLoadResults(void); // Show results during test
			void						ShowHelpWindow(void);	// Shows the Help window
			void						HelpBoundsChanged(void); // Called to livresize the helpwindow
			
			// getters & setters
			AboutWindow*			GetAboutWindow(void) { return mAbout; }
			TestWindow*				GetTestWindow(void) { return mTW; }
			bool						HasAboutWindow(void) { return (mAbout!=NULL); }
			bool						HasHelpWindow(void) { return (mHelpWindow!=NULL); }
			void						AboutClosed(void) { mAbout=NULL; }
			void						MakeWindowShowLoadResult(void);
			void						CleanHelp(void)	{ 	::ReleaseWindow(mHelpWindow);
																	mHelpWindow = NULL;
																}
									
private:
			// members
			unsigned long			mCarbonLibVersion;		// What version of CarbonLib
			EventHandlerUPP 		mAppCommandProcess;  	// Application Event handler
			short						mBarID;						// MenubarID & OSX or not information
			TestWindow*				mTW;							// The testwindowclass ptr
			AboutWindow*			mAbout;						// The abouutwindowclass ptr
			WindowRef				mHelpWindow;				// Window containing Help Info
			EventHandlerUPP		mHelpWinEvtHandler;		// EventHandler for the Help Window
			ControlRef				mHelpBox;					// Reference to the Help text box
			
			// methods
			OSStatus					Initialize(void);					// Initialize System stuff
			bool 						RunningOnCarbonX(void); 		// Returns true if we run on X
			void						DoOldCarbonLibAlert(void);		// Shows the "Too old carbonlib Alert"
			OSStatus 				MakeMenu(void);					// Create menues correctly on 8.6, 9 & X
			OSStatus					InstallAppEvents(void);			// Install the application event handlers

		


};

#endif
