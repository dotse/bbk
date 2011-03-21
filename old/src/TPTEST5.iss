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
Source: "tptest.conf"; DestDir: "{app}"; DestName: "tptest.conf"; MinVersion: 0, 1
Source: "zlibwapi.dll"; DestDir: "{app}"; DestName: "zlibwapi.dll"; MinVersion: 0, 1

;Source: "BS105_9X.exe"; DestDir: "{app}"; DestName: "Bluffstopparen.exe"; MinVersion: 1, 0

[Icons]
Name: "{commonstartup}\TPTEST5"; Filename: "{app}\TPTEST5.exe"; MinVersion: 0, 1

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


