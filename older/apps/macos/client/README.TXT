How to build TPTest for MAC using CodeWarrior:

1.	Unpack the TPTest distribution file tptest3.0.tar. You then get
	a directory structure like this:
	
	tptest/apps/macos/client
			
		macgui_resourcedefs.h
		macgui_main.cp
		macgui_main.h
		macgui_about.cp
		macgui_about.h
		macgui_statwin.c
		macgui_statwin.h
		macgui_testwindow.cp
		macgui_testwindow.h

		TPTest.mcp.hqx
		TPTest.rsrc.hqx

		README.TXT
		textify.hqx
		textify.txt
		
	tptest/engine

		tpclient.c
		tpclient.h
		tpcommon.c
		tpcommon.h
		tpengine.c
		tpengine.h
		tpio.h
		
	tptest/os-dep/macos

		tpio_mac.c
		tpio_mac.h

	and some others for the unix and win32 platforms.
	
2.	Go to 'tptest/apps/macos/client' where you will find

	TPTest.mcp.hqx
	TPTest.rsrc.hqx
	textify.hqx
	
	Unstuff these files to get the real ones
	
	TPTest.mcp
	TPTest.rsrc
	textify
	
3.	Run the application 'textify'. It is a utility that makes sure that you
	get the correct file type/creator on the necessary source files. You
	need to do this or older CW:s won't open them.
	
4.	Open 'TPTest.mcp' with CodeWarrior.

5.	Open 'tptest/engine/tpengine.h' and uncomment the platform selection
	for MAC so it looks like this. Then save it.
	
	// Select platform
	// #define __WIN32
	#define MACOS

	You will notice if you forget this step - tons of error.
	
6.	In CW: Remove object code - just to get a clean start.

7.	'Make' the target 'TPTest_3'. The result should be the 'TPTest 3.0.X'
	application.

Note: You need a precompiled version of the Carbon headers in the file
'CarbonHeadersPCH'. This is specified as prefix file in the C++ settings
pane in the target settings.

What could possibly go wrong?

