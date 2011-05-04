/*
 *	
 * TPTEST 3.0 (C) Copyright II-Stiftelsen 2002
 *
 * macgui_resourcedefs.h - Mac resource definitions
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

#ifndef __MACGUI_RESOURCEDEFS_H__
#define __MACGUI_RESOURCEDEFS_H__

// Constants for resources etc.
const	short		kOSXMenuBar					= 129;
const	short		kOS9MenuBar					= 128;

const	short		kSheetCancelButton		= 501;
const	short		kSheetProgressBar			= 502;
const	short		kSheetInfoText				= 503;

const short		kAlertSheetOKButton		= 601;
const short		kAlertSheetOKText			= 602;

const	short		kAboutBox					= 1000;
const	short		kAboutOKButton				= 1001;
const	short		kAboutLine1					= 1002;
const	short		kAboutLine2					= 1003;
const	short		kAboutLegal					= 1006;

const	short		kBarPic						= 2001;

const	short		kHelpWindow					= 3000;

const	short		kTestWindow					= 2000;
const	short		kTestStartButton			= 2001;
const	short		kTestServerPopup			= 2002;
const	short		kTestMethodPopup			= 2003;
const	short		kTestWindowProgressBar	= 2004;
const	short		kTestWindowFlipper		= 2005;
const	short		kTestWindowFlipperInfo	= 2006;
const	short		kTestWindowServerEF		= 2007;
const	short		kTestWindowServerST		= 2008;
const	short		kTestWindowPortEF			= 2009;
const	short		kTestWindowDataEF			= 2010;
const	short		kTestWindowDataST			= 2011;
const	short		kTestWindowPackEF			= 2012;
const	short		kTestWindowPackST			= 2013;
const	short		kTestWindowRateEF			= 2014;
const	short		kTestWindowRateST			= 2015;
const	short		kTestWindowTimeEF			= 2016;
const	short		kTestWindowTimeST			= 2017;
const	short		kTestWindowTCPEF			= 2018;
const	short		kTestWindowTCPST			= 2019;
const	short		kTestWindowAdvGroup		= 2020;
const	short		kTestWindowTypePop		= 2021;
const	short		kTestWindowPortST			= 2023;
const	short		kTestWindowTCPGrp			= 2024;
const	short		kTestWindowUDPGrp			= 2025;
const short		kHelpBox						= 3001;

// Commands
const	UInt32		kCommandClose			= 'clos';
const	UInt32		kCommandRun				= 'run ';
const	UInt32		kCommandOK				= 'ok  ';
const	UInt32		kCommandFlip			= 'flip';
const	UInt32		kTypePopup				= 'TPop';
const UInt32		kServerFlip				= 'sFlp';
const	UInt32		kFlipResize				= 172;

// Some constants for carbonLib versions and alerts
#define kBadCarbonLibVersion				4711
#define kMinimumCarbonLib					0x131
#define kBadCarbonLibVersionAlert		4712
#define kKontaktAlert						4711

// Texts
#define kClientType				"TPTest 3.0.3 Mac"
#define kAboutTitle				"\pTPTest 3.0.3"
#define kAboutText1				"För MacOS 8.6, MacOS 9.x & MacOS X"
#define kAboutText2				"MacOS 8.6 eller 9.x kräver CarbonLib version 1.3.1 eller senare."
#define kSplashInfotext			"Hämtar serverlista från masterserver."
#define kMasterServerError		"Ett fel uppstod vid kontakten med masterservern."
#define kTestTimeText			"Testtid (sekunder):"
#define kTestTCPText				"TCP Bytes att skicka:"
#define kPackrateText			"Paket per sekund:"
#define kPacksizeText			"Paketstorlek (bytes):"
#define kDatarateText			"Datahastighet (bitar/s):"
#define kServerText				"Server:"
#define kPortText					"Port:"
#define kAdvancedModeText 		"Visa avancerat läge"
#define kNormalModeText			"Göm avancerat läge"
#define kDefaultMaster			"testledare.ip-performance.se"
#define kpTestIsOn				"\pTPTest pågår..."
#define kpErrorText				"\pEtt inmatningsfel."
#define kpCancelText				"\pAvbryt"
#define kpStartText				"\pStarta"
#define kpSplashTitle			"\pKontaktar masterservern"

#endif
