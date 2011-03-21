#include "OSInfo.h"

OSInfo::OSInfo() {
	char tmpbuf[200];

	bIsWin95 = false;
	bIsWin98 = false;
	bIsWinME = false;
	bIsWinNT = false;
	bIsWin2k = false;
	bIsWinXP = false;
	bIsWin2003 = false;
	bIsWin32s = false;

	os_info_string.clear();

   // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
   // If that fails, try using the OSVERSIONINFO structure.

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

   if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
   {
      osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
      if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) 
         return;
   }

   switch (osvi.dwPlatformId)
   {
      // Test for the Windows NT product family.
      case VER_PLATFORM_WIN32_NT:

         // Test for the specific product family.
		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 ) {
			bIsWin2003 = true;
			os_info_string.append("Microsoft Windows Server 2003 family, ");
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 ) {
			bIsWinXP = true;
            os_info_string.append("Microsoft Windows XP ");
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 ) {
			bIsWin2k = true;
            os_info_string.append("Microsoft Windows 2000 ");
		}

		if ( osvi.dwMajorVersion <= 4 ) {
			bIsWinNT = true;
            os_info_string.append("Microsoft Windows NT ");
		}

         // Test for specific product on Windows NT 4.0 SP6 and later.
         if( bOsVersionInfoEx )
         {
            // Test for the workstation type.
            if ( osvi.wProductType == VER_NT_WORKSTATION )
            {
               if( osvi.dwMajorVersion == 4 )
                  os_info_string.append( "Workstation 4.0 " );
               else if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
                  os_info_string.append( "Home Edition " );
               else
                  os_info_string.append( "Professional " );
            }
            
            // Test for the server type.
            else if ( osvi.wProductType == VER_NT_SERVER )
            {
               if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
               {
                  if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                     os_info_string.append( "Datacenter Edition " );
                  else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                     os_info_string.append( "Enterprise Edition " );
                  else if ( osvi.wSuiteMask == VER_SUITE_BLADE )
                     os_info_string.append( "Web Edition " );
                  else
                     os_info_string.append( "Standard Edition " );
               }

               else if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
               {
                  if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                     os_info_string.append( "Datacenter Server " );
                  else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                     os_info_string.append( "Advanced Server " );
                  else
                     os_info_string.append( "Server " );
               }

               else  // Windows NT 4.0 
               {
                  if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                     os_info_string.append("Server 4.0, Enterprise Edition " );
                  else
                     os_info_string.append( "Server 4.0 " );
               }
            }
         }
         else  // Test for specific product on Windows NT 4.0 SP5 and earlier
         {
            HKEY hKey;
            WCHAR szProductType[BUFSIZE];
            DWORD dwBufLen=BUFSIZE;
            LONG lRet;

            lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
               L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
               0, KEY_QUERY_VALUE, &hKey );
            if( lRet != ERROR_SUCCESS )
               return;

            lRet = RegQueryValueEx( hKey, L"ProductType", NULL, NULL,
               (LPBYTE) szProductType, &dwBufLen);
            if( (lRet != ERROR_SUCCESS) || (dwBufLen > BUFSIZE) )
               return;

            RegCloseKey( hKey );

            if ( lstrcmpi( L"WINNT", szProductType) == 0 )
               os_info_string.append( "Workstation " );
            if ( lstrcmpi( L"LANMANNT", szProductType) == 0 )
               os_info_string.append( "Server " );
            if ( lstrcmpi( L"SERVERNT", szProductType) == 0 )
               os_info_string.append( "Advanced Server " );

			sprintf(tmpbuf, "%d.%d ", osvi.dwMajorVersion, osvi.dwMinorVersion);
			os_info_string.append(tmpbuf);
         }

      // Display service pack (if any) and build number.

         if( osvi.dwMajorVersion == 4 && 
             lstrcmpi( osvi.szCSDVersion, L"Service Pack 6" ) == 0 )
         {
            HKEY hKey;
            LONG lRet;

            // Test for SP6 versus SP6a.
            lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
               L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009",
               0, KEY_QUERY_VALUE, &hKey );
			if( lRet == ERROR_SUCCESS ) {
				sprintf(tmpbuf, "Service Pack 6a (Build %d)\n", osvi.dwBuildNumber & 0xFFFF );
				os_info_string.append(tmpbuf);
			}
            else // Windows NT 4.0 prior to SP6a
            {
               sprintf(tmpbuf, "%s (Build %d)\n",
                  osvi.szCSDVersion,
                  osvi.dwBuildNumber & 0xFFFF);
			   os_info_string.append(tmpbuf);
            }

            RegCloseKey( hKey );
         }
         else // Windows NT 3.51 and earlier or Windows 2000 and later
         {
            sprintf(tmpbuf, "%s (Build %d)\n",
               osvi.szCSDVersion,
               osvi.dwBuildNumber & 0xFFFF);
			os_info_string.append(tmpbuf);
         }


         break;

      // Test for the Windows 95 product family.
      case VER_PLATFORM_WIN32_WINDOWS:

         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
         {
			 bIsWin95 = true;
             os_info_string.append("Microsoft Windows 95 ");
             if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' )
                os_info_string.append("OSR2 " );
         } 

         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
         {
			 bIsWin98 = true;
             os_info_string.append("Microsoft Windows 98 ");
             if ( osvi.szCSDVersion[1] == 'A' )
                os_info_string.append("SE " );
         } 

         if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
         {
			 bIsWinME = true;
             os_info_string.append("Microsoft Windows Millennium Edition ");
         } 
         break;

      case VER_PLATFORM_WIN32s:
		 bIsWin32s = true;
         os_info_string.append("Microsoft Win32s ");
         break;
   }
   return;
}

const std::string & OSInfo::GetOSInfoString() {
	return os_info_string;
}

OSVERSIONINFOEX * OSInfo::GetOSInfo() {
	return &osvi;
}