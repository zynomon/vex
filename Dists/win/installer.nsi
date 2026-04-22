!define APPNAME "Vex"
!define VERSION "4.2"
!define PUBLISHER "Zynomon Aelius"
!define VENDOR "Zynomon Aelius"
!define SUMMARY "Extensive text editor"
!define LICENSE "Apache 2.0"
!define URL "https://github.com/zynomon/vex"
!define REGPATH_UNINSTSUBKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

Name "${APPNAME} ${VERSION}"
OutFile "${APPNAME}_${VERSION}_Setup.exe"
InstallDir "$PROGRAMFILES\${APPNAME}"
InstallDirRegKey HKLM "Software\${APPNAME}" ""

!define MUI_ICON "vexpkg.ico"
!define MUI_UNICON "vexpkg.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "banner.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "banner.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "header.bmp"
!define MUI_HEADERIMAGE_RIGHT
!define MUI_BRANDINGTEXT ""
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_BITMAP "banner.bmp"
!define MUI_UNFINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_BITMAP "banner.bmp"
!define MUI_FINISHPAGE_LINK "Visit ${APPNAME} on GitHub"
!define MUI_FINISHPAGE_LINK_LOCATION "${URL}"
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APPNAME}.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${APPNAME}"

!define MUI_WELCOMEPAGE_TITLE "Welcome to ${APPNAME} ${VERSION} Setup"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of ${APPNAME}.$\r$\n$\r$\nVex is an ${SUMMARY} with Qt style, theming and tons of features.$\r$\n$\r$\n$_CLICK"

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "WinMessages.nsh"
!include "nsDialogs.nsh"
!include "LogicLib.nsh"

BrandingText " "

Var AssociateCheckbox
Var DesktopCheckbox
Var TaskbarCheckbox
Var AssociateState
Var DesktopState
Var TaskbarState

Var InstallModeUser
Var InstallModeGlobal
Var InstallModeCustom
Var InstallModeState
Var hWndPathEdit

!define PREF_REG_PATH "Software\${APPNAME}\InstallerPrefs"

Function .onInit

  ReadRegStr $0 HKCU "${PREF_REG_PATH}" "InstallMode"
  ${If} $0 != ""

    StrCpy $InstallModeState $0

    ReadRegStr $DesktopState HKCU "${PREF_REG_PATH}" "DesktopState"
    ReadRegStr $TaskbarState HKCU "${PREF_REG_PATH}" "TaskbarState"
    ReadRegStr $AssociateState HKCU "${PREF_REG_PATH}" "AssociateState"
    ReadRegStr $INSTDIR HKCU "${PREF_REG_PATH}" "InstallPath"

    DeleteRegKey HKCU "${PREF_REG_PATH}"

    SetAutoClose true
    Return
  ${EndIf}

FunctionEnd

Function WelcomePre

  ReadRegStr $0 HKCU "${PREF_REG_PATH}" "InstallMode"
  ${If} $0 != ""
    Abort

  ${EndIf}
FunctionEnd

Function OptionsPage
  !insertmacro MUI_HEADER_TEXT "Additional Options" "Configure additional installation options"

  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}

  ${NSD_CreateLabel} 0u 0u 100% 20u "Select additional tasks to perform during installation:"
  Pop $0

  ${NSD_CreateCheckbox} 10u 25u 100% 10u "Create &desktop shortcut"
  Pop $DesktopCheckbox
  ${NSD_Check} $DesktopCheckbox

  ${NSD_CreateCheckbox} 10u 40u 100% 10u "&Pin to taskbar (requires restart)"
  Pop $TaskbarCheckbox

  ${NSD_CreateCheckbox} 10u 60u 100% 10u "Add ${APPNAME} to 'Open with' menu for all files"
  Pop $AssociateCheckbox
  ${NSD_Check} $AssociateCheckbox

  ${NSD_CreateLabel} 20u 75u 100% 40u "Vex can open any file. When enabled, it will appear in the$\r$\n'Open with' context menu for all file types. Files opened$\r$\nwith Vex will display the Vex file icon (txt.ico)."
  Pop $0

  nsDialogs::Show
