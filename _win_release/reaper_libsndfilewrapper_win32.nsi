; example1.nsi
;
; This script is perhaps one of the simplest NSIs you can make. All of the
; optional settings are left to their default settings. The installer simply 
; prompts the user asking them where to install, and drops a copy of example1.nsi
; there. 

;--------------------------------
!include "MUI.nsh"

; The name of the installer
Name "reaper_libsndfilewrapper_win32"

; The file to write
OutFile "reaper_libsndfilewrapper_win32.exe"

; The default installation directory
InstallDir $PROGRAMFILES\REAPER\Plugins

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

; Pages
!insertmacro MUI_PAGE_LICENSE "../README.md"

Page directory
Page instfiles

;--------------------------------

; The stuff to install
Section "" ;No components page, name is not important

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File ..\_win32\Release\reaper_libsndfilewrapper.dll

  SetOutPath "$SYSDIR"

  File libs32\libsndfile-1.dll

  ;Microsoft Visual C++ 2012 Redistributable

  ;SetOutPath "$Temp"
  ;File ..\..\win-libs\x86\vcredist_x86.exe

  ;Execwait '"$Temp\vcredist_x86.exe" /q' ; '/q' to install silently

SectionEnd ; end the section
