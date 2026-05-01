;--------------------------------
; Novadesk Installer Script
;--------------------------------

; Define global version variable (Update this as needed)
!define VERSION "0.9.3.0"

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
${StrRep}

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
!insertmacro MUI_PAGE_INSTFILES

; Finish page settings
!define MUI_FINISHPAGE_RUN "$INSTDIR\manage_novadesk.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Run Novadesk"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
UninstPage custom un.CompleteRemovePageCreate un.CompleteRemovePageLeave
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Data
;--------------------------------
Var InstallMode
Var RadioStandard
Var RadioPortable
Var DocsRoot
Var ScriptsRoot
Var RemoveCompletely
Var UnRemoveCheckbox

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
  nsExec::ExecToStack 'taskkill /F /IM "novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "Novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "manage_novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "restart_novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "ndpkg_installer.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "nwm.exe"'
  Sleep 1000

  ; Add Novadesk files from dist
  SetOutPath "$INSTDIR"
  File "..\dist\novadesk.exe"
  File "..\dist\manage_novadesk.exe"
  File "..\dist\restart_novadesk.exe"
  File "..\dist\ndpkg_installer.exe"
  File /r "..\dist\images"

  ; Add installer stub from dist
  SetOutPath "$INSTDIR\nwm"
  File "..\dist\nwm\installer_stub.exe"
  
  ; Add nwm files from dist
  SetOutPath "$INSTDIR\nwm"
  File "..\dist\nwm\nwm.exe"
  ; Copy nwm template if it exists in dist
  File /r "..\dist\nwm\template"
  SetOutPath "$INSTDIR"

  ${If} $InstallMode == "standard"
    StrCpy $DocsRoot "$DOCUMENTS\Novadesk"
    CreateDirectory "$DocsRoot"
    ; In standard mode, widgets/addons live in Documents\Novadesk
    SetOutPath "$DocsRoot"
    File /r "..\dist\Widgets"
    File /r "..\dist\Addons"

    ; Create settings in AppData\Novadesk
    CreateDirectory "$APPDATA\Novadesk"
    StrCpy $ScriptsRoot "$DocsRoot\Widgets\Fental\index.js"
    ${StrRep} $1 $ScriptsRoot "\" "\\"
    FileOpen $0 "$APPDATA\Novadesk\manage_novadesk_settings.json" "w"
    FileWrite $0 "{$\r$\n"
    FileWrite $0 "  $\"loadedScripts$\": [$\r$\n"
    FileWrite $0 "    $\"$1$\"$\r$\n"
    FileWrite $0 "  ]$\r$\n"
    FileWrite $0 "}$\r$\n"
    FileClose $0

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

    ; Always create shortcuts in standard mode
    CreateDirectory "$SMPROGRAMS\Novadesk"
    CreateShortCut "$SMPROGRAMS\Novadesk\Novadesk.lnk" "$INSTDIR\manage_novadesk.exe" "" "$INSTDIR\manage_novadesk.exe" 0
    CreateShortCut "$DESKTOP\Novadesk.lnk" "$INSTDIR\manage_novadesk.exe" "" "$INSTDIR\manage_novadesk.exe" 0

    ; Always add to PATH in standard mode
    EnVar::SetHKLM
    EnVar::AddValue "PATH" "$INSTDIR"
    Pop $0
    DetailPrint "Add root to PATH returned=|$0|"
    EnVar::AddValue "PATH" "$INSTDIR\nwm"
    Pop $0
    DetailPrint "Add nwm to PATH returned=|$0|"

    ; Register .ndpkg file association with ndpkg_installer
    WriteRegStr HKLM "Software\Classes\.ndpkg" "" "Novadesk.ndpkg"
    WriteRegStr HKLM "Software\Classes\Novadesk.ndpkg" "" "Novadesk Package"
    WriteRegStr HKLM "Software\Classes\Novadesk.ndpkg\DefaultIcon" "" "$INSTDIR\ndpkg_installer.exe,0"
    WriteRegStr HKLM "Software\Classes\Novadesk.ndpkg\shell" "" "open"
    WriteRegStr HKLM "Software\Classes\Novadesk.ndpkg\shell\open\command" "" '$\"$INSTDIR\ndpkg_installer.exe$\" $\"%1$\"'
    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
  ${Else}
    ; In portable mode, widgets/addons live in install root
    SetOutPath "$INSTDIR"
    File /r "..\dist\Widgets"
    File /r "..\dist\Addons"

    ; Create manage_novadesk_settings.json in installation root
    StrCpy $ScriptsRoot "$INSTDIR\Widgets\Fental\index.js"
    ${StrRep} $1 $ScriptsRoot "\" "\\"
    FileOpen $0 "$INSTDIR\manage_novadesk_settings.json" "w"
    FileWrite $0 "{$\r$\n"
    FileWrite $0 "  $\"loadedScripts$\": [$\r$\n"
    FileWrite $0 "    $\"$1$\"$\r$\n"
    FileWrite $0 "  ]$\r$\n"
    FileWrite $0 "}$\r$\n"
    FileClose $0

    DetailPrint "Portable mode selected: skipped uninstaller and global registry writes."
  ${EndIf}

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
  ${Else}
    StrCpy $InstallMode "standard"
  ${EndIf}