FunctionEnd

Function OptionsPageLeave
  ${NSD_GetState} $DesktopCheckbox $DesktopState
  ${NSD_GetState} $TaskbarCheckbox $TaskbarState
  ${NSD_GetState} $AssociateCheckbox $AssociateState
FunctionEnd

Function PathPage
  !insertmacro MUI_HEADER_TEXT "Installation Path" "Choose installation type and location for ${APPNAME}"

  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}

  ${NSD_CreateLabel} 0u 0u 100% 20u "Select where you want to install ${APPNAME}:"
  Pop $0

  ${NSD_CreateRadioButton} 10u 25u 100% 10u "&User (Current user only)"
  Pop $InstallModeUser
  ${NSD_OnClick} $InstallModeUser OnInstallModeChange

  ${NSD_CreateRadioButton} 10u 40u 100% 10u "&Global (All users)"
  Pop $InstallModeGlobal
  ${NSD_OnClick} $InstallModeGlobal OnInstallModeChange

  ${NSD_CreateRadioButton} 10u 55u 100% 10u "&Custom"
  Pop $InstallModeCustom
  ${NSD_OnClick} $InstallModeCustom OnInstallModeChange

  ${NSD_CreateGroupBox} 5u 75u 390u 40u "Path"
  Pop $0

  ${NSD_CreateText} 10u 87u 370u 12u "$INSTDIR"
  Pop $hWndPathEdit
  ${NSD_OnChange} $hWndPathEdit OnPathChange

  UserInfo::GetAccountType
  Pop $0

  ${If} $0 == "admin"

    ${NSD_Check} $InstallModeGlobal
    StrCpy $InstallModeState "GLOBAL"
    SendMessage $hWndPathEdit ${WM_SETTEXT} 0 "STR:$PROGRAMFILES\${APPNAME}"
    EnableWindow $hWndPathEdit 0
  ${Else}

    ${NSD_Check} $InstallModeUser
    StrCpy $InstallModeState "USER"
    SendMessage $hWndPathEdit ${WM_SETTEXT} 0 "STR:$LOCALAPPDATA\${APPNAME}"
    EnableWindow $hWndPathEdit 0
  ${EndIf}

  nsDialogs::Show
FunctionEnd

Function OnInstallModeChange
  Pop $R0

  ${NSD_GetState} $InstallModeUser $0
  ${If} $0 == ${BST_CHECKED}
    StrCpy $InstallModeState "USER"
    SendMessage $hWndPathEdit ${WM_SETTEXT} 0 "STR:$LOCALAPPDATA\${APPNAME}"
    EnableWindow $hWndPathEdit 0
    Return
  ${EndIf}

  ${NSD_GetState} $InstallModeGlobal $0
  ${If} $0 == ${BST_CHECKED}
    StrCpy $InstallModeState "GLOBAL"
    SendMessage $hWndPathEdit ${WM_SETTEXT} 0 "STR:$PROGRAMFILES\${APPNAME}"
    EnableWindow $hWndPathEdit 0
    Return
  ${EndIf}

  ${NSD_GetState} $InstallModeCustom $0
  ${If} $0 == ${BST_CHECKED}
    StrCpy $InstallModeState "CUSTOM"
    EnableWindow $hWndPathEdit 1
  ${EndIf}
FunctionEnd

Function OnPathChange
  ${NSD_GetText} $hWndPathEdit $0
  StrCpy $INSTDIR $0
FunctionEnd

