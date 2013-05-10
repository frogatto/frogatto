; Windows Installer file for Frogatto&Friends

[Setup]
AppName=Frogatto & Friends
AppVersion=1.3.2
DefaultDirName={pf}\Frogatto
DefaultGroupName=Frogatto & Friends
UninstallDisplayIcon={app}\frogatto.exe
Compression=lzma2
SolidCompression=yes
OutputDir=c:\projects\frogatto\

[Files]
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\*.*"; DestDir: "{app}"; Excludes: "*.pdb,.*,*.lib,*.exp,std*.*";
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\data\*.*"; DestDir: "{app}\data"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\images\*.*"; DestDir: "{app}\images"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto\vcredist_x86.exe"; DestDir: "{app}"; Flags: deleteafterinstall

; This is everything which is not nescessarily whats wanted
; Source: "C:\Projects\frogatto-build\Frogatto\Win32\Release\modules\frogatto\*.*"; DestDir: "{app}\modules\frogatto"; Excludes: ".*"; Flags: recursesubdirs

; These are more selective.
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\*.cfg"; DestDir: "{app}\modules\frogatto"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\data\*.*"; DestDir: "{app}\modules\frogatto\data"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\images\*.*"; DestDir: "{app}\modules\frogatto\images"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\locale\*.*"; DestDir: "{app}\modules\frogatto\locale"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\sounds\*.*"; DestDir: "{app}\modules\frogatto\sounds"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\music\*.*"; DestDir: "{app}\modules\frogatto\music"; Excludes: ".*"; Flags: recursesubdirs


[Icons]
Name: "{group}\Frogatto & Friends"; Filename: "{app}\frogatto.exe"
Name: "{group}\Uninstall Frogatto & Friends"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\vcredist_x86.exe"; Parameters: "/q"
