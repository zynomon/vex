!define APPNAME "Vex"
!define VERSION "4.1"
!define PUBLISHER "Zynomon Aelius"
!define VENDOR "Zynomon Aelius"
!define SUMMARY "Extensive Qt C++ text editor,"
!define LICENSE "Apache 2.0"
!define URL "https://github.com/zynomon/vex"
!define REGPATH_UNINSTSUBKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

Name "${APPNAME} ${VERSION}"
OutFile "${APPNAME}-${VERSION}.exe"
InstallDir "$PROGRAMFILES\${APPNAME}"
InstallDirRegKey HKLM "Software\${APPNAME}" ""

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "WinMessages.nsh"
!include "nsDialogs.nsh"
!include "LogicLib.nsh"
!include "StrFunc.nsh"
${StrTok}

!define MUI_ICON "vexpkg.ico"
!define MUI_UNICON "vexpkg.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "banner.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "header.bmp"
!define MUI_HEADERIMAGE_RIGHT
!define MUI_BRANDINGTEXT ""
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_LINK "Visit ${APPNAME} on GitHub"
!define MUI_FINISHPAGE_LINK_LOCATION "${URL}"
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APPNAME}.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${APPNAME}"

!define MUI_WELCOMEPAGE_TITLE "Welcome to ${APPNAME} ${VERSION} Setup"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of ${APPNAME}.$\r$\n$\r$\nVex is an ${SUMMARY} With Qtstyle , theming and tons of features$\r$\n$\r$\n$_CLICK"

BrandingText " "

Var DesktopShortcut
Var TaskbarPin
Var FileAssoc
Var DesktopShortcutState
Var TaskbarPinState
Var FileAssocState

Function customOptionsPage
  !insertmacro MUI_HEADER_TEXT "Installation Options" "Choose additional options for ${APPNAME}"

  nsDialogs::Create 1018
  Pop $0
  ${If} $0 == error
    Abort
  ${EndIf}

  ${NSD_CreateLabel} 0u 0u 100% 20u \
    "Select additional tasks to perform during installation:"
  Pop $0

  ${NSD_CreateCheckbox} 10u 25u 100% 12u \
    "Create desktop shortcut"
  Pop $DesktopShortcut
  ${NSD_Check} $DesktopShortcut

  ${NSD_CreateCheckbox} 10u 42u 100% 12u \
    "Pin to taskbar (requires restart)"
  Pop $TaskbarPin
  ${NSD_Uncheck} $TaskbarPin

  ${NSD_CreateCheckbox} 10u 59u 100% 12u \
    "Associate ${APPNAME} with common text files"
  Pop $FileAssoc
  ${NSD_Check} $FileAssoc

  nsDialogs::Show
FunctionEnd

Function customOptionsPageLeave
  ${NSD_GetState} $DesktopShortcut $DesktopShortcutState
  ${NSD_GetState} $TaskbarPin $TaskbarPinState
  ${NSD_GetState} $FileAssoc $FileAssocState
FunctionEnd

Function RefreshIconCache
  System::Call 'shell32.dll::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
FunctionEnd

Function un.RefreshIconCache
  System::Call 'shell32.dll::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
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

!macro AssociateExtension extension progid description iconPath
  WriteRegStr HKCR "${progid}" "" "${description}"
  WriteRegStr HKCR "${progid}\DefaultIcon" "" "${iconPath},0"
  WriteRegStr HKCR "${progid}\shell\open\command" "" '"$INSTDIR\${APPNAME}.exe" -f "%1"'

  WriteRegStr HKCR "${progid}\FriendlyAppName" "" "${APPNAME}"

  WriteRegStr HKCR "${extension}" "" "${progid}"
  WriteRegStr HKCR "${extension}\OpenWithProgids" "${progid}" ""

  WriteRegStr HKCR "${extension}\OpenWithList\${APPNAME}.exe" "" ""
