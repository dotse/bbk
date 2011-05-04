/*
 *	
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * macgui_about.h - Header for Mac 'about' window
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

#ifndef __ABOUTWINDOW__H__
#define __ABOUTWINDOW__H__

// Includes
#include <Carbon.h>
#include "macgui_resourcedefs.h"

// used classes
class TPTestCarbon;

// About window class def
class AboutWindow {
public:
		// Con and Destructor
										AboutWindow(TPTestCarbon* inowner);
		virtual						~AboutWindow();
		
private:
		// members
		ControlRef					mRootControl;					// The Windows root control
		ControlRef					mAboutOKButtonRef;			// The OK Button
		WindowRef					mAboutWindow;					// The ref to the About Window
		EventHandlerUPP			mAboutWinEvtHandler;			// The About window event handler
		TPTestCarbon*				mOwner;							// The application that owns me
		
		// methods
		void							CreateWindow(void);			// Create the window
};

#endif
