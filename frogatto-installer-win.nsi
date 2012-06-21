;Frogatto Windows Installer Script for NSIS
;Written by Kristina

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"

;-------------------------------
; Test if Visual Studio Redistributables 2005+ SP1 installed
; Returns -1 if there is no VC redistributables intstalled
Function CheckVCRedist
   Push $R0
   ClearErrors
   #ReadRegDword $R0 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{196BB40D-1578-3D01-B289-BEFC77A11A1E}" "Version"
   ReadRegDword $R0 HKLM "SOFTWARE\Microsoft\VisualStudio\10.0\VC\VCRedist\x86" "Installed"

   ; if VS 2010 redist not installed, install it
   IfErrors 0 VSRedistInstalled
   StrCpy $R0 "-1"
   File "c:\projects\Frogatto\vcredist_x86.exe"
   ExecWait '"$INSTDIR\vcredist_x86.exe" /q' $0

VSRedistInstalled:
   Exch $R0
FunctionEnd
   
;--------------------------------
;General

  ;Name and file
  Name "Frogatto & Friends" "Frogatto && Friends"
  OutFile "Frogatto-installer.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\Frogatto"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\Frogatto" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel user

;--------------------------------
;Variables

  Var StartMenuFolder

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "c:\projects\Frogatto\LICENSE"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY

  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Frogatto" 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  
  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "Frogatto & Friends" SecInstaller

  SetOutPath "$INSTDIR"
  
  File /a /r "C:\Projects\frogatto-build\src\Release\*.*"
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Frogatto" "" $INSTDIR

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Frogatto" \
    "DisplayName" "Frogatto & Friends"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Frogatto" \
    "UninstallString" "$\"$INSTDIR\uninstall.exe$\""  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  ;Create shortcuts
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Frogatto.lnk" "$INSTDIR\Frogatto.exe"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END

  Call CheckVCRedist
SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  ;LangString DESC_SecDummy ${LANG_ENGLISH} "A test section."

  ;Assign language strings to sections
  ;!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  ;  !insertmacro MUI_DESCRIPTION_TEXT ${SecDummy} $(DESC_SecDummy)
  ;!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...

  Delete "$INSTDIR\Uninstall.exe"

  ; Check that Frogatto is not installed in root of Program Files; if it is, just delete known folders and files
  StrCmp $INSTDIR $PROGRAMFILES 0 +13
    RMDir /r "$INSTDIR\data"
    RMDir /r "$INSTDIR\images"
    RMDir /r "$INSTDIR\locale"
    RMDir /r "$INSTDIR\modules"
    RMDir /r "$INSTDIR\music"
    RMDir /r "$INSTDIR\music_aac"
    RMDir /r "$INSTDIR\music_aac_mini"
    RMDir /r "$INSTDIR\po"
    RMDir /r "$INSTDIR\sounds"
    RMDir /r "$INSTDIR\sounds_wav"
    RMDir /r "$INSTDIR\utils"
    Goto +2
    RMDir /r "$INSTDIR"
    
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  
  Delete "$SMPROGRAMS\$StartMenuFolder\Frogatto.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"
 
  DeleteRegKey /ifempty HKCU "Software\Frogatto"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Frogatto"

SectionEnd