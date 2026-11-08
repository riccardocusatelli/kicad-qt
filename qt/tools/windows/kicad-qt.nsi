; The installer of KiCad on Qt for Windows (64 bit): the installed tree of the build (bin,
; share\kicad with the libraries, lib\ngspice, etc\fonts) with the start menu, the file type of the
; projects and the uninstaller.
;
;   makensis /DVERSION=10.0.6 /DSOURCE=<the tree> /DOUTFILE=<installer.exe> /DROOT=<sources> kicad-qt.nsi
;
; Silent: installer.exe /S /D=C:\Path (the last argument, without quotes); the uninstaller also
; takes /S.

Unicode true

!ifndef VERSION
  !error "VERSION is not defined"
!endif

!ifndef SOURCE
  !error "SOURCE (the installed tree) is not defined"
!endif

!ifndef ROOT
  !error "ROOT (the sources of KiCad) is not defined"
!endif

!ifndef OUTFILE
  !define OUTFILE "kicad-qt-${VERSION}-x86_64.exe"
!endif

!define PRODUCT "KiCad (Qt)"
!define COMPANY "KiCad"
!define APPKEY "KiCad-Qt"
!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPKEY}"
!define PROGID "KiCadQt.Project"

!include "MUI2.nsh"
!include "x64.nsh"
!include "FileFunc.nsh"

Name "${PRODUCT} ${VERSION}"
OutFile "${OUTFILE}"
InstallDir "$PROGRAMFILES64\${APPKEY}"
InstallDirRegKey HKLM "${UNINSTKEY}" "InstallLocation"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
ShowInstDetails nevershow
ShowUninstDetails nevershow
BrandingText "${PRODUCT} ${VERSION}"

VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${PRODUCT}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "FileDescription" "${PRODUCT} installer"
VIAddVersionKey "LegalCopyright" "The KiCad Developers, GPLv3"

!define MUI_ICON "${ROOT}\resources\bitmaps_png\icons\icon_kicad.ico"
!define MUI_UNICON "${ROOT}\resources\bitmaps_png\icons\icon_kicad.ico"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\kicad-qt.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Start KiCad"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${ROOT}\LICENSE.GPLv3"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"


Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP "${PRODUCT} needs a 64-bit Windows."
    Abort
  ${EndIf}

  SetRegView 64
FunctionEnd


Function un.onInit
  SetRegView 64
FunctionEnd


Section "KiCad" SecMain
  SectionIn RO

  ; The files of a previous version are replaced, not mixed with the new ones
  RMDir /r "$INSTDIR\bin"
  RMDir /r "$INSTDIR\share"
  RMDir /r "$INSTDIR\lib"
  RMDir /r "$INSTDIR\etc"

  SetOutPath "$INSTDIR"
  File /r "${SOURCE}\*.*"

  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; The start menu, for all the users
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\${PRODUCT}"
  CreateShortcut "$SMPROGRAMS\${PRODUCT}\KiCad.lnk" "$INSTDIR\bin\kicad-qt.exe"
  CreateShortcut "$SMPROGRAMS\${PRODUCT}\Uninstall KiCad.lnk" "$INSTDIR\uninstall.exe"

  ; Programs and Features
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayName" "${PRODUCT} ${VERSION}"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "${UNINSTKEY}" "Publisher" "${COMPANY}"
  WriteRegStr HKLM "${UNINSTKEY}" "URLInfoAbout" "https://www.kicad.org"
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayIcon" "$INSTDIR\bin\kicad-qt.exe,0"
  WriteRegStr HKLM "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTKEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "${UNINSTKEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINSTKEY}" "NoRepair" 1

  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "${UNINSTKEY}" "EstimatedSize" "$0"
SectionEnd


Section "Desktop shortcut" SecDesktop
  SetShellVarContext all
  CreateShortcut "$DESKTOP\KiCad (Qt).lnk" "$INSTDIR\bin\kicad-qt.exe"
SectionEnd


Section "Open the projects (.kicad_pro) with KiCad" SecAssoc
  WriteRegStr HKLM "Software\Classes\${PROGID}" "" "KiCad Project"
  WriteRegStr HKLM "Software\Classes\${PROGID}\DefaultIcon" "" "$INSTDIR\bin\kicad-qt.exe,0"
  WriteRegStr HKLM "Software\Classes\${PROGID}\shell\open\command" "" '"$INSTDIR\bin\kicad-qt.exe" "%1"'
  WriteRegStr HKLM "Software\Classes\.kicad_pro\OpenWithProgids" "${PROGID}" ""
  WriteRegStr HKLM "Software\Classes\.kicad_pro" "" "${PROGID}"

  ; The shell sees the new file type
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
SectionEnd


!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} "The programs of KiCad, with the symbol and footprint libraries and the templates."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} "A shortcut of KiCad on the desktop."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecAssoc} "The KiCad projects open in KiCad from the file manager."
!insertmacro MUI_FUNCTION_DESCRIPTION_END


Section "Uninstall"
  ; Only what the installer wrote: not the folder itself if the user has files in it
  RMDir /r "$INSTDIR\bin"
  RMDir /r "$INSTDIR\share"
  RMDir /r "$INSTDIR\lib"
  RMDir /r "$INSTDIR\etc"
  Delete "$INSTDIR\uninstall.exe"
  RMDir "$INSTDIR"

  SetShellVarContext all
  Delete "$SMPROGRAMS\${PRODUCT}\KiCad.lnk"
  Delete "$SMPROGRAMS\${PRODUCT}\Uninstall KiCad.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT}"
  Delete "$DESKTOP\KiCad (Qt).lnk"

  ; The file type, if it is still the one of this installer
  ReadRegStr $0 HKLM "Software\Classes\.kicad_pro" ""
  StrCmp $0 "${PROGID}" 0 +2
    DeleteRegValue HKLM "Software\Classes\.kicad_pro" ""
  DeleteRegValue HKLM "Software\Classes\.kicad_pro\OpenWithProgids" "${PROGID}"
  DeleteRegKey HKLM "Software\Classes\${PROGID}"
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'

  DeleteRegKey HKLM "${UNINSTKEY}"
SectionEnd
