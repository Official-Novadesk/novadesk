;--------------------------------
; Novadesk Installer Script
;--------------------------------

; Define global version variable (Update this as needed)
!define VERSION "0.5.0.0"

; The name of the installer
Name "Novadesk"

; The file to write
OutFile "dist_output\Novadesk_Setup_v${VERSION}_Beta.exe"
SetCompressor /SOLID lzma

; The default installation directory
InstallDir "$PROGRAMFILES64\Novadesk"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Novadesk" "Install_Dir"

; Request application privileges for Windows Vista+
RequestExecutionLevel admin

; Use 64-bit registry view
!include "x64.nsh"
!include "LogicLib.nsh"
!include "nsDialogs.nsh"
!include "Sections.nsh"
!include "StrFunc.nsh"
${StrStr}

;--------------------------------
; Interface Settings
;--------------------------------
!include "MUI2.nsh"

;--------------------------------
; Windows Message Constants
;--------------------------------
!ifndef WM_WININICHANGE
!define WM_WININICHANGE 0x001A
!endif
!ifndef HWND_BROADCAST
!define HWND_BROADCAST 0xFFFF
!endif

;--------------------------------
; Icon & Images
;--------------------------------
!define MUI_ICON "assets\installer.ico"
!define MUI_UNICON "assets\uninstaller.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "assets\header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "assets\banner.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "assets\banner.bmp"

;--------------------------------
; Pages
;--------------------------------
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
Page custom InstallModePageCreate InstallModePageLeave
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE DirectoryPageLeave
!insertmacro MUI_PAGE_DIRECTORY
!define MUI_PAGE_CUSTOMFUNCTION_PRE ComponentsPageShowPre
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES

; Finish page settings
!define MUI_FINISHPAGE_RUN "$INSTDIR\Novadesk.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Run Novadesk"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Data
;--------------------------------
Var InstallMode
Var RadioStandard
Var RadioPortable

;--------------------------------
; Sections
;--------------------------------

Section -CoreFiles SecCoreFiles
  SectionIn RO

  SetRegView 64
  ; Enforce 64-bit redirection
  ${If} ${RunningX64}
    ${DisableX64FSRedirection}
  ${EndIf}

  SetOutPath "$INSTDIR"
  
  ; Kill process if running
  nsExec::ExecToStack 'taskkill /F /IM "Novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "nwm.exe"'

  ; Add Novadesk files (x64 Release)
  SetOutPath "$INSTDIR"
  File "..\x64\Release\Novadesk.exe"
  ; Add installer stub (x64 Release)
  SetOutPath "$INSTDIR\nwm"
  File "..\x64\Release\nwm\installer_stub.exe"
  ; Copy Widgets folder
  SetOutPath "$INSTDIR"
  File /r "..\x64\Release\Widgets"
  
  ; Add nwm files (x64 Release)
  SetOutPath "$INSTDIR\nwm"
  File "..\x64\Release\nwm\nwm.exe"
  ; Copy nwm templates if they exist in output
  File /r "..\x64\Release\nwm\widget"
  SetOutPath "$INSTDIR"

  ${If} $InstallMode == "standard"
    ; Store installation folder
    WriteRegStr HKLM "Software\Novadesk" "Install_Dir" "$INSTDIR"

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; Add to Add/Remove Programs
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "DisplayName" "Novadesk v${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "DisplayIcon" "$INSTDIR\Novadesk.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "Publisher" "OfficialNovadesk"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "URLInfoAbout" "https://novadesk.pages.dev"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "HelpLink" "https://novadesk.pages.dev"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "DisplayVersion" "${VERSION}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk" \
                     "NoRepair" 1
  ${Else}
    DetailPrint "Portable mode selected: skipping uninstaller and registry writes."
  ${EndIf}

SectionEnd

Section "Add to PATH" SecPATH

  ${If} $InstallMode == "portable"
    DetailPrint "Portable mode selected: skipping PATH registry update."
    Return
  ${EndIf}

  ; Add the installation directory to the PATH using EnVar plugin
  ; Adding both root (for Novadesk?) and nwm folder (for nwm)
  ; User specifically asked for nwm to be in path.
  EnVar::SetHKLM
  EnVar::AddValue "PATH" "$INSTDIR\nwm"
  Pop $0
  DetailPrint "Add to PATH returned=|$0|"
  
SectionEnd

Function InstallModePageCreate
  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}

  ${NSD_CreateLabel} 0 0 100% 24u "Choose installation type:"
  Pop $0
  ${NSD_CreateRadioButton} 0 28u 100% 12u "Standard (recommended): install with uninstaller and registry entries"
  Pop $RadioStandard
  ${NSD_CreateRadioButton} 0 44u 100% 12u "Portable: no uninstaller, no registry entries"
  Pop $RadioPortable

  ${NSD_Check} $RadioStandard
  StrCpy $InstallMode "standard"
  nsDialogs::Show
FunctionEnd

Function .onInit
  StrCpy $InstallMode "standard"
  !insertmacro SelectSection ${SecCoreFiles}