FunctionEnd

Function un.CompleteRemovePageCreate
  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}

  ${NSD_CreateLabel} 0 0 100% 24u "Uninstall Options"
  Pop $0
  ${NSD_CreateCheckbox} 0 28u 100% 14u "Completely Remove Novadesk (remove Documents\Novadesk and AppData\Novadesk)"
  Pop $UnRemoveCheckbox
  ${NSD_Check} $UnRemoveCheckbox
  StrCpy $RemoveCompletely ${BST_CHECKED}
  nsDialogs::Show
FunctionEnd

Function un.CompleteRemovePageLeave
  ${NSD_GetState} $UnRemoveCheckbox $RemoveCompletely
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

;--------------------------------
; Uninstaller
;--------------------------------
Section "Uninstall"

  SetRegView 64
  ; Read the installation directory from the registry
  ReadRegStr $INSTDIR HKLM "Software\Novadesk" "Install_Dir"
  
  ; Kill process if running
  nsExec::ExecToStack 'taskkill /F /IM "novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "manage_novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "restart_novadesk.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "ndpkg_installer.exe"'
  nsExec::ExecToStack 'taskkill /F /IM "nwm.exe"'
  Sleep 1000

  ; Remove from PATH
  EnVar::SetHKLM
  EnVar::DeleteValue "PATH" "$INSTDIR"
  Pop $0
  DetailPrint "Remove root from PATH returned=|$0|"
  EnVar::DeleteValue "PATH" "$INSTDIR\nwm"
  Pop $0
  DetailPrint "Remove nwm from PATH returned=|$0|"
  
  ; Always remove registry entries
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Novadesk"
  DeleteRegKey HKLM "Software\Novadesk"
  DeleteRegKey HKLM "Software\Classes\Novadesk.ndpkg"
  DeleteRegKey HKLM "Software\Classes\.ndpkg"
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'

  ; Remove files
  Delete "$INSTDIR\novadesk.exe"
  Delete "$INSTDIR\manage_novadesk.exe"
  Delete "$INSTDIR\restart_novadesk.exe"
  Delete "$INSTDIR\ndpkg_installer.exe"
  Delete "$INSTDIR\nwm\installer_stub.exe"
  Delete "$INSTDIR\nwm\nwm.exe"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$INSTDIR\images"
  RMDir /r "$INSTDIR\Widgets"
  RMDir /r "$INSTDIR\Addons"
  
  ${If} $RemoveCompletely == ${BST_CHECKED}
    ; Completely remove user data only when explicitly requested
    RMDir /r "$APPDATA\Novadesk"
    RMDir /r "$DOCUMENTS\Novadesk"
  ${Else}
    DetailPrint "Keeping user data folders (Documents\Novadesk and AppData\Novadesk)."
  ${EndIf}

  ; Remove files from root directory if in portable mode
  Delete "$INSTDIR\settings.json"
  Delete "$INSTDIR\logs.log"
  Delete "$INSTDIR\config.json"
  Delete "$INSTDIR\manage_novadesk_settings.json"
  
  ; Remove nwm directory
  RMDir /r "$INSTDIR\nwm"

  ; Remove directories used
  RMDir "$INSTDIR"
  
  ; Remove desktop shortcut
  Delete "$DESKTOP\Novadesk.lnk"
  Delete "$SMPROGRAMS\Novadesk\Novadesk.lnk"
  RMDir "$SMPROGRAMS\Novadesk"

SectionEnd
