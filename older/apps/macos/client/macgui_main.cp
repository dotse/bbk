/*
 *	
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * macgui_main.cp - main GUI routines for Mac platform
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

#include "macgui_main.h"
#include "macgui_testwindow.h"
#include "macgui_about.h"
#include "macgui_resourcedefs.h"

// Callbacks that have to be pure C
extern "C" {
	static pascal OSErr QuitAppleEventHandler(const AppleEvent *appleEvt, AppleEvent* reply, long refcon);
	static pascal OSStatus AppCommandHandler(EventHandlerCallRef nextHandler, EventRef theEvent, void* userData);
	static pascal OSStatus HelpEventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData);
}	

// Global, just to let tpcommon draw
TPTestCarbon* application = NULL;

// --------------------------------------------------------
// Entry point - main
// --------------------------------------------------------
int main(void)
{	
	// Locals
	OSStatus	status;
	
	// Toolbox needs this [This loads the Carbon.framework/CarbonLib]
	::InitCursor();

	// We want to be appearance savvy
	(void) ::RegisterAppearanceClient();
	
	// Create Application Object
	application = new TPTestCarbon();
	
	// Check that we could create thye application object
	if ( !application ) {
		return 1; 
	}	
		
	// Initializations
	status = application->Launch();
	
	// Check status
	if ( status == noErr ) {
    	// Run the Application event loop
    	::RunApplicationEventLoop();
	
	   // Done	
   		(void) ::UnregisterAppearanceClient();
	}
   
   // Shut down test engine
   delete application;
       
   // Done 
   return 0;
}
// --------------------------------------------------------
// TPTestCarbon [constructor]
// --------------------------------------------------------
TPTestCarbon::TPTestCarbon()
{
	// Initialize class members	
	mAbout=NULL;
	mTW =NULL;
	mHelpWindow=NULL;
	
}

// --------------------------------------------------------
// ~TPTestCarbon [destructor]
// --------------------------------------------------------
TPTestCarbon::~TPTestCarbon()
{
	// Destruct any objects created
	if ( mTW  ) {
		delete mTW;
	}
	if ( mAbout ) {
		delete mAbout; 
	}
}

// -----------------------------------------------------------------------------------
// Launch from TPTestCarbon
// -----------------------------------------------------------------------------------
OSStatus
TPTestCarbon::Launch(void)
{
	// Locals
	OSStatus status;
	
	// Initializations
	status = Initialize();
	
	// Check status
	if ( status == noErr ) {
	
		// Build menus
    	status = MakeMenu();
   }
   else if ( status == kBadCarbonLibVersion ) {
   	DoOldCarbonLibAlert();
   }

	// Check status
	if ( status == noErr ) {
		// Install CarbonEvent handlers
    	status = InstallAppEvents();
    }
    
	// Check status
	if ( status == noErr ) {
	
		// Create Test Window
		mTW = new TestWindow( RunningOnCarbonX());
		
		if ( mTW == NULL ) {	
			status = -1;
		}
		
		if ( status == noErr ) {
			if ( mTW->IsOK() == false ) {
				status = -1;
			}
		}
    }
    
    return status;
}

// -----------------------------------------------------------------------------------
// Initialize toolbox
// -----------------------------------------------------------------------------------
OSStatus
TPTestCarbon::Initialize(void)
{
	// Locals
	long 		response;
	OSStatus 	err;
	
	// Track what version of CarbonLib we do run under
	if ( RunningOnCarbonX() ) {
		mCarbonLibVersion=0x1000;
	}
	else {
		err = ::Gestalt(gestaltCarbonVersion, &response);
		if (err == noErr) {
			mCarbonLibVersion = response;
			// Check to old version
			if ( mCarbonLibVersion < kMinimumCarbonLib ) {
				return kBadCarbonLibVersion;
			}

		}
		else {
			mCarbonLibVersion=0; // unknown
			return kBadCarbonLibVersion;
		}
	}
	
	// Register Quit AE handler, and return the result
   return (OSStatus) ::AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, NewAEEventHandlerUPP(QuitAppleEventHandler), 0, false);
	
}

// -----------------------------------------------------------------------------------
// RunningOnCarbonX - Is this X?
// -----------------------------------------------------------------------------------
bool 
TPTestCarbon::RunningOnCarbonX(void)
{
    // Locals
    UInt32 response;

    return ( ::Gestalt(gestaltSystemVersion, (SInt32 *) &response) == noErr) && (response >= 0x01000);
}

// -----------------------------------------------------------------------------------
// DoOldCarbonLibAlert - The user have a too old version of carbonlib installed
// -----------------------------------------------------------------------------------
void 
TPTestCarbon::DoOldCarbonLibAlert(void)
{
	// Show it
	::StopAlert(kBadCarbonLibVersionAlert, NULL );
	::ResetAlertStage();
}

// -----------------------------------------------------------------------------------
// MakeMenu - handled OS9/X differences in menu layout
// -----------------------------------------------------------------------------------
OSStatus 
TPTestCarbon::MakeMenu(void)
{
	// Locals
	Handle				menuBar;
	long 				response;
	OSStatus			err = noErr;
	MenuRef				hMenuRef;
	UInt16				hMenuItems;
	
	// Are we running on 8/9 or X?
	err = ::Gestalt(gestaltMenuMgrAttr, &response);
	if ((err == noErr) && (response & gestaltMenuMgrAquaLayoutMask)) {
		mBarID=kOSXMenuBar;
	}
	else {
		mBarID=kOS9MenuBar;
	}

	// Load menubar resource
	menuBar = ::GetNewMBar(mBarID);
	
	// Did we get the menubar
	if (menuBar) {
		::SetMenuBar(menuBar);
		
		// Draw the menubar
		::DrawMenuBar();
		
		// Add the Help entry to the help menu
		::HMGetHelpMenu(&hMenuRef, NULL );
		if ( hMenuRef ) {
			hMenuItems = ::CountMenuItems(hMenuRef)+1;
			::MacAppendMenu(hMenuRef, "\pBruksanvisning..." );
			::SetMenuItemCommandID(hMenuRef, hMenuItems, 'Hlp!');
			::MacAppendMenu(hMenuRef, "\pKontaktinfo..." );
			hMenuItems++;
			::SetMenuItemCommandID(hMenuRef, hMenuItems, 'Kon!');
		}
		
		
		// Done OK
		return noErr;
	}
	else {
		return -1;
	}
}


// -----------------------------------------------------------------------------------
// InstallAppEvents - The needed CE handlers
// -----------------------------------------------------------------------------------
OSStatus 
TPTestCarbon::InstallAppEvents(void)
{
	// Locals
	EventTypeSpec  eventType[] =	{ 	{ kEventClassCommand, kEventCommandProcess},
										{ kEventClassApplication, kEventAppFrontSwitched},
										{ kEventClassCommand, kEventCommandUpdateStatus}
									};

	// Create an UPP to the Eventhandler function
   mAppCommandProcess = ::NewEventHandlerUPP(AppCommandHandler);
    
   // Install the Event handler
	return ::InstallApplicationEventHandler(mAppCommandProcess, sizeof(eventType)/sizeof(EventTypeSpec), eventType, this, NULL);
}

// -----------------------------------------------------------------------------------
// DoAboutBox - show the aboutbox
// -----------------------------------------------------------------------------------
void
TPTestCarbon::DoAboutBox(void)
{
	mAbout = new AboutWindow(this);
}

// -----------------------------------------------------------------------------------
// MakeWindowShowLoadResult - show the current results, pushed from tpcommon.
// -----------------------------------------------------------------------------------
void
TPTestCarbon::MakeWindowShowLoadResult(void)
{
	// Do we have a window
	if ( mTW == NULL ) {
		return;
	}
	
	mTW->ShowLoadResults();
	
}

// -----------------------------------------------------------------------------------
// ShowHelpWindow - show the help window
// -----------------------------------------------------------------------------------
void
TPTestCarbon::ShowHelpWindow(void)
{
	// Locals
	ControlRef	root;
	
	// Needed even specs
	EventTypeSpec	evtlist[] = {	{kEventClassWindow, kEventWindowClose},
											{kEventClassWindow, kEventWindowBoundsChanged} };

	if (HasHelpWindow()) {
		::BringToFront(mHelpWindow);
		return;
	}

	// Create the HelpWindow from the resource
	mHelpWindow = ::GetNewCWindow(kHelpWindow, NULL, (WindowRef) -1);
	if ( !mHelpWindow ) {
		::SysBeep(1);
		return;
	}

	// Set Window Attributes for Carbon Events Handling and closebox
	(void) ::ChangeWindowAttributes(mHelpWindow, kWindowStandardHandlerAttribute , kWindowFullZoomAttribute );

	// Set The correct Window background theme
	(void) ::SetThemeWindowBackground(mHelpWindow, kThemeBrushDialogBackgroundActive, true);
	
	// Add controlitems to the HelpWindow
	// [root control]
	::CreateRootControl( mHelpWindow, &root );
	
	// [Scrolling Text]
	mHelpBox = ::GetNewControl(kHelpBox, mHelpWindow);
	::EmbedControl(mHelpBox, root );
	
	// Add EventHandler to the Window
	mHelpWinEvtHandler = ::NewEventHandlerUPP(HelpEventHandler);
   ::InstallWindowEventHandler(mHelpWindow, mHelpWinEvtHandler, 2, evtlist, this, NULL);
	
	// Show it
	::ShowWindow(mHelpWindow);
	::DrawControls(mHelpWindow);
}

// -----------------------------------------------------------------------------------
// HelpBoundsChanged - live resize of the help window
// -----------------------------------------------------------------------------------
void
TPTestCarbon::HelpBoundsChanged(void)
{
	// Fšr den som vill gšra ett liveresizable hjŠlpfšnster ;-)
}



// ************************************************************
// *****
// *****							NON MEMBERS
// *****
// ************************************************************


// -----------------------------------------------------------------------------------
// QuitAppleEventHandler - To handle OS8/9 shutdown Quit Apple Events
// -----------------------------------------------------------------------------------
static pascal OSErr QuitAppleEventHandler(const AppleEvent *appleEvt, AppleEvent* reply, long refcon)
{
#pragma unused (appleEvt, reply, refcon)
	::QuitApplicationEventLoop();
	return noErr;
}


// -----------------------------------------------------------------------------------
// AppCommandHandler - Handles Application Events
// -----------------------------------------------------------------------------------
pascal OSStatus AppCommandHandler(EventHandlerCallRef /*nextHandler*/, EventRef theEvent, void* userData)
{
	// Locals
	HICommand  		aCommand;
	OSStatus   		result = eventNotHandledErr;
	TPTestCarbon* 	tp = (TPTestCarbon*) userData;
		
	// What messge type
	switch ( ::GetEventClass(theEvent ) ) {
		
		case kEventClassCommand: 
			switch( ::GetEventKind(theEvent) ) {
				
				case kEventCommandProcess:
					// Get the Event Parameter
					::GetEventParameter(theEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
				    
				    // Check Command id
					switch (aCommand.commandID)
					{
						case 'Hlp!':
							tp->ShowHelpWindow();
							result = noErr;
							break;
						
						case 'Kon!':
							::NoteAlert(kKontaktAlert, NULL );
							::ResetAlertStage();
							result=noErr;
							break;
							
						case kHICommandAbout:
							tp->DoAboutBox();
							result = noErr; 
							break;
											                
						case kHICommandQuit:
							::QuitApplicationEventLoop();
							result = noErr;
							break;
				        
						default:
							break;
					}
					
					::HiliteMenu(0);
					break;
					
				case kEventCommandUpdateStatus:
					// Get the Event Parameter
					::GetEventParameter(theEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
				    
				    // Check Command id
					switch (aCommand.commandID)
					{
						case 'Hlp!':
							result = noErr; 
							break;
							
						case 'abou':
							if ( tp->HasAboutWindow() ) {
								::DisableMenuItem( aCommand.menu.menuRef, aCommand.menu.menuItemIndex );
							}
							else {
								::EnableMenuItem( aCommand.menu.menuRef, aCommand.menu.menuItemIndex );
							}
							result = noErr; 
							break;
											                
				        case kHICommandPreferences:
							::DisableMenuItem( aCommand.menu.menuRef, aCommand.menu.menuItemIndex );
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
	
	return result;
}

// -----------------------------------------------------------------------------------
// HelpEventHandler - Event handler for the Help Window
// -----------------------------------------------------------------------------------
static pascal OSStatus HelpEventHandler(EventHandlerCallRef /*myHandler*/, EventRef event, void* userData)
{
	// Locals
   OSStatus			result = eventNotHandledErr;
	TPTestCarbon* 	tp = (TPTestCarbon*) userData;
	
	switch( ::GetEventClass(event) ) {
	
		case kEventClassWindow:
		    switch(::GetEventKind(event)) {
		    
		    	case kEventWindowClose:
					tp->CleanHelp();
		      	result = noErr;
		    		break;
		 
		 		case kEventWindowBoundsChanged:
		 			tp->HelpBoundsChanged();
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