Function PathPageLeave
  ${NSD_GetText} $hWndPathEdit $0
  StrCpy $INSTDIR $0

  ${If} $INSTDIR == ""
    MessageBox MB_OK|MB_ICONEXCLAMATION "Please enter an installation path."
    Abort
  ${EndIf}

  ${If} $InstallModeState == "GLOBAL"
  ${OrIf} $InstallModeState == "CUSTOM"
    UserInfo::GetAccountType
    Pop $0
    ${If} $0 != "admin"

      WriteRegStr HKCU "${PREF_REG_PATH}" "InstallMode" $InstallModeState
      WriteRegStr HKCU "${PREF_REG_PATH}" "DesktopState" $DesktopState
      WriteRegStr HKCU "${PREF_REG_PATH}" "TaskbarState" $TaskbarState
      WriteRegStr HKCU "${PREF_REG_PATH}" "AssociateState" $AssociateState
      WriteRegStr HKCU "${PREF_REG_PATH}" "InstallPath" $INSTDIR

      MessageBox MB_ICONINFORMATION|MB_OK \
        "Global and Custom installations require administrator privileges.$\r$\n$\r$\n\
        The installer will now restart with administrator rights and begin installation immediately."

      ExecShell "open" "$EXEPATH" "" SW_SHOW
      Abort
    ${EndIf}
  ${EndIf}

FunctionEnd

Function PinToTaskbar
  FileOpen $0 "$TEMP\pin.vbs" w
  FileWrite $0 'Set shell = CreateObject("Shell.Application")$\r$\n'
  FileWrite $0 'Set folder = shell.Namespace("$INSTDIR")$\r$\n'
  FileWrite $0 'Set file = folder.ParseName("${APPNAME}.exe")$\r$\n'
  FileWrite $0 'If Not (file Is Nothing) Then$\r$\n'
  FileWrite $0 '  file.InvokeVerb("taskbarpin")$\r$\n'
  FileWrite $0 'End If$\r$\n'
  FileClose $0

  ExecWait 'wscript.exe "$TEMP\pin.vbs"'
  Delete "$TEMP\pin.vbs"
FunctionEnd

Function RefreshIconCache
  System::Call 'shell32.dll::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
FunctionEnd

Function un.RefreshIconCache
  System::Call 'shell32.dll::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
FunctionEnd

!define MUI_PAGE_CUSTOMFUNCTION_PRE WelcomePre
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
Page custom OptionsPage OptionsPageLeave

Page custom PathPage PathPageLeave

