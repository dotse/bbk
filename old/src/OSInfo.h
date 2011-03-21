#ifndef _OSINFO_H_
#define _OSINFO_H_

#include <windows.h>
#include <string>

#define BUFSIZE 80

class OSInfo {
public:
	OSInfo();
	OSVERSIONINFOEX * GetOSInfo();
	const std::string & GetOSInfoString();
	bool IsWinXP() { return bIsWinXP; }
	bool IsWin2k() { return bIsWin2k; }
	bool IsWin98() { return bIsWin98; }
	bool IsWin95() { return bIsWin95; }
	bool IsWinME() { return bIsWinME; }
	bool IsWinNT() { return bIsWinNT; }
	bool IsWin2003() { return bIsWin2003; }
	bool IsWin32s() { return bIsWin32s; }
private:
	OSVERSIONINFOEX osvi;
	BOOL bOsVersionInfoEx;
	std::string os_info_string;
	char buf[BUFSIZE];
	bool bIsWinXP;
	bool bIsWin2k;
	bool bIsWin98;
	bool bIsWin95;
	bool bIsWinME;
	bool bIsWinNT;
	bool bIsWin2003;
	bool bIsWin32s;
};

#endif