/*
 *	
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * macgui_about.cp - main GUI routines for Mac platform
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "macgui_about.h"
#include "macgui_main.h"

// Callbacks that have to be pure C
extern "C" {
	static pascal OSStatus AboutBoxEventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData);
}

// --------------------------------------------------------
// AboutWindow [constructor]
// --------------------------------------------------------
AboutWindow::AboutWindow(TPTestCarbon* inowner)
{
	// get incoming
	mOwner = inowner;
	
	// Initialize class members
	mAboutWinEvtHandler = NULL;
 	mAboutWindow = NULL;
 	
 	// Do the stuff
 	CreateWindow();
}

// --------------------------------------------------------
// ~AboutWindow [destructor]
// --------------------------------------------------------
AboutWindow::~AboutWindow()
{
	// Delete any stuff we have
	if ( mAboutWinEvtHandler ) {
		DisposeEventHandlerUPP(mAboutWinEvtHandler);
		mAboutWinEvtHandler=NULL;
	}
	
	if ( mAboutWindow ) {
  		ReleaseWindow(mAboutWindow);
 		mAboutWindow=NULL;
	}
	
	// Tell owner
	if ( mOwner != NULL ) {
		mOwner->AboutClosed();
	}
}

// -----------------------------------------------------------------------------------
// CreateWindow - Show the cool aboutbox
// -----------------------------------------------------------------------------------
void 
AboutWindow::CreateWindow(void)
{
	// Locals
	ControlRef				dummyStatic;
	ControlFontStyleRec		cfontrec;
	Boolean					indefinite = true;
	
	// Needed even specs
	EventTypeSpec	evtlist[] = {{kEventClassWindow, kEventWindowClose},
									{kEventClassWindow, kEventWindowDrawContent},
									{kEventClassCommand, kEventCommandProcess}
								   };

	// Create the AboutWindow from the resource
	mAboutWindow = ::GetNewCWindow(kAboutBox, NULL, (WindowRef) -1);
	if ( !mAboutWindow ) {
		::SysBeep(1);
		return;
	}
	
	// WindowTitle
	::SetWTitle( mAboutWindow, kAboutTitle );

	// Set Window Attributes for Carbon Events Handling and closebox
	(void) ::ChangeWindowAttributes(mAboutWindow, kWindowStandardHandlerAttribute ,kWindowCollapseBoxAttribute | kWindowFullZoomAttribute );

	// Set The correct Window background theme
	(void) ::SetThemeWindowBackground(mAboutWindow, kThemeBrushDialogBackgroundActive, true);
	
	// Add controlitems to the AboutWindow
	// [root control]
	::CreateRootControl( mAboutWindow, &mRootControl );
	
	// [OK Button]
	mAboutOKButtonRef = ::GetNewControl(kAboutOKButton, mAboutWindow);
	::EmbedControl(mAboutOKButtonRef, mRootControl );
	::SetControlCommandID(mAboutOKButtonRef, kCommandClose);
	::SetWindowDefaultButton( mAboutWindow, mAboutOKButtonRef );
	
	// [Big line 1]
	dummyStatic = ::GetNewControl(kAboutLine1, mAboutWindow);
	cfontrec.font = kControlFontBigSystemFont;
	cfontrec.flags = kControlUseFontMask+kControlUseFaceMask+kControlUseSizeMask+kControlUseJustMask;
	cfontrec.just = teJustCenter;
	(void)::SetControlData( dummyStatic, 0, kControlStaticTextTextTag, strlen(kAboutText1), kAboutText1);
	(void)::SetControlData( dummyStatic, 0, kControlStaticTextStyleTag, sizeof(cfontrec), &cfontrec );	
	::EmbedControl(dummyStatic, mRootControl );
	   
	// [Small line 2]
	dummyStatic = ::GetNewControl(kAboutLine2, mAboutWindow);
	cfontrec.font = kControlFontSmallSystemFont;
	cfontrec.flags = kControlUseFontMask+kControlUseFaceMask+kControlUseSizeMask+kControlUseJustMask;
	cfontrec.just = teJustCenter;
	(void)::SetControlData( dummyStatic, 0, kControlStaticTextTextTag, strlen(kAboutText2), kAboutText2);
	(void)::SetControlData( dummyStatic, 0, kControlStaticTextStyleTag, sizeof(cfontrec), &cfontrec );	
	::EmbedControl(dummyStatic, mRootControl );
	
	// [Scrolling Text]
	dummyStatic = ::GetNewControl(kAboutLegal, mAboutWindow);
	::EmbedControl(dummyStatic, mRootControl );
	
	// Add EventHandler to the Window
	mAboutWinEvtHandler = ::NewEventHandlerUPP(AboutBoxEventHandler);
   ::InstallWindowEventHandler(mAboutWindow, mAboutWinEvtHandler, 3, evtlist, this, NULL);
	
	// Show it
	::ShowWindow(mAboutWindow);
	::DrawControls(mAboutWindow);
}


// ***********************************************************************************
//
//								NON Class methods and callbacks
//
// ***********************************************************************************

// -----------------------------------------------------------------------------------
// AboutBoxEventHandler - Event handler for the Aboutbox Window
// -----------------------------------------------------------------------------------
static pascal OSStatus AboutBoxEventHandler(EventHandlerCallRef /*myHandler*/, EventRef event, void* userData)
{
	// Locals
	OSStatus		result = eventNotHandledErr;
	HICommand  		aCommand;
	AboutWindow*	aw = (AboutWindow*) userData;
	
	switch( ::GetEventClass(event) ) {
		case kEventClassCommand:
			::GetEventParameter(event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
			switch (aCommand.commandID) {
				case kCommandClose:
						delete aw;
			        	result = noErr;
			        	break;
			        
			      default:
			      	break;
		    }
			break;
			
		case kEventClassWindow:
		    if (GetEventKind(event) == kEventWindowClose) {
					delete aw;
		      	result = noErr;
		    }
		    break;
		 
		 default:
		 	break;
	}
	
	// Done
    return result;
}


