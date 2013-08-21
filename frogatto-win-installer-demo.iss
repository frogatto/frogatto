; Windows Installer file for Frogatto&Friends

[Setup]
AppName=Frogatto & Friends Demo
AppVersion=1.3.3
DefaultDirName={pf}\Frogatto Demo
DefaultGroupName=Frogatto & Friends Demo
UninstallDisplayIcon={app}\frogatto.exe
Compression=lzma2
SolidCompression=yes
OutputDir=c:\projects\frogatto\

[Files]
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\*.*"; DestDir: "{app}"; Excludes: "*.pdb,.*,*.lib,*.exp,std*.*"; AfterInstall: CreateMasterConfig(ExpandConstant('{app}'))
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\data\*.*"; DestDir: "{app}\data"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\images\*.*"; DestDir: "{app}\images"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto\vcredist_x86.exe"; DestDir: "{app}"; Flags: deleteafterinstall; AfterInstall: ProcessLocale(ExpandConstant('{userappdata}'))

; This is everything which is not nescessarily whats wanted
; Source: "C:\Projects\frogatto-build\Frogatto\Win32\Release\modules\frogatto\*.*"; DestDir: "{app}\modules\frogatto"; Excludes: ".*"; Flags: recursesubdirs

; These are more selective.
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\*.cfg"; DestDir: "{app}\modules\frogatto"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\data\*.*"; DestDir: "{app}\modules\frogatto\data"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\images\*.*"; DestDir: "{app}\modules\frogatto\images"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\locale\*.*"; DestDir: "{app}\modules\frogatto\locale"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\sounds\*.*"; DestDir: "{app}\modules\frogatto\sounds"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\music\arcade\*.*"; DestDir: "{app}\modules\frogatto\music\arcade"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\music\general\*.*"; DestDir: "{app}\modules\frogatto\music\general"; Excludes: ".*"; Flags: recursesubdirs
Source: "C:\Projects\frogatto-build\Frogatto\Release\Win32\modules\frogatto\music\seaside\*.*"; DestDir: "{app}\modules\frogatto\music\seaside"; Excludes: ".*"; Flags: recursesubdirs


[Icons]
Name: "{group}\Frogatto & Friends Demo"; Filename: "{app}\frogatto.exe"
Name: "{group}\Uninstall Frogatto & Friends Demo"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\vcredist_x86.exe"; Parameters: "/q"

; en, de, es, fr, gd, it, pt_BR, zh_CN
[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"
Name: "gd"; MessagesFile: "compiler:Languages\ScotsGaelic.isl"
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "pt_BR"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "zh_CN"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
; Language code is in ExpandConstant('{language}')

[Code]
var
  OptionsPage: TInputOptionWizardPage;
  
procedure InitializeWizard;
begin
  { Create the pages }
  
  OptionsPage := CreateInputOptionPage(wpWelcome, 'Installation Options', 'Select from the following options', '', False, False);
  OptionsPage.Add('Default fullscreen mode');
  OptionsPage.Add('Send statistics to frogatto.com server');

  { Set default values, using settings that were stored last time if possible }
  if GetPreviousData('FullscreenMode', 'True') = 'True' then
	OptionsPage.Values[0] := True
  else
	OptionsPage.Values[0] := False;

  if GetPreviousData('SendStats', 'False') = 'True' then
	OptionsPage.Values[1] := True
  else
	OptionsPage.Values[1] := False;
  
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
  { Store the settings so we can restore them next time }
  if OptionsPage.Values[0] then
	SetPreviousData(PreviousDataKey, 'FullscreenMode', 'True')
  else
	SetPreviousData(PreviousDataKey, 'FullscreenMode', 'False');

  if OptionsPage.Values[1] then
	SetPreviousData(PreviousDataKey, 'SendStats', 'True')
  else
	SetPreviousData(PreviousDataKey, 'SendStats', 'False');
end;

procedure CreateMasterConfig(Appdir: String);
begin
  Log('CreateMasterConfig(''' + Appdir + ''')');
  SaveStringToFile(Appdir + '\master-config.cfg', '{' #10 'id: "frogatto",' #10 'name: "Frogatto & Friends",' #10 'arguments: ["--module=frogatto"', False);
  if OptionsPage.Values[0] then
	SaveStringToFile(Appdir + '\master-config.cfg', ',"--fullscreen"', True);
  if OptionsPage.Values[1] then
	SaveStringToFile(Appdir + '\master-config.cfg', ',"--send-stats"', True)
  else
	SaveStringToFile(Appdir + '\master-config.cfg', ',"--no-send-stats"', True);
  SaveStringToFile(Appdir + '\master-config.cfg', ']' #10 '}', True);
end;

// Split String separated by delimiter.
// Multi-Character delimiters should work but are probably a bad idea :-)
// Trailing separator should be ignored
// Leading separator should give a leading empty field
// '' -> [] ... empty string yields empty list
// 'str' -> ['str']
// 'str1,str2' -> ['str1', 'str2']
// ',' -> ['']
// ',,' -> ['', '']
// 'a,' -> ['a']
// ',b' -> ['', 'b']
//
function Split(sExpression, sDelim: String): TArrayOfString;
var
  i, len:Integer;
  part: String;
begin
  len := 0;
  i := 1;
  while (Length(sExpression) > 0) and (i > 0) do begin
    i:=Pos(sDelim, sExpression);
    if i>0 then begin
      part := Copy(sExpression, 0, i-1);
      sExpression := Copy(sExpression, i+Length(sDelim),
      Length(sExpression));
      len := len + 1;
      SetArrayLength(Result, len);
      Result[len-1] := part;
    end;
  end;
  if Length(sExpression) > 0 then begin
    len := len + 1;
    SetArrayLength(Result, len);
    Result[len-1] := sExpression;
  end;
end;

procedure ProcessLocale(AppDataDir: String);
var 
  FileName : String;
  Index : Integer;
  data : AnsiString;
  FileLines: TArrayOfString;
  output_file_data : String;
begin
  Log('AppDataDir: ' + AppDataDir);
  FileName := AppDataDir + '\frogatto\preferences.cfg';
  if LoadStringFromFile(FileName, data) then
  begin
    FileLines := Split(data, #10);
    for Index := 0 to GetArrayLength(FileLines) - 1 do
    begin
      if Pos('locale', FileLines[Index]) <> 0 then
        output_file_data := output_file_data + '    "locale": "' + ExpandConstant('{language}') + '",' #10
      else
        output_file_data := output_file_data + FileLines[Index] + #10;
    end;
  end
  else
    output_file_data := '{' #10 '"locale": "' + ExpandConstant('{language}') + '",' #10 '}';
  SaveStringToFile(FileName, output_file_data, False);
end;
