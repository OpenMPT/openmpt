; OpenMPT Install script
; https://openmpt.org/



#define BuildFolder "release\vs2019-win10-static"
#define BuildFolderLegacyx86 "release\vs2019-win7-static"
#define BuildFolderLegacyamd64 "release\vs2019-win7-static"

#define DefaultArchName "x86"

#define GetAppVersion GetFileProductVersion("..\bin\" + BuildFolder + "\" + DefaultArchName + "\OpenMPT.exe")
#define GetAppVersionMedium Copy(GetAppVersion, 1, 7)
#define GetAppVersionShort Copy(GetAppVersion, 1, 4)



[Setup]

AlwaysShowComponentsList=no
AppId={{40c97d3e-7763-4b88-8c6a-0901befee4af}
AppVerName=OpenMPT {#GetAppVersionMedium}
AppVersion={#GetAppVersion}
AppName=OpenMPT
AppPublisher=OpenMPT
AppPublisherURL=https://openmpt.org/
AppSupportURL=https://forum.openmpt.org/
AppUpdatesURL=https://openmpt.org/
ArchitecturesInstallIn64BitMode=x64 arm64 ia64
ChangesAssociations=yes
Compression=lzma2/ultra64
;DefaultDirName={autopf}\OpenMPT
DefaultDirName={code:CodeGetDefaultDirName|}
DefaultGroupName=OpenMPT
DirExistsWarning=auto
DisableDirPage=no
DisableProgramGroupPage=yes
DisableReadyMemo=yes
DisableWelcomePage=no
MinVersion=6.1sp1
OutputDir=.\
OutputBaseFilename=OpenMPT-{#GetAppVersion}-Setup
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=commandline dialog
SetupIconFile=..\mptrack\res\MPTRACK.ICO
SolidCompression=yes
TimeStampsInUTC=yes
UninstallDisplayIcon={app}\OpenMPT App Icon.ico
VersionInfoCopyright=Copyright © 2004-2021 OpenMPT Project Developers and Contributors, Copyright © 1997-2003 Olivier Lapicque
VersionInfoProductTextVersion={#GetAppVersion}
VersionInfoProductVersion={#GetAppVersion}
VersionInfoTextVersion={#GetAppVersion}
VersionInfoVersion={#GetAppVersion}
WizardImageFile=install-big.bmp
WizardSmallImageFile=install-small.bmp
WizardStyle=modern



[Types]

Name: "default"; Description: "Default installation"; Flags: iscustom



[Components]

Name: "archx86"; Description: "OpenMPT x86"; Types: default; Flags: fixed disablenouninstallwarning
Name: "archamd64"; Description: "OpenMPT amd64"; Types: default; Flags: fixed disablenouninstallwarning
Name: "archarm"; Description: "OpenMPT arm"; Types: default; Flags: fixed disablenouninstallwarning
Name: "archarm64"; Description: "OpenMPT arm64"; Types: default; Flags: fixed disablenouninstallwarning



[Languages]

Name: en; MessagesFile: compiler:Default.isl



[Files]

; note: packageTemplate\ contains files specific for the "install package".
; for files that are common with the "zip package", use ..\packageTemplate\

Source: ..\bin\{#BuildFolder}\x86\OpenMPT.exe; DestDir: {app}\bin\x86; Flags: ignoreversion; Components: archx86; MinVersion: 10.0
Source: ..\bin\{#BuildFolder}\x86\PluginBridge-x86.exe; DestDir: {app}\bin\x86; Flags: ignoreversion; Components: archx86; MinVersion: 10.0
Source: ..\bin\{#BuildFolder}\x86\PluginBridgeLegacy-x86.exe; DestDir: {app}\bin\x86; Flags: ignoreversion; Components: archx86; MinVersion: 10.0
Source: ..\bin\{#BuildFolder}\x86\openmpt-lame.dll; DestDir: {app}\bin\x86; Flags: ignoreversion; Components: archx86; MinVersion: 10.0
Source: ..\bin\{#BuildFolder}\x86\openmpt-mpg123.dll; DestDir: {app}\bin\x86; Flags: ignoreversion; Components: archx86; MinVersion: 10.0
Source: ..\bin\{#BuildFolder}\x86\openmpt-soundtouch.dll; DestDir: {app}\bin\x86; Flags: ignoreversion; Components: archx86; MinVersion: 10.0
Source: ..\bin\{#BuildFolderLegacyx86}\x86\OpenMPT.exe; DestDir: {app}\bin\x86; Flags: ignoreversion; Components: archx86; OnlyBelowVersion: 10.0
Source: ..\bin\{#BuildFolderLegacyx86}\x86\PluginBridge-x86.exe; DestDir: {app}\bin\x86; Flags: ignoreversion; Components: archx86; OnlyBelowVersion: 10.0
Source: ..\bin\{#BuildFolderLegacyx86}\x86\PluginBridgeLegacy-x86.exe; DestDir: {app}\bin\x86; Flags: ignoreversion; Components: archx86; OnlyBelowVersion: 10.0
Source: ..\bin\{#BuildFolderLegacyx86}\x86\openmpt-lame.dll; DestDir: {app}\bin\x86; Flags: ignoreversion; Components: archx86; OnlyBelowVersion: 10.0
Source: ..\bin\{#BuildFolderLegacyx86}\x86\openmpt-mpg123.dll; DestDir: {app}\bin\x86; Flags: ignoreversion; Components: archx86; OnlyBelowVersion: 10.0
Source: ..\bin\{#BuildFolderLegacyx86}\x86\openmpt-soundtouch.dll; DestDir: {app}\bin\x86; Flags: ignoreversion; Components: archx86; OnlyBelowVersion: 10.0

Source: ..\bin\{#BuildFolder}\amd64\OpenMPT.exe; DestDir: {app}\bin\amd64; Flags: ignoreversion; Components: archamd64; MinVersion: 10.0
Source: ..\bin\{#BuildFolder}\amd64\PluginBridge-amd64.exe; DestDir: {app}\bin\amd64; Flags: ignoreversion; Components: archamd64; MinVersion: 10.0
Source: ..\bin\{#BuildFolder}\amd64\PluginBridgeLegacy-amd64.exe; DestDir: {app}\bin\amd64; Flags: ignoreversion; Components: archamd64; MinVersion: 10.0
Source: ..\bin\{#BuildFolder}\amd64\openmpt-lame.dll; DestDir: {app}\bin\amd64; Flags: ignoreversion; Components: archamd64; MinVersion: 10.0
Source: ..\bin\{#BuildFolder}\amd64\openmpt-mpg123.dll; DestDir: {app}\bin\amd64; Flags: ignoreversion; Components: archamd64; MinVersion: 10.0
Source: ..\bin\{#BuildFolder}\amd64\openmpt-soundtouch.dll; DestDir: {app}\bin\amd64; Flags: ignoreversion; Components: archamd64; MinVersion: 10.0
Source: ..\bin\{#BuildFolderLegacyamd64}\amd64\OpenMPT.exe; DestDir: {app}\bin\amd64; Flags: ignoreversion; Components: archamd64; OnlyBelowVersion: 10.0
Source: ..\bin\{#BuildFolderLegacyamd64}\amd64\PluginBridge-amd64.exe; DestDir: {app}\bin\amd64; Flags: ignoreversion; Components: archamd64; OnlyBelowVersion: 10.0
Source: ..\bin\{#BuildFolderLegacyamd64}\amd64\PluginBridgeLegacy-amd64.exe; DestDir: {app}\bin\amd64; Flags: ignoreversion; Components: archamd64; OnlyBelowVersion: 10.0
Source: ..\bin\{#BuildFolderLegacyamd64}\amd64\openmpt-lame.dll; DestDir: {app}\bin\amd64; Flags: ignoreversion; Components: archamd64; OnlyBelowVersion: 10.0
Source: ..\bin\{#BuildFolderLegacyamd64}\amd64\openmpt-mpg123.dll; DestDir: {app}\bin\amd64; Flags: ignoreversion; Components: archamd64; OnlyBelowVersion: 10.0
Source: ..\bin\{#BuildFolderLegacyamd64}\amd64\openmpt-soundtouch.dll; DestDir: {app}\bin\amd64; Flags: ignoreversion; Components: archamd64; OnlyBelowVersion: 10.0

Source: ..\bin\{#BuildFolder}\arm\OpenMPT.exe; DestDir: {app}\bin\arm; Flags: ignoreversion; Components: archarm
Source: ..\bin\{#BuildFolder}\arm\PluginBridge-arm.exe; DestDir: {app}\bin\arm; Flags: ignoreversion; Components: archarm
Source: ..\bin\{#BuildFolder}\arm\PluginBridgeLegacy-arm.exe; DestDir: {app}\bin\arm; Flags: ignoreversion; Components: archarm
Source: ..\bin\{#BuildFolder}\arm\openmpt-lame.dll; DestDir: {app}\bin\arm; Flags: ignoreversion; Components: archarm
Source: ..\bin\{#BuildFolder}\arm\openmpt-mpg123.dll; DestDir: {app}\bin\arm; Flags: ignoreversion; Components: archarm
Source: ..\bin\{#BuildFolder}\arm\openmpt-soundtouch.dll; DestDir: {app}\bin\arm; Flags: ignoreversion; Components: archarm

Source: ..\bin\{#BuildFolder}\arm64\OpenMPT.exe; DestDir: {app}\bin\arm64; Flags: ignoreversion; Components: archarm64
Source: ..\bin\{#BuildFolder}\arm64\PluginBridge-arm64.exe; DestDir: {app}\bin\arm64; Flags: ignoreversion; Components: archarm64
Source: ..\bin\{#BuildFolder}\arm64\PluginBridgeLegacy-arm64.exe; DestDir: {app}\bin\arm64; Flags: ignoreversion; Components: archarm64
Source: ..\bin\{#BuildFolder}\arm64\openmpt-lame.dll; DestDir: {app}\bin\arm64; Flags: ignoreversion; Components: archarm64
Source: ..\bin\{#BuildFolder}\arm64\openmpt-mpg123.dll; DestDir: {app}\bin\arm64; Flags: ignoreversion; Components: archarm64
Source: ..\bin\{#BuildFolder}\arm64\openmpt-soundtouch.dll; DestDir: {app}\bin\arm64; Flags: ignoreversion; Components: archarm64

Source: "..\mptrack\res\MPTRACK.ICO"; DestName: "OpenMPT App Icon.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\packageTemplate\OpenMPT File Icon.ico"; DestName: "OpenMPT File Icon.ico"; DestDir: "{app}"; Flags: ignoreversion

Source: ..\bin\{#BuildFolder}\{#DefaultArchName}\openmpt-wine-support.zip; DestDir: {app}; Flags: ignoreversion

; todo: use proper folder
Source: ..\packageTemplate\ExampleSongs\*.*; DestDir: {app}\ExampleSongs\; Flags: ignoreversion recursesubdirs

Source: packageTemplate\readme.txt; DestDir: {app}; Flags: ignoreversion
Source: ..\packageTemplate\History.txt; DestDir: {app}; Flags: ignoreversion
Source: ..\packageTemplate\OpenMPT Manual.chm; DestDir: {app}; Flags: ignoreversion

Source: ..\packageTemplate\ReleaseNotesImages\*.*; DestDir: {app}\ReleaseNotesImages\; Flags: ignoreversion
Source: ..\packageTemplate\Release Notes.html; DestDir: {app}; Flags: ignoreversion

Source: ..\packageTemplate\OpenMPT Support and Community Forum.url; DestDir: {app}; Flags: ignoreversion
Source: ..\packageTemplate\OpenMPT Issue Tracker.url; DestDir: {app}; Flags: ignoreversion

Source: ..\packageTemplate\License.txt; DestDir: {app}; Flags: ignoreversion
Source: ..\packageTemplate\Licenses\*.*; DestDir: {app}\Licenses; Flags: ignoreversion

Source: ..\packageTemplate\ExtraKeymaps\*.*; DestDir: {app}\ExtraKeymaps; Flags: ignoreversion



[Icons]

Name: {autodesktop}\OpenMPT; Filename: {app}\bin\x86\OpenMPT.exe; Check: CheckDefaultArch('x86')
Name: {autodesktop}\OpenMPT; Filename: {app}\bin\amd64\OpenMPT.exe; Check: CheckDefaultArch('amd64')
Name: {autodesktop}\OpenMPT; Filename: {app}\bin\arm\OpenMPT.exe; Check: CheckDefaultArch('arm')
Name: {autodesktop}\OpenMPT; Filename: {app}\bin\arm64\OpenMPT.exe; Check: CheckDefaultArch('arm64')

Name: {group}\OpenMPT; Filename: {app}\bin\x86\OpenMPT.exe; Check: CheckDefaultArch('x86')
Name: {group}\OpenMPT; Filename: {app}\bin\amd64\OpenMPT.exe; Check: CheckDefaultArch('amd64')
Name: {group}\OpenMPT; Filename: {app}\bin\arm\OpenMPT.exe; Check: CheckDefaultArch('arm')
Name: {group}\OpenMPT; Filename: {app}\bin\arm64\OpenMPT.exe; Check: CheckDefaultArch('arm64')

Name: {group}\OpenMPT (x86); Filename: {app}\bin\x86\OpenMPT.exe; Components: archx86
Name: {group}\OpenMPT (amd64); Filename: {app}\bin\amd64\OpenMPT.exe; Components: archamd64
Name: {group}\OpenMPT (arm); Filename: {app}\bin\arm\OpenMPT.exe; Components: archarm
Name: {group}\OpenMPT (arm64); Filename: {app}\bin\arm64\OpenMPT.exe; Components: archarm64

Name: {group}\Manual; Filename: {app}\OpenMPT Manual.chm
Name: {group}\Support and Community Forum; Filename: {app}\OpenMPT Support and Community Forum.url



[Registry]

#include "filetypes-multi-arch.iss"



[Run]

Filename: "{app}\Release Notes.html"; Description: "View Release Notes"; Flags: runasoriginaluser shellexec nowait postinstall skipifsilent

Filename: {app}\bin\x86\OpenMPT.exe; Parameters: """{code:RandomExampleFile}"""; Description: {cm:LaunchProgram,OpenMPT}; Flags: runasoriginaluser nowait postinstall skipifsilent; Check: CheckDefaultArch('x86')
Filename: {app}\bin\amd64\OpenMPT.exe; Parameters: """{code:RandomExampleFile}"""; Description: {cm:LaunchProgram,OpenMPT}; Flags: runasoriginaluser nowait postinstall skipifsilent; Check: CheckDefaultArch('amd64')
Filename: {app}\bin\arm\OpenMPT.exe; Parameters: """{code:RandomExampleFile}"""; Description: {cm:LaunchProgram,OpenMPT}; Flags: runasoriginaluser nowait postinstall skipifsilent; Check: CheckDefaultArch('arm')
Filename: {app}\bin\arm64\OpenMPT.exe; Parameters: """{code:RandomExampleFile}"""; Description: {cm:LaunchProgram,OpenMPT}; Flags: runasoriginaluser nowait postinstall skipifsilent; Check: CheckDefaultArch('arm64')



[InstallDelete]

; as recommended by Inno Setup manual on [Components]

Type: files; Name: {app}\bin\x86\OpenMPT.exe
Type: files; Name: {app}\bin\x86\PluginBridge-x86.exe
Type: files; Name: {app}\bin\x86\PluginBridgeLegacy-x86.exe
Type: files; Name: {app}\bin\x86\openmpt-lame.dll
Type: files; Name: {app}\bin\x86\openmpt-mpg123.dll
Type: files; Name: {app}\bin\x86\openmpt-soundtouch.dll

Type: files; Name: {app}\bin\amd64\OpenMPT.exe
Type: files; Name: {app}\bin\amd64\PluginBridge-amd64.exe
Type: files; Name: {app}\bin\amd64\PluginBridgeLegacy-amd64.exe
Type: files; Name: {app}\bin\amd64\openmpt-lame.dll
Type: files; Name: {app}\bin\amd64\openmpt-mpg123.dll
Type: files; Name: {app}\bin\amd64\openmpt-soundtouch.dll

Type: files; Name: {app}\bin\arm\OpenMPT.exe
Type: files; Name: {app}\bin\arm\PluginBridge-arm.exe
Type: files; Name: {app}\bin\arm\PluginBridgeLegacy-arm.exe
Type: files; Name: {app}\bin\arm\openmpt-lame.dll
Type: files; Name: {app}\bin\arm\openmpt-mpg123.dll
Type: files; Name: {app}\bin\arm\openmpt-soundtouch.dll

Type: files; Name: {app}\bin\arm64\OpenMPT.exe
Type: files; Name: {app}\bin\arm64\PluginBridge-arm64.exe
Type: files; Name: {app}\bin\arm64\PluginBridgeLegacy-arm64.exe
Type: files; Name: {app}\bin\arm64\openmpt-lame.dll
Type: files; Name: {app}\bin\arm64\openmpt-mpg123.dll
Type: files; Name: {app}\bin\arm64\openmpt-soundtouch.dll



#include "uninstall-single-arch.iss"



[Code]

procedure InitializeWizard();
begin
	WizardSelectComponents('archx86,!archamd64,!archarm,!archarm64');
	case ProcessorArchitecture() of
		paUnknown:
			begin
				WizardSelectComponents('archx86,!archamd64,!archarm,!archarm64');
			end;
		paX86:
			begin
				WizardSelectComponents('archx86,!archamd64,!archarm,!archarm64');
			end;
		paX64:
			begin
				WizardSelectComponents('archx86,archamd64,!archarm,!archarm64');
			end;
		paIA64:
			begin
				WizardSelectComponents('archx86,!archamd64,!archarm,!archarm64');
			end;
		paARM64:
			begin
				WizardSelectComponents('archx86,archamd64,archarm,archarm64');
			end;
	end;
end;

function GetDefaultArch(): String;
begin
	Result := 'x86';
	case ProcessorArchitecture() of
		paUnknown:
			begin
				Result := 'x86';
			end;
		paX86:
			begin
				Result := 'x86';
			end;
		paX64:
			begin
				Result := 'amd64';
			end;
		paIA64:
			begin
				Result := 'x86';
			end;
		paARM64:
			begin
				Result := 'arm64';
			end;
	end;
end;

function CodeGetDefaultArch(Param: String): String;
begin
	Result := GetDefaultArch();
end;

function CheckDefaultArch(Arch: String): Boolean;
begin
	Result := Arch = GetDefaultArch();
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
	Result := False;
	case PageID of
		wpSelectDir:
			begin
				Result := IsAdmin() and HasPreviousSingleArchInstallPath();
			end;
	end;
	case PageID of
		wpSelectComponents:
			begin
				Result := True;
			end;
	end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
	case CurStep of
		ssInstall:
			begin
				if not UninstallSingleArch() then
				begin
					RaiseException('Uninstallation of previous OpenMPT installation failed. Please uninstall manually.');
				end;
			end;
	end;
end;

// Picks a random example song file to play
Function RandomExampleFile(Dummy: String): String;
var
	Files: TstringList;
	FindRec: TFindRec;
begin
	Result := '';
	if FindFirst(ExpandConstant('{app}\ExampleSongs\*'), FindRec) then
	try
		Files := TstringList.Create;
		repeat
			if FindRec.Attributes and FILE_ATTRIBUTE_DIRECTORY = 0 then
			Files.Add(FindRec.Name);
		until not FindNext(FindRec);
		Result := ExpandConstant('{app}\ExampleSongs\') + Files[Random(Files.Count)];
	finally
		FindClose(FindRec);
	end;
end;

function CodeGetDefaultDirName(Param: String): String;
begin
	if IsAdmin() and HasPreviousSingleArchInstallPath() then
	begin
		Result := GetPreviousSingleArchInstallPath();
	end
	else
	begin
		Result := ExpandConstant('{autopf}\OpenMPT');
	end;
end;


