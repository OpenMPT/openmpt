; OpenMPT Install script
; Written by Johannes Schultz
; http://openmpt.org/
; http://sagamusix.de/

; This file cannot be compiled on its own. You need to compile any of these files:
; win32.iss - For generating the standard Win32 setup.
; win64.iss - For generating the standard Win64 setup.
; install-unmo3-free-itd.iss - For generating the unmo3-free setup with InnoTools Downloader.
; install-unmo3-free.iss - For generating the unmo3-free setup with ISTool downloader.

#define GetAppVersion StringChange(GetFileProductVersion("..\bin\Win32\mptrack.exe"), ",", ".")
#define GetAppVersionShort Copy(GetAppVersion, 1, 4)

#ifndef PlatformName
#error You must specify which installer to build by compiliing either win32.iss or win64.iss
#endif

#ifndef BaseNameAddition
#define BaseNameAddition
#endif

[Setup]
AppVerName=OpenMPT {#GetAppVersionShort} ({#PlatformName})
AppVersion={#GetAppVersion}
AppName=OpenMPT ({#PlatformName})
AppPublisher=OpenMPT Devs / Olivier Lapicque
AppPublisherURL=http://openmpt.org/
AppSupportURL=http://forum.openmpt.org/
AppUpdatesURL=http://openmpt.org/
DefaultDirName={pf}\OpenMPT
DefaultGroupName=OpenMPT
AllowNoIcons=yes
OutputDir=.\
OutputBaseFilename=OpenMPT-{#GetAppVersion}-Setup{#BaseNameAddition}
Compression=lzma2
SolidCompression=yes
WizardImageFile=install-big.bmp
WizardSmallImageFile=install-small.bmp
CreateUninstallRegKey=not IsTaskSelected('portable')
;LicenseFile=license.txt
;The following setting is recommended by the Aero wizard guidelines.
DisableWelcomePage=yes

[Tasks]
; icons and install mode
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}
Name: quicklaunchicon; Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked
#ifdef DOWNLOAD_MO3
Name: downloadmo3; Description: Download unmo3 (library needed for reading MO3 files, recommended); GroupDescription: Options:
#endif
Name: portable; Description: Portable mode (use program folder for storing settings, no registry changes); GroupDescription: Options:; Flags: unchecked
; file associations - put this below all other [tasks]!
#include "filetypes.iss"

[Languages]
Name: english; MessagesFile: compiler:Default.isl

[Files]
; note: packageTemplate\ contains files specific for the "install package".
; for files that are common with the "zip package", use ..\packageTemplate\

; preserve file type order for best solid compression results (first binary, then text)
; home folder
Source: ..\bin\{#PlatformFolder}\mptrack.exe; DestDir: {app}; Flags: ignoreversion
Source: ..\bin\{#PlatformFolder}\PluginBridge32.exe; DestDir: {app}; Flags: ignoreversion
Source: ..\bin\{#PlatformFolder}\PluginBridge64.exe; DestDir: {app}; Flags: ignoreversion
Source: ..\bin\{#PlatformFolder}\OpenMPT_SoundTouch_f32.dll; DestDir: {app}; Flags: ignoreversion
#ifndef DOWNLOAD_MO3
Source: ..\bin\{#PlatformFolder}\unmo3.dll; DestDir: {app}; Flags: ignoreversion
#endif

; Plugins
;Source: ..\packageTemplate\Plugins\*.*; DestDir: {app}\Plugins\; Flags: ignoreversion recursesubdirs createallsubdirs
Source: ..\bin\{#PlatformFolder}\MIDI Input Output.dll; DestDir: {app}\Plugins\MIDI\; Flags: ignoreversion

Source: ..\packageTemplate\ExampleSongs\*.*; DestDir: {app}\ExampleSongs\; Flags: ignoreversion sortfilesbyextension

Source: packageTemplate\readme.txt; DestDir: {app}; Flags: ignoreversion
Source: ..\packageTemplate\History.txt; DestDir: {app}; Flags: ignoreversion
Source: ..\packageTemplate\OpenMPT Manual.chm; DestDir: {app}; Flags: ignoreversion

; release notes
Source: ..\packageTemplate\ReleaseNotesImages\general\*.*; DestDir: {app}\ReleaseNotesImages\general\; Flags: ignoreversion sortfilesbyextension
Source: ..\packageTemplate\ReleaseNotesImages\{#GetAppVersionShort}\*.*; DestDir: {app}\ReleaseNotesImages\{#GetAppVersionShort}\; Flags: ignoreversion sortfilesbyextension
Source: ..\packageTemplate\OMPT_{#GetAppVersionShort}_ReleaseNotes.html; DestDir: {app}; Flags: ignoreversion

; soundtouch license stuff
Source: ..\packageTemplate\SoundTouch\*.*; DestDir: {app}\SoundTouch; Flags: ignoreversion sortfilesbyextension
; keymaps
Source: ..\packageTemplate\ExtraKeymaps\*.*; DestDir: {app}\ExtraKeymaps; Flags: ignoreversion sortfilesbyextension

; kind of auto-backup - handy!
Source: {userappdata}\OpenMPT\Keybindings.mkb; DestDir: {userappdata}\OpenMPT; DestName: Keybindings.mkb.old; Flags: external skipifsourcedoesntexist; Tasks: not portable
Source: {userappdata}\OpenMPT\mptrack.ini; DestDir: {userappdata}\OpenMPT; DestName: mptrack.ini.old; Flags: external skipifsourcedoesntexist; Tasks: not portable
Source: {userappdata}\OpenMPT\SongSettings.ini; DestDir: {userappdata}\OpenMPT; DestName: SongSettings.ini.old; Flags: external skipifsourcedoesntexist; Tasks: not portable
Source: {userappdata}\OpenMPT\plugin.cache; DestDir: {userappdata}\OpenMPT; DestName: plugin.cache.old; Flags: external skipifsourcedoesntexist; Tasks: not portable

[Dirs]
; option dirs for non-portable mode
Name: {userappdata}\OpenMPT; Tasks: not portable
Name: {userappdata}\OpenMPT\tunings; Tasks: not portable
; dirst for portable mode
Name: {app}\tunings; Tasks: portable

[Icons]
; start menu
Name: {group}\OpenMPT; Filename: {app}\mptrack.exe

; app's directory and keymaps directory (for ease of use)
Name: {app}\Configuration files; Filename: {userappdata}\OpenMPT\; Tasks: not portable
Name: {userappdata}\OpenMPT\More Keymaps; Filename: {app}\extraKeymaps\; Tasks: not portable

; desktop, quick launch
Name: {userdesktop}\OpenMPT; Filename: {app}\mptrack.exe; Tasks: desktopicon
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\OpenMPT; Filename: {app}\mptrack.exe; Tasks: quicklaunchicon

[INI]
; enable portable mode
Filename: {app}\mptrack.ini; Section: Paths; Key: UseAppDataDirectory; String: 0; Flags: createkeyifdoesntexist; Tasks: portable
; internet shortcut
Filename: {app}\ModPlug Central.url; Section: InternetShortcut; Key: URL; String: http://forum.openmpt.org/; Flags: createkeyifdoesntexist

[Run]
; duh
Filename: "{app}\OMPT_{#GetAppVersionShort}_ReleaseNotes.html"; Description: "View Release Notes"; Flags: shellexec nowait postinstall skipifsilent
Filename: {app}\mptrack.exe; Parameters: """{app}\ExampleSongs\manwe - evening glow.it"""; Description: {cm:LaunchProgram,OpenMPT}; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; internet shortcut has to be deleted manually
Type: files; Name: {app}\ModPlug Central.url
; normal installation
Type: dirifempty; Name: {userappdata}\OpenMPT\Autosave; Tasks: not portable
Type: dirifempty; Name: {userappdata}\OpenMPT\TemplateModules; Tasks: not portable
Type: dirifempty; Name: {userappdata}\OpenMPT\tunings; Tasks: not portable
Type: dirifempty; Name: {userappdata}\OpenMPT; Tasks: not portable
; portable installation
Type: dirifempty; Name: {app}\Autosave; Tasks: portable
Type: dirifempty; Name: {app}\TemplateModules; Tasks: portable
Type: dirifempty; Name: {app}\tunings; Tasks: portable
#ifdef DOWNLOAD_MO3
Type: files; Name: {app}\unmo3.dll; Tasks: downloadmo3
#endif

#include "utilities.iss"
#include "plugins.iss"

[Code]

#ifdef DOWNLOAD_MO3
procedure VerifyUNMO3Checksum(); forward;
#endif

// Copy old config files to the AppData directory, if there are any (and if the files don't exist already)
procedure CopyConfigsToAppDataDir();
var
    adjustIniPath: Boolean;
    keyFile: String;

begin

    // Not needed if portable mode is enabled.
    if(IsTaskSelected('portable')) then
    begin
        Exit;
    end;

    // If there was an INI file with portable mode flag set, we have to reset it (or else, the mptrack.ini in %appdata% will never be used!)
    if(IniKeyExists('Paths', 'UseAppDataDirectory', ExpandConstant('{app}\mptrack.ini'))) then
    begin
        DeleteIniEntry('Paths', 'UseAppDataDirectory', ExpandConstant('{app}\mptrack.ini'));
    end;

    FileCopy(ExpandConstant('{app}\mptrack.ini'), ExpandConstant('{userappdata}\OpenMPT\mptrack.ini'), true);
    FileCopy(ExpandConstant('{app}\plugin.cache'), ExpandConstant('{userappdata}\OpenMPT\plugin.cache'), true);
    FileCopy(ExpandConstant('{app}\mpt_intl.ini'), ExpandConstant('{userappdata}\OpenMPT\mpt_intl.ini'), true);
    adjustIniPath := FileCopy(ExpandConstant('{app}\Keybindings.mkb'), ExpandConstant('{userappdata}\OpenMPT\Keybindings.mkb'), true);
    adjustIniPath := adjustIniPath or FileCopy(ExpandConstant('{app}\default.mkb'), ExpandConstant('{userappdata}\OpenMPT\Keybindings.mkb'), true);

    // If the keymappings moved, we might have to update the path in the INI file.
    keyFile := GetIniString('Paths', 'Key_Config_File', '', ExpandConstant('{userappdata}\OpenMPT\mptrack.ini'));
    if(((keyFile = ExpandConstant('{app}\Keybindings.mkb')) or (keyFile = ExpandConstant('{app}\default.mkb'))) and (adjustIniPath)) then
    begin
        SetIniString('Paths', 'Key_Config_File', ExpandConstant('{userappdata}\OpenMPT\Keybindings.mkb'), ExpandConstant('{userappdata}\OpenMPT\mptrack.ini'));
    end;

end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
    case CurStep of
    ssPostInstall:
        begin

#ifdef DOWNLOAD_MO3
            VerifyUNMO3Checksum();
#endif

            RegisterPlugin('MIDI\MIDI Input Output.dll');

            // Copy old config files from app's directory, if possible and necessary.
            CopyConfigsToAppDataDir();
        end;
    end;
end;

// Crappy workaround for uninstall stuff
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
    filepath: String;

begin
    case CurUninstallStep of
    usUninstall:
        begin
            if MsgBox('Do you want to keep your OpenMPT settings files (mptrack.ini, SongSettings.ini, Keybindings.mkb, plugin.cache and local_tunings.tc)?', mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDNO then
            begin
                if(GetIniInt('Paths', 'UseAppDataDirectory', 1, 0, 0, ExpandConstant('{app}\mptrack.ini')) = 1) then
                begin
                    filepath := ExpandConstant('{userappdata}\OpenMPT\');
                end else
                    filepath := ExpandConstant('{app}\');
                begin
                end;
                DeleteFile(filepath + 'mptrack.ini');
                DeleteFile(filepath + 'SongSettings.ini');
                DeleteFile(filepath + 'Keybindings.mkb');
                DeleteFile(filepath + 'plugin.cache');
                DeleteFile(filepath + 'tunings\local_tunings.tc');
                DelTree(filepath + 'Autosave\*.Autosave.*', False, True, False);

                filepath := GetTempDir();
                if(filepath <> '') then
                begin
                  filepath := filepath + 'OpenMPT Crash Files\';
                  DelTree(filepath, True, True, True);
                end;
            end;
        end;
    end;
end;