FunctionEnd

Function InstallModePageLeave
  ${NSD_GetState} $RadioPortable $0
  ${If} $0 == ${BST_CHECKED}
    StrCpy $InstallMode "portable"
    StrCpy $INSTDIR "$EXEDIR"
    !insertmacro SelectSection ${SecCoreFiles}
    !insertmacro SelectSection ${SecPATH}
    !insertmacro SelectSection ${SecDesktop}
    !insertmacro SelectSection ${SecStartup}
  ${Else}
    StrCpy $InstallMode "standard"
    !insertmacro SelectSection ${SecCoreFiles}
    !insertmacro SelectSection ${SecPATH}
    !insertmacro SelectSection ${SecDesktop}
    !insertmacro SelectSection ${SecStartup}
  ${EndIf}
FunctionEnd

Function ComponentsPageShowPre
  ${If} $InstallMode == "portable"
    ; Hide Components page in portable mode so optional standard-only items aren't shown.
    Abort
  ${EndIf}
FunctionEnd

Function DirectoryPageLeave
  ${If} $InstallMode == "portable"
    StrCpy $0 "0"

    ${StrStr} $1 "$INSTDIR" "$PROGRAMFILES64"
    ${If} $1 != ""
      StrCpy $0 "1"
    ${EndIf}

    ${StrStr} $1 "$INSTDIR" "$PROGRAMFILES"
    ${If} $1 != ""
      StrCpy $0 "1"
    ${EndIf}

    ${StrStr} $1 "$INSTDIR" "$WINDIR"
    ${If} $1 != ""
      StrCpy $0 "1"
    ${EndIf}

    ${If} $0 == "1"
      MessageBox MB_ICONEXCLAMATION|MB_OK "Portable mode cannot install into system-protected folders (Program Files / Windows). Go back and choose Standard mode for this location."
      Abort
    ${EndIf}
  ${EndIf}
FunctionEnd

Section "Desktop Shortcut" SecDesktop

  ${If} $InstallMode == "portable"
    DetailPrint "Portable mode selected: skipping desktop shortcut."
    Return
  ${EndIf}

  ; Create desktop shortcut
  CreateShortCut "$DESKTOP\Novadesk.lnk" "$INSTDIR\Novadesk.exe" "" "$INSTDIR\Novadesk.exe" 0
  
SectionEnd

Section "Run on Startup" SecStartup

  ${If} $InstallMode == "portable"
    DetailPrint "Portable mode selected: skipping startup shortcut."
    Return
  ${EndIf}

  ; Create startup shortcut
  CreateShortCut "$SMSTARTUP\Novadesk.lnk" "$INSTDIR\Novadesk.exe" "" "$INSTDIR\Novadesk.exe" 0
  
  ; Ensure Windows StartupApps state is reset to enabled for this shortcut.
  ; If user previously disabled it in Task Manager, the disabled flag persists.
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\StartupFolder" "Novadesk.lnk"
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run" "Novadesk"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\StartupFolder" "Novadesk.lnk"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run" "Novadesk"
  
SectionEnd

;--------------------------------
; Descriptions
;--------------------------------
LangString DESC_SecPATH ${LANG_ENGLISH} "Add nwm to your system PATH."
LangString DESC_SecDesktop ${LANG_ENGLISH} "Create a desktop shortcut."
LangString DESC_SecStartup ${LANG_ENGLISH} "Run Novadesk automatically when Windows starts."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SecPATH} $(DESC_SecPATH)
!insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} $(DESC_SecDesktop)
!insertmacro MUI_DESCRIPTION_TEXT ${SecStartup} $(DESC_SecStartup)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Uninstaller
;--------------------------------
Section "Uninstall"

  SetRegView 64
  ; Read the installation directory from the registry
  ReadRegStr $INSTDIR HKLM "Software\Novadesk" "Install_Dir"
  
  ; Kill process if running
  nsExec::ExecToStack 'taskkill /F /IM "Novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "nwm.exe"'

  ; Remove from PATH
  EnVar::SetHKLM
  EnVar::DeleteValue "PATH" "$INSTDIR\nwm"
  Pop $0
  DetailPrint "Remove from PATH returned=|$0|"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk"
  DeleteRegKey HKLM "Software\Novadesk"

  ; Remove files
  Delete "$INSTDIR\Novadesk.exe"
  Delete "$INSTDIR\nwm\installer_stub.exe"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$INSTDIR\Widgets"
  
  ; Remove AppData (Settings, Logs, Config)
  RMDir /r "$APPDATA\Novadesk"

  ; Remove files from root directory if in portable mode
  Delete "$INSTDIR\settings.json"
  Delete "$INSTDIR\logs.log"
  Delete "$INSTDIR\config.json"
  
  ; Remove nwm directory
  RMDir /r "$INSTDIR\nwm"

  ; Remove directories used
  RMDir "$INSTDIR"
  
  ; Remove desktop shortcut
  Delete "$DESKTOP\Novadesk.lnk"
  Delete "$SMSTARTUP\Novadesk.lnk"

SectionEnd
