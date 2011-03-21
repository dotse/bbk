; -- Example1.iss --
; Demonstrates copying 3 files and creating an icon.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

[Setup]
AppName=TPTEST5
AppVerName=TPTEST 5.0
DefaultDirName={pf}\TPTEST5
DefaultGroupName=TPTEST5
UninstallDisplayIcon={app}\TPTEST5.exe

[Files]
Source: "TPTEST5.exe"; DestDir: "{app}"; DestName: "TPTEST5.exe"; MinVersion: 0, 1
Source: "bgd.dll"; DestDir: "{app}"; DestName: "bgd.dll"; MinVersion: 0, 1
Source: "libcurl.dll"; DestDir: "{app}"; DestName: "libcurl.dll"; MinVersion: 0, 1
Source: "zlibwapi.dll"; DestDir: "{app}"; DestName: "zlibwapi.dll"; MinVersion: 0, 1
Source: "msvcp71.dll"; DestDir: "{app}"; DestName: "msvcp71.dll"; MinVersion: 0, 1
Source: "msvcr71.dll"; DestDir: "{app}"; DestName: "msvcr71.dll"; MinVersion: 0, 1
Source: "GHNlogo.jpg"; DestDir: "{app}"; DestName: "GHNlogo.jpg"; MinVersion: 0, 1
Source: "start.jpg"; DestDir: "{app}"; DestName: "start.jpg"; MinVersion: 0, 1
Source: "help.html"; DestDir: "{app}"; DestName: "help.html"; MinVersion: 0, 1
Source: "tptest-result-list.jpg"; DestDir: "{app}"; DestName: "tptest-result-list.jpg"; MinVersion: 0, 1
Source: "trend-colorpoint.jpg"; DestDir: "{app}"; DestName: "trend-colorpoint.jpg"; MinVersion: 0, 1
Source: "tptest.css"; DestDir: "{app}"; DestName: "tptest.css"; MinVersion: 0, 1
Source: "skapa-felrapport.jpg"; DestDir: "{app}"; DestName: "skapa-felrapport.jpg"; MinVersion: 0, 1


;Source: "BS105_9X.exe"; DestDir: "{app}"; DestName: "Bluffstopparen.exe"; MinVersion: 1, 0

[Icons]
Name: "{group}\TPTEST5"; Filename: "{app}\TPTEST5.exe"; WorkingDir: "{app}"; MinVersion: 0, 1

[Registry]
;Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "Bluffstopparen"; ValueData: "{app}\Bluffstopparen.exe"; MinVersion: 1, 0
;Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\TPTEST5"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey

[Languages]
Name: "se"; MessagesFile: "Swedish_8401.isl"

[InstallDelete]
Type: files; Name: "{app}\TPTEST5.exe"

[UninstallDelete]
Type: files; Name: "{app}\TPTEST5.exe"


