; ============================================================================
;  reaper_libsndfilewrapper Windows installer (Inno Setup 6).
;
;  This script is invoked by scripts\build_win.bat which passes:
;     /DReaperWrapVersion=X.Y.Z
;     /DReaperWrapStageDir=path\to\staged\plugin\folder
;     /DReaperWrapOutputDir=path\to\put\setup.exe
;  so this file does not need to be edited per-release.
; ============================================================================

#ifndef ReaperWrapVersion
  #error You must invoke ISCC with /DReaperWrapVersion=X.Y.Z (use scripts\build_win.bat)
#endif
#ifndef ReaperWrapStageDir
  #error You must invoke ISCC with /DReaperWrapStageDir=...
#endif
#ifndef ReaperWrapOutputDir
  #error You must invoke ISCC with /DReaperWrapOutputDir=...
#endif

[Setup]
AppId={{A7E4D2C1-9F3B-4D6E-8A12-5C7B3E9F1A24}
AppName=reaper_libsndfilewrapper
AppVersion={#ReaperWrapVersion}
AppPublisher=Matthias Kronlachner
AppPublisherURL=https://www.matthiaskronlachner.com
AppSupportURL=https://www.matthiaskronlachner.com
DefaultDirName={userappdata}\REAPER\UserPlugins
DefaultGroupName=reaper_libsndfilewrapper
DisableProgramGroupPage=yes
DisableDirPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
OutputDir={#ReaperWrapOutputDir}
OutputBaseFilename=reaper_libsndfilewrapper_v{#ReaperWrapVersion}_win64_setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
UninstallDisplayName=reaper_libsndfilewrapper {#ReaperWrapVersion}
LicenseFile={#SourcePath}\..\..\COPYING

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Plugin DLL goes directly into REAPER\UserPlugins. Statically linked
; (libsndfile + WDL + static CRT) — no runtime deps to bundle alongside it.
Source: "{#ReaperWrapStageDir}\reaper_libsndfilewrapper.dll"; \
    DestDir: "{app}"; Flags: ignoreversion
; LGPL license texts (plugin + libsndfile).
Source: "{#ReaperWrapStageDir}\reaper_libsndfilewrapper-licenses\*"; \
    DestDir: "{app}\reaper_libsndfilewrapper-licenses"; Flags: ignoreversion recursesubdirs

[Icons]

[Run]

[Code]
function NeedsAddPath(Param: string): boolean;
begin
  Result := False;
end;