!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "!${APPNAME}" SecMain
  SectionIn RO

  SetOutPath "$INSTDIR"

  File /r "*.exe"
  File /r "*.dll"
  File "vex.ico"
  File "txt.ico"
  File "vexpkg.ico"
  File "vxsyn.ico"
  File "LICENSE"

  ${If} ${FileExists} "platforms\*.*"
    SetOutPath "$INSTDIR\platforms"
    File /r "platforms\*.dll"
    SetOutPath "$INSTDIR"
  ${EndIf}

  ${If} ${FileExists} "styles\*.*"
    SetOutPath "$INSTDIR\styles"
    File /r "styles\*.dll"
    SetOutPath "$INSTDIR"
  ${EndIf}

  ${If} $InstallModeState == "GLOBAL"
  ${OrIf} $InstallModeState == "CUSTOM"

    WriteRegStr HKLM "Software\${APPNAME}" "Version" "${VERSION}"
    WriteRegStr HKLM "Software\${APPNAME}" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\${APPNAME}" "Vendor" "${VENDOR}"
    WriteRegStr HKLM "Software\${APPNAME}" "URL" "${URL}"
    WriteRegStr HKLM "Software\${APPNAME}" "License" "${LICENSE}"

    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\${APPNAME}.exe" \
      "" "$INSTDIR\${APPNAME}.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\${APPNAME}.exe" \
      "Path" "$INSTDIR"

    WriteRegStr HKLM "${REGPATH_UNINSTSUBKEY}" "DisplayName" "${APPNAME} ${VERSION}"
    WriteRegStr HKLM "${REGPATH_UNINSTSUBKEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKLM "${REGPATH_UNINSTSUBKEY}" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
    WriteRegStr HKLM "${REGPATH_UNINSTSUBKEY}" "DisplayIcon" "$INSTDIR\vex.ico,0"
    WriteRegStr HKLM "${REGPATH_UNINSTSUBKEY}" "Publisher" "${PUBLISHER}"
    WriteRegStr HKLM "${REGPATH_UNINSTSUBKEY}" "DisplayVersion" "${VERSION}"
    WriteRegStr HKLM "${REGPATH_UNINSTSUBKEY}" "URLInfoAbout" "${URL}"
    WriteRegStr HKLM "${REGPATH_UNINSTSUBKEY}" "Comments" "${SUMMARY}"
    WriteRegDWORD HKLM "${REGPATH_UNINSTSUBKEY}" "NoModify" 1
    WriteRegDWORD HKLM "${REGPATH_UNINSTSUBKEY}" "NoRepair" 1

    WriteRegStr HKCR "Vex.File" "" "Vex Document"
    WriteRegStr HKCR "Vex.File\DefaultIcon" "" "$INSTDIR\txt.ico,0"
    WriteRegStr HKCR "Vex.File\shell\open\command" "" '"$INSTDIR\${APPNAME}.exe" -f "%1"'
    WriteRegStr HKCR "Vex.File\shell\open" "FriendlyAppName" "${APPNAME}"

    WriteRegStr HKCR "Vex.SyntaxFile" "" "Vex Syntax File"
    WriteRegStr HKCR "Vex.SyntaxFile\DefaultIcon" "" "$INSTDIR\vxsyn.ico,0"
    WriteRegStr HKCR "Vex.SyntaxFile\shell\open\command" "" '"$INSTDIR\${APPNAME}.exe" -f "%1"'
    WriteRegStr HKCR "Vex.SyntaxFile\shell\open" "FriendlyAppName" "${APPNAME}"

    WriteRegStr HKCR "Applications\${APPNAME}.exe" "FriendlyAppName" "${APPNAME}"
    WriteRegStr HKCR "Applications\${APPNAME}.exe\shell\open" "FriendlyAppName" "${APPNAME}"
    WriteRegStr HKCR "Applications\${APPNAME}.exe\shell\open\command" "" \
      '"$INSTDIR\${APPNAME}.exe" -f "%1"'
    WriteRegStr HKCR "Applications\${APPNAME}.exe\DefaultIcon" "" "$INSTDIR\txt.ico,0"

    WriteRegStr HKCR "*\OpenWithList\${APPNAME}.exe" "" ""

    ${If} $AssociateState == ${BST_CHECKED}

      WriteRegStr HKLM "Software\${APPNAME}\Capabilities" "ApplicationName" "${APPNAME}"
      WriteRegStr HKLM "Software\${APPNAME}\Capabilities" "ApplicationDescription" "${SUMMARY}"
      WriteRegStr HKLM "Software\${APPNAME}\Capabilities\ApplicationIcon" "" "$INSTDIR\txt.ico,0"
      WriteRegStr HKLM "Software\${APPNAME}\Capabilities\ApplicationDisplayName" "" "${APPNAME}"

      WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" "*" "Vex.File"

      WriteRegStr HKLM "Software\RegisteredApplications" "${APPNAME}" \
        "Software\${APPNAME}\Capabilities"

      WriteRegStr HKCR ".vxsyn" "" "Vex.SyntaxFile"
      WriteRegStr HKCR ".vxsyn\OpenWithProgids" "Vex.SyntaxFile" ""
    ${EndIf}

  ${Else}

    WriteRegStr HKCU "Software\${APPNAME}" "Version" "${VERSION}"
    WriteRegStr HKCU "Software\${APPNAME}" "InstallDir" "$INSTDIR"
    WriteRegStr HKCU "Software\${APPNAME}" "Vendor" "${VENDOR}"
    WriteRegStr HKCU "Software\${APPNAME}" "URL" "${URL}"
    WriteRegStr HKCU "Software\${APPNAME}" "License" "${LICENSE}"

    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\App Paths\${APPNAME}.exe" \
      "" "$INSTDIR\${APPNAME}.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\App Paths\${APPNAME}.exe" \
      "Path" "$INSTDIR"

    WriteRegStr HKCU "${REGPATH_UNINSTSUBKEY}" "DisplayName" "${APPNAME} ${VERSION}"
    WriteRegStr HKCU "${REGPATH_UNINSTSUBKEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKCU "${REGPATH_UNINSTSUBKEY}" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
    WriteRegStr HKCU "${REGPATH_UNINSTSUBKEY}" "DisplayIcon" "$INSTDIR\vex.ico,0"
    WriteRegStr HKCU "${REGPATH_UNINSTSUBKEY}" "Publisher" "${PUBLISHER}"
    WriteRegStr HKCU "${REGPATH_UNINSTSUBKEY}" "DisplayVersion" "${VERSION}"
    WriteRegStr HKCU "${REGPATH_UNINSTSUBKEY}" "URLInfoAbout" "${URL}"
    WriteRegStr HKCU "${REGPATH_UNINSTSUBKEY}" "Comments" "${SUMMARY}"
    WriteRegDWORD HKCU "${REGPATH_UNINSTSUBKEY}" "NoModify" 1
    WriteRegDWORD HKCU "${REGPATH_UNINSTSUBKEY}" "NoRepair" 1

    WriteRegStr HKCU "Software\Classes\Vex.File" "" "Vex Document"
    WriteRegStr HKCU "Software\Classes\Vex.File\DefaultIcon" "" "$INSTDIR\txt.ico,0"
    WriteRegStr HKCU "Software\Classes\Vex.File\shell\open\command" "" '"$INSTDIR\${APPNAME}.exe" -f "%1"'
    WriteRegStr HKCU "Software\Classes\Vex.File\shell\open" "FriendlyAppName" "${APPNAME}"

    WriteRegStr HKCU "Software\Classes\Vex.SyntaxFile" "" "Vex Syntax File"
    WriteRegStr HKCU "Software\Classes\Vex.SyntaxFile\DefaultIcon" "" "$INSTDIR\vxsyn.ico,0"
    WriteRegStr HKCU "Software\Classes\Vex.SyntaxFile\shell\open\command" "" '"$INSTDIR\${APPNAME}.exe" -f "%1"'
    WriteRegStr HKCU "Software\Classes\Vex.SyntaxFile\shell\open" "FriendlyAppName" "${APPNAME}"

    WriteRegStr HKCU "Software\Classes\Applications\${APPNAME}.exe" "FriendlyAppName" "${APPNAME}"
    WriteRegStr HKCU "Software\Classes\Applications\${APPNAME}.exe\shell\open" "FriendlyAppName" "${APPNAME}"
    WriteRegStr HKCU "Software\Classes\Applications\${APPNAME}.exe\shell\open\command" "" \
      '"$INSTDIR\${APPNAME}.exe" -f "%1"'
    WriteRegStr HKCU "Software\Classes\Applications\${APPNAME}.exe\DefaultIcon" "" "$INSTDIR\txt.ico,0"

    WriteRegStr HKCU "Software\Classes\*\OpenWithList\${APPNAME}.exe" "" ""

    ${If} $AssociateState == ${BST_CHECKED}

      WriteRegStr HKCU "Software\Classes\.vxsyn" "" "Vex.SyntaxFile"
      WriteRegStr HKCU "Software\Classes\.vxsyn\OpenWithProgids" "Vex.SyntaxFile" ""
    ${EndIf}
  ${EndIf}

  ${If} $DesktopState == ${BST_CHECKED}
    CreateShortCut "$DESKTOP\${APPNAME}.lnk" \
      "$INSTDIR\${APPNAME}.exe" \
      "" \
      "$INSTDIR\vex.ico" 0 \
      SW_SHOWNORMAL \
      "" \
      "${SUMMARY}"
  ${EndIf}

  CreateDirectory "$SMPROGRAMS\${APPNAME}"
  CreateShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" \
    "$INSTDIR\${APPNAME}.exe" \
    "" \
    "$INSTDIR\vex.ico" 0 \
    SW_SHOWNORMAL \
    "" \
    "${SUMMARY}"
  WriteINIStr "$SMPROGRAMS\${APPNAME}\${APPNAME} Wiki.url" \
    "InternetShortcut" \
    "URL" \
    "${URL}/wiki"
  CreateShortCut "$SMPROGRAMS\${APPNAME}\Uninstall ${APPNAME}.lnk" \
    "$INSTDIR\Uninstall.exe" \
    "" \
    "$INSTDIR\vex.ico" 0 \
    SW_SHOWNORMAL \
    "" \
    "Uninstall ${APPNAME}"

  ${If} $TaskbarState == ${BST_CHECKED}
    Call PinToTaskbar
  ${EndIf}

  WriteUninstaller "$INSTDIR\Uninstall.exe"
  Call RefreshIconCache