!macroend

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
Page custom customOptionsPage customOptionsPageLeave
!insertmacro MUI_PAGE_DIRECTORY
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

  ${If} ${Errors}
    MessageBox MB_ICONSTOP "Failed to copy files. Installation aborted."
    Abort
  ${EndIf}

  WriteRegStr HKLM "Software\${APPNAME}" "Version" "${VERSION}"
  WriteRegStr HKLM "Software\${APPNAME}" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\${APPNAME}" "Vendor" "${VENDOR}"
  WriteRegStr HKLM "Software\${APPNAME}" "License" "${LICENSE}"
  WriteRegStr HKLM "Software\${APPNAME}" "URL" "${URL}"

  ${If} $DesktopShortcutState == ${BST_CHECKED}
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

  ${If} $TaskbarPinState == ${BST_CHECKED}
    Call PinToTaskbar
  ${EndIf}

  WriteRegStr HKCR "Applications\${APPNAME}.exe" "" "${APPNAME}"
  WriteRegStr HKCR "Applications\${APPNAME}.exe\FriendlyAppName" "" "${APPNAME}"

  WriteRegStr HKCR "Applications\${APPNAME}.exe\DefaultIcon" "" "$INSTDIR\vex.ico,0"
  WriteRegStr HKCR "Applications\${APPNAME}.exe\shell\open\command" "" \
    '"$INSTDIR\${APPNAME}.exe" -f "%1"'

  WriteRegStr HKLM "Software\${APPNAME}\Capabilities" "ApplicationName" "${APPNAME}"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities" "ApplicationDescription" "${SUMMARY}"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\ApplicationIcon" "" "$INSTDIR\vex.ico,0"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\ApplicationDisplayName" "" "${APPNAME}"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\HelpUrl" "" "${URL}"

  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".txt" "Vex.TextFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".log" "Vex.LogFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".md" "Vex.MarkdownFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".ini" "Vex.INIFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".conf" "Vex.ConfigFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".cfg" "Vex.ConfigFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".properties" "Vex.PropertiesFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".toml" "Vex.TOMLFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".yml" "Vex.YAMLFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".yaml" "Vex.YAMLFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".json" "Vex.JSONFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".xml" "Vex.XMLFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".sh" "Vex.ShellScriptFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".bash" "Vex.BashFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".py" "Vex.PythonFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".rb" "Vex.RubyFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".lua" "Vex.LuaFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".php" "Vex.PHPFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".js" "Vex.JavaScriptFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".c" "Vex.CSourceFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".h" "Vex.CHeaderFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".cpp" "Vex.CPPSourceFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".cxx" "Vex.CPPSourceFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".cc" "Vex.CPPSourceFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".hpp" "Vex.CPPHeaderFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".hxx" "Vex.CPPHeaderFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".hh" "Vex.CPPHeaderFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".html" "Vex.HTMLFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".htm" "Vex.HTMLFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".css" "Vex.CSSFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".sql" "Vex.SQLFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".tex" "Vex.LaTeXFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".bib" "Vex.BibTeXFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".cmake" "Vex.CMakeFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".mak" "Vex.MakefileFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".dockerfile" "Vex.DockerfileFile"
  WriteRegStr HKLM "Software\${APPNAME}\Capabilities\FileAssociations" ".vxsyn" "Vex.SyntaxFile"

  WriteRegStr HKLM "Software\RegisteredApplications" "${APPNAME}" \
    "Software\${APPNAME}\Capabilities"

  ${If} $FileAssocState == ${BST_CHECKED}

    !insertmacro AssociateExtension ".txt" "Vex.TextFile" \
      "Text Document" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".log" "Vex.LogFile" \
      "Log File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".md" "Vex.MarkdownFile" \
      "Markdown Document" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".ini" "Vex.INIFile" \
      "Configuration Settings File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".conf" "Vex.ConfigFile" \
      "Configuration File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".cfg" "Vex.ConfigFile" \
      "Configuration File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".properties" "Vex.PropertiesFile" \
      "Properties File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".toml" "Vex.TOMLFile" \
      "TOML Configuration File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".yml" "Vex.YAMLFile" \
      "YAML Source" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".yaml" "Vex.YAMLFile" \
      "YAML Source" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".json" "Vex.JSONFile" \
      "JSON Source" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".xml" "Vex.XMLFile" \
      "XML Document" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".sh" "Vex.ShellScriptFile" \
      "Shell Script" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".bash" "Vex.BashFile" \
      "Bash Script" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".py" "Vex.PythonFile" \
      "Python Source File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".rb" "Vex.RubyFile" \
      "Ruby Source File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".lua" "Vex.LuaFile" \
      "Lua Source File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".php" "Vex.PHPFile" \
      "PHP Source File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".js" "Vex.JavaScriptFile" \
      "JavaScript Source File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".c" "Vex.CSourceFile" \
      "C Source File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".h" "Vex.CHeaderFile" \
      "C Header File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".cpp" "Vex.CPPSourceFile" \
      "C++ Source File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".cxx" "Vex.CPPSourceFile" \
      "C++ Source File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".cc" "Vex.CPPSourceFile" \
      "C++ Source File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".hpp" "Vex.CPPHeaderFile" \
      "C++ Header File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".hxx" "Vex.CPPHeaderFile" \
      "C++ Header File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".hh" "Vex.CPPHeaderFile" \
      "C++ Header File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".html" "Vex.HTMLFile" \
      "HTML Document" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".htm" "Vex.HTMLFile" \
      "HTML Document" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".css" "Vex.CSSFile" \
      "CSS Source File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".sql" "Vex.SQLFile" \
      "SQL Source File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".tex" "Vex.LaTeXFile" \
      "LaTeX Document" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".bib" "Vex.BibTeXFile" \
      "BibTeX Bibliography File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".cmake" "Vex.CMakeFile" \
      "CMake File" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".mak" "Vex.MakefileFile" \
      "Makefile" "$INSTDIR\txt.ico"
    !insertmacro AssociateExtension ".dockerfile" "Vex.DockerfileFile" \
      "Dockerfile" "$INSTDIR\txt.ico"

  ${EndIf}

  !insertmacro AssociateExtension ".vxsyn" "Vex.SyntaxFile" \
    "Vex Syntax File" "$INSTDIR\vxsyn.ico"

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
  WriteRegStr HKLM "${REGPATH_UNINSTSUBKEY}" "HelpLink" "${URL}/issues"
  WriteRegStr HKLM "${REGPATH_UNINSTSUBKEY}" "URLUpdateInfo" "${URL}/releases"
  WriteRegStr HKLM "${REGPATH_UNINSTSUBKEY}" "Comments" "${SUMMARY}"
  WriteRegStr HKLM "${REGPATH_UNINSTSUBKEY}" "Contact" "${PUBLISHER}"
  WriteRegDWORD HKLM "${REGPATH_UNINSTSUBKEY}" "NoModify" 1
  WriteRegDWORD HKLM "${REGPATH_UNINSTSUBKEY}" "NoRepair" 1
  WriteRegDWORD HKLM "${REGPATH_UNINSTSUBKEY}" "EstimatedSize" 5000
  WriteRegDWORD HKLM "${REGPATH_UNINSTSUBKEY}" "VersionMajor" 4
  WriteRegDWORD HKLM "${REGPATH_UNINSTSUBKEY}" "VersionMinor" 1

  WriteUninstaller "$INSTDIR\Uninstall.exe"

  Call RefreshIconCache
