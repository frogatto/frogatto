; Windows Installer file for Frogatto&Friends

[Setup]
AppName=Frogatto & Friends
AppVersion=1.3.alpha
DefaultDirName={pf}\Frogatto
DefaultGroupName=Frogatto & Friends
UninstallDisplayIcon={app}\frogatto.exe
Compression=lzma2
SolidCompression=yes
OutputDir=c:\projects\frogatto\

[Files]
Source: "C:\Projects\frogatto-build\Frogatto\Win32\Release\*.*"; DestDir: "{app}"; Excludes: "*.pdb,.*,std*.*";
Source: "C:\Projects\frogatto-build\Frogatto\Win32\Release\modules\frogatto\*.*"; DestDir: "{app}\modules\frogatto"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Win32\Release\data\*.*"; DestDir: "{app}\data"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Win32\Release\images\*.*"; DestDir: "{app}\images"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto\vcredist_x86.exe"; DestDir: "{app}"; Flags: deleteafterinstall

[Icons]
Name: "{group}\Frogatto & Friends"; Filename: "{app}\frogatto.exe"
Name: "{group}\Uninstall Frogatto & Friends"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\vcredist_x86.exe"; Parameters: "/q"