SectionEnd

Section "Uninstall"

  Delete "$DESKTOP\${APPNAME}.lnk"
  Delete "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"
  Delete "$SMPROGRAMS\${APPNAME}\${APPNAME} Wiki.url"
  Delete "$SMPROGRAMS\${APPNAME}\Uninstall ${APPNAME}.lnk"
  RMDir "$SMPROGRAMS\${APPNAME}"

  DeleteRegKey HKLM "Software\${APPNAME}"
  DeleteRegKey HKCU "Software\${APPNAME}"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\${APPNAME}.exe"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\App Paths\${APPNAME}.exe"

  DeleteRegKey HKLM "${REGPATH_UNINSTSUBKEY}"
  DeleteRegKey HKCU "${REGPATH_UNINSTSUBKEY}"

  DeleteRegValue HKLM "Software\RegisteredApplications" "${APPNAME}"
  DeleteRegValue HKCU "Software\RegisteredApplications" "${APPNAME}"

  DeleteRegKey HKCR "Applications\${APPNAME}.exe"
  DeleteRegKey HKCU "Software\Classes\Applications\${APPNAME}.exe"

  DeleteRegKey HKCR "Vex.File"
  DeleteRegKey HKCR "Vex.SyntaxFile"
  DeleteRegKey HKCU "Software\Classes\Vex.File"
  DeleteRegKey HKCU "Software\Classes\Vex.SyntaxFile"

  DeleteRegKey HKCR "*\OpenWithList\${APPNAME}.exe"
  DeleteRegKey HKCU "Software\Classes\*\OpenWithList\${APPNAME}.exe"

  DeleteRegKey HKCR ".vxsyn"
  DeleteRegKey HKCU "Software\Classes\.vxsyn"

  DeleteRegKey HKCU "${PREF_REG_PATH}"

  ReadRegStr $0 HKLM "Software\${APPNAME}" "InstallDir"
  ${If} $0 == ""
    ReadRegStr $0 HKCU "Software\${APPNAME}" "InstallDir"
  ${EndIf}

  ${If} $0 != ""
    RMDir /r "$0"
  ${Else}
    RMDir /r "$INSTDIR"
  ${EndIf}

  MessageBox MB_YESNO|MB_ICONQUESTION \
    "Do you want to remove your ${APPNAME} configuration files?$\r$\n$\r$\n$PROFILE\.vex" \
    IDNO skip_configs

  RMDir /r "$PROFILE\.vex"

  skip_configs:
  Call un.RefreshIconCache
SectionEnd

VIProductVersion "4.2.0.0"
VIAddVersionKey "ProductName" "${APPNAME}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" "© ${VENDOR}"
VIAddVersionKey "FileDescription" "${SUMMARY}"
VIAddVersionKey "CompanyName" "${VENDOR}"
VIAddVersionKey "LegalTrademarks" "${LICENSE}"
VIAddVersionKey "Comments" "Visit ${URL} for more information"
