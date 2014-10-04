; example1.nsi
;
; This script is perhaps one of the simplest NSIs you can make. All of the
; optional settings are left to their default settings. The installer simply 
; prompts the user asking them where to install, and drops a copy of example1.nsi
; there. 

;--------------------------------
!include x64.nsh
!include "MUI.nsh"

; The name of the installer
Name "reaper_libsndfilewrapper_win64"

; The file to write
OutFile "reaper_libsndfilewrapper_win64.exe"

; The default installation directory
InstallDir "$PROGRAMFILES64\REAPER (x64)\Plugins"

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
  File ..\_win64\Release\reaper_libsndfilewrapper.dll

  ${DisableX64FSRedirection}

  SetOutPath "$SYSDIR"
  File libs64\libsndfile-1.dll

  ;Microsoft Visual C++ 2012 Redistributable

  ;SetOutPath "$Temp"
  ;File ..\..\win-libs\x64\vcredist_x64.exe

  ;Execwait '"$Temp\vcredist_x64.exe" /q' ; '/q' to install silently
  
SectionEnd ; end the section
