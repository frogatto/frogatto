; Windows Installer file for Frogatto&Friends

[Setup]
AppName=Frogatto & Friends
AppVersion=1.4.alpha
DefaultDirName={pf}\Frogatto
DefaultGroupName=Frogatto & Friends
UninstallDisplayIcon={app}\frogatto.exe
Compression=lzma2
SolidCompression=yes
OutputDir=c:\projects\frogatto\

[Files]
Source: "C:\Projects\frogatto\vs2012\Frogatto\Release\Win32\*.*"; DestDir: "{app}"; Excludes: "*.pdb,.*,std*.*";
Source: "C:\Projects\frogatto\vs2012\Frogatto\Release\Win32\data\*.*"; DestDir: "{app}\data"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto\vs2012\Frogatto\Release\Win32\images\*.*"; DestDir: "{app}\images"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto\vcredist_x86.exe"; DestDir: "{app}"; Flags: deleteafterinstall

; This is everything which is not nescessarily whats wanted
; Source: "C:\Projects\frogatto\vs2012\Frogatto\Release\Win32\modules\frogatto\*.*"; DestDir: "{app}\modules\frogatto"; Excludes: ".*"; Flags: recursesubdirs

; These are more selective.
Source: "C:\Projects\frogatto\vs2012\Frogatto\Release\Win32\modules\frogatto\*.cfg"; DestDir: "{app}\modules\frogatto"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto\vs2012\Frogatto\Release\Win32\modules\frogatto\data\*.*"; DestDir: "{app}\modules\frogatto\data"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto\vs2012\Frogatto\Release\Win32\modules\frogatto\images\*.*"; DestDir: "{app}\modules\frogatto\images"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto\vs2012\Frogatto\Release\Win32\modules\frogatto\locale\*.*"; DestDir: "{app}\modules\frogatto\locale"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto\vs2012\Frogatto\Release\Win32\modules\frogatto\sounds\*.*"; DestDir: "{app}\modules\frogatto\sounds"; Excludes: ".*"; Flags: recursesubdirs
;Source: "C:\Projects\frogatto\vs2012\Frogatto\Release\Win32\modules\frogatto\music\*.*"; DestDir: "{app}\modules\frogatto\music"; Excludes: ".*"; Flags: recursesubdirs


[Icons]
Name: "{group}\Frogatto & Friends"; Filename: "{app}\frogatto.exe"
Name: "{group}\Uninstall Frogatto & Friends"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\vcredist_x86.exe"; Parameters: "/q"

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "gla"; MessagesFile: "compiler:Languages\ScotsGaelic.isl"
Name: "ptBR"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "cz"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "nl"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "jp"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "pl"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "cat"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "dan"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "fin"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "cat"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "heb"; MessagesFile: "compiler:Languages\Hungarian.isl"
Name: "nor"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "por"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "srp"; MessagesFile: "compiler:Languages\SerbianCyrillic.isl"
Name: "srplatin"; MessagesFile: "compiler:Languages\SerbianLatin.isl"
Name: "slv"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "spa"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "ukr"; MessagesFile: "compiler:Languages\Ukrainian.isl"