SectionEnd

Section "Uninstall"

  Delete "$DESKTOP\${APPNAME}.lnk"
  Delete "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"
  Delete "$SMPROGRAMS\${APPNAME}\${APPNAME} Documentation.url"
  Delete "$SMPROGRAMS\${APPNAME}\Uninstall ${APPNAME}.lnk"
  RMDir "$SMPROGRAMS\${APPNAME}"

  DeleteRegKey HKCR "Applications\${APPNAME}.exe"

  DeleteRegKey HKCR "Vex.TextFile"
  DeleteRegKey HKCR "Vex.LogFile"
  DeleteRegKey HKCR "Vex.MarkdownFile"
  DeleteRegKey HKCR "Vex.INIFile"
  DeleteRegKey HKCR "Vex.ConfigFile"
  DeleteRegKey HKCR "Vex.PropertiesFile"
  DeleteRegKey HKCR "Vex.TOMLFile"
  DeleteRegKey HKCR "Vex.YAMLFile"
  DeleteRegKey HKCR "Vex.JSONFile"
  DeleteRegKey HKCR "Vex.XMLFile"
  DeleteRegKey HKCR "Vex.ShellScriptFile"
  DeleteRegKey HKCR "Vex.BashFile"
  DeleteRegKey HKCR "Vex.PythonFile"
  DeleteRegKey HKCR "Vex.RubyFile"
  DeleteRegKey HKCR "Vex.LuaFile"
  DeleteRegKey HKCR "Vex.PHPFile"
  DeleteRegKey HKCR "Vex.JavaScriptFile"
  DeleteRegKey HKCR "Vex.CSourceFile"
  DeleteRegKey HKCR "Vex.CHeaderFile"
  DeleteRegKey HKCR "Vex.CPPSourceFile"
  DeleteRegKey HKCR "Vex.CPPHeaderFile"
  DeleteRegKey HKCR "Vex.HTMLFile"
  DeleteRegKey HKCR "Vex.CSSFile"
  DeleteRegKey HKCR "Vex.SQLFile"
  DeleteRegKey HKCR "Vex.LaTeXFile"
  DeleteRegKey HKCR "Vex.BibTeXFile"
  DeleteRegKey HKCR "Vex.CMakeFile"
  DeleteRegKey HKCR "Vex.MakefileFile"
  DeleteRegKey HKCR "Vex.DockerfileFile"
  DeleteRegKey HKCR "Vex.SyntaxFile"

  DeleteRegValue HKCR ".txt\OpenWithProgids" "Vex.TextFile"
  DeleteRegValue HKCR ".log\OpenWithProgids" "Vex.LogFile"
  DeleteRegValue HKCR ".md\OpenWithProgids" "Vex.MarkdownFile"
  DeleteRegValue HKCR ".ini\OpenWithProgids" "Vex.INIFile"
  DeleteRegValue HKCR ".conf\OpenWithProgids" "Vex.ConfigFile"
  DeleteRegValue HKCR ".cfg\OpenWithProgids" "Vex.ConfigFile"
  DeleteRegValue HKCR ".properties\OpenWithProgids" "Vex.PropertiesFile"
  DeleteRegValue HKCR ".toml\OpenWithProgids" "Vex.TOMLFile"
  DeleteRegValue HKCR ".yml\OpenWithProgids" "Vex.YAMLFile"
  DeleteRegValue HKCR ".yaml\OpenWithProgids" "Vex.YAMLFile"
  DeleteRegValue HKCR ".json\OpenWithProgids" "Vex.JSONFile"
  DeleteRegValue HKCR ".xml\OpenWithProgids" "Vex.XMLFile"
  DeleteRegValue HKCR ".sh\OpenWithProgids" "Vex.ShellScriptFile"
  DeleteRegValue HKCR ".bash\OpenWithProgids" "Vex.BashFile"
  DeleteRegValue HKCR ".py\OpenWithProgids" "Vex.PythonFile"
  DeleteRegValue HKCR ".rb\OpenWithProgids" "Vex.RubyFile"
  DeleteRegValue HKCR ".lua\OpenWithProgids" "Vex.LuaFile"
  DeleteRegValue HKCR ".php\OpenWithProgids" "Vex.PHPFile"
  DeleteRegValue HKCR ".js\OpenWithProgids" "Vex.JavaScriptFile"
  DeleteRegValue HKCR ".c\OpenWithProgids" "Vex.CSourceFile"
  DeleteRegValue HKCR ".h\OpenWithProgids" "Vex.CHeaderFile"
  DeleteRegValue HKCR ".cpp\OpenWithProgids" "Vex.CPPSourceFile"
  DeleteRegValue HKCR ".cxx\OpenWithProgids" "Vex.CPPSourceFile"
  DeleteRegValue HKCR ".cc\OpenWithProgids" "Vex.CPPSourceFile"
  DeleteRegValue HKCR ".hpp\OpenWithProgids" "Vex.CPPHeaderFile"
  DeleteRegValue HKCR ".hxx\OpenWithProgids" "Vex.CPPHeaderFile"
  DeleteRegValue HKCR ".hh\OpenWithProgids" "Vex.CPPHeaderFile"
  DeleteRegValue HKCR ".html\OpenWithProgids" "Vex.HTMLFile"
  DeleteRegValue HKCR ".htm\OpenWithProgids" "Vex.HTMLFile"
  DeleteRegValue HKCR ".css\OpenWithProgids" "Vex.CSSFile"
  DeleteRegValue HKCR ".sql\OpenWithProgids" "Vex.SQLFile"
  DeleteRegValue HKCR ".tex\OpenWithProgids" "Vex.LaTeXFile"
  DeleteRegValue HKCR ".bib\OpenWithProgids" "Vex.BibTeXFile"
  DeleteRegValue HKCR ".cmake\OpenWithProgids" "Vex.CMakeFile"
  DeleteRegValue HKCR ".mak\OpenWithProgids" "Vex.MakefileFile"
  DeleteRegValue HKCR ".dockerfile\OpenWithProgids" "Vex.DockerfileFile"
  DeleteRegValue HKCR ".vxsyn\OpenWithProgids" "Vex.SyntaxFile"

  DeleteRegValue HKCR ".txt" ""
  DeleteRegValue HKCR ".vxsyn" ""

  DeleteRegKey HKCR ".txt\OpenWithList\${APPNAME}.exe"
  DeleteRegKey HKCR ".log\OpenWithList\${APPNAME}.exe"
  DeleteRegKey HKCR ".md\OpenWithList\${APPNAME}.exe"

  DeleteRegValue HKLM "Software\RegisteredApplications" "${APPNAME}"

  DeleteRegKey HKLM "Software\${APPNAME}\Capabilities"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\App Paths\${APPNAME}.exe"

  DeleteRegKey HKLM "${REGPATH_UNINSTSUBKEY}"

  DeleteRegKey HKLM "Software\${APPNAME}"

  RMDir /r "$INSTDIR"

  MessageBox MB_YESNO|MB_ICONQUESTION \
    "Do you want to remove your ${APPNAME} configuration files?$\r$\n$\r$\n$LOCALAPPDATA\${APPNAME}" \
    IDNO skip_configs

  RMDir /r "$LOCALAPPDATA\${APPNAME}"
  RMDir /r "$APPDATA\${APPNAME}"

  skip_configs:

  Call un.RefreshIconCache
SectionEnd

VIProductVersion "4.1.0.0"
VIAddVersionKey "ProductName" "${APPNAME}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" "© ${VENDOR}"
VIAddVersionKey "FileDescription" "${SUMMARY}"
VIAddVersionKey "CompanyName" "${VENDOR}"
VIAddVersionKey "LegalTrademarks" "${LICENSE}"
VIAddVersionKey "Comments" "Visit ${URL} for more information"
