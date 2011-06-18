#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.

; Example #2: Calculate the size of a folder, including the files in all its subfolders:
SetBatchLines, -1  ; Make the operation run at maximum speed.
FolderSizeKB = 0
FileRemoveDir, thumbs, 1
FileCreateDir, thumbs

FileSelectFolder, WhichFolder, %A_ScriptDir%\data\level ; Ask the user to pick a folder.
Loop, %WhichFolder%\*.cfg, , 0  ; Recurse into subfolders.
{
    Run Frogatto.exe --level %A_LoopFileName%, , UseErrorLevel, FrogProcess
    Sleep, 3500	;increase this if the screenshot happens before the level is loaded
    SendEvent, {AltDown}{PrintScreen}{AltUp}
    Sleep, 100
    Process, Close, %FrogProcess%
    Sleep, 100
    
    ; FileAppend, %ClipboardAll%, %A_ScriptDir%\thumbs\%A_LoopFileName%.clip
    Run C:\WINDOWS\system32\mspaint.exe
    Sleep, 500
    Send {CtrlDown}vs{CtrlUp}
    Sleep, 500
    pngPath = %A_ScriptDir%\thumbs\%A_LoopFileName%.png
    Send %pngPath%{Enter}
    Sleep, 100,
    Send {AltDown}{F4}{AltUp}
    Sleep, 100
    IfWinActive, Save As,
    {
        Send, {Tab}{Enter}
        Sleep, 100
        Send, {AltDown}{F4}{AltUp}
        }
    IfWinNotActive, Frogatto.exe, , WinActivate, Frogatto.exe, 
    IfWinActive, Frogatto.exe,
    {
        Send, {Enter}
        Sleep, 100
        }
    
    MsgBox, 4, , Processed to %A_ScriptDir%\thumbs\%A_LoopFileName%.clip`n`nContinue?, 1
    IfMsgBox, No
        break
    }

