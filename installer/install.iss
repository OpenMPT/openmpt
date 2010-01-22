; OpenMPT Install script
; Written by Johannes Schultz
; http://sagamusix.de/
; http://sagagames.de/

[Setup]
AppId=OpenMPT
AppVerName=OpenMPT 1.18
AppVersion=1.18.00.00
AppName=OpenMPT
AppPublisher=OpenMPT Devs / Olivier Lapicque
AppPublisherURL=http://www.modplug.com/
AppSupportURL=http://www.modplug.com/
AppUpdatesURL=http://www.modplug.com/
DefaultDirName={pf}\OpenMPT
DefaultGroupName=OpenMPT
AllowNoIcons=yes
OutputDir=.\
OutputBaseFilename=OpenMPT Setup
Compression=lzma
SolidCompression=yes
WizardImageFile=install-big.bmp
WizardSmallImageFile=install-small.bmp
;LicenseFile=license.txt
;CreateUninstallRegKey=no

[Tasks]
; icons and install mode
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}";
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "portable"; Description: "Use program directory to store configuration in"; GroupDescription: "Options:"; Flags: unchecked
; file associations - put this below all other [tasks]!
#include "filetypes.iss"

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"

[Files]
; you may want to change the base paths here

; preserve file type order for best solid compression results (first binary, then text)
; home folder
Source: "..\mptrack\bin\mptrack.exe";                DestDir: "{app}"; Flags: ignoreversion
Source: "..\mptrack\bin\OpenMPT_SoundTouch_i16.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\mptrack\bin\unmo3.dll";                  DestDir: "{app}"; Flags: ignoreversion
Source: "packageTemplate\readme.txt";                DestDir: "{app}"; Flags: ignoreversion
Source: "..\packageTemplate\history.txt";            DestDir: "{app}"; Flags: ignoreversion
; soundtouch license stuff
Source: "..\packageTemplate\SoundTouch\*.*";     DestDir: "{app}\SoundTouch"; Flags: ignoreversion
; keymaps
Source: "..\packageTemplate\extraKeymaps\*.*";   DestDir: "{app}\extraKeymaps"; Flags: ignoreversion

; kind of auto-backup - handy!
Source: "{userappdata}\OpenMPT\mptrack.ini"; DestDir: "{userappdata}\OpenMPT\mptrack.ini.old"; Flags: external skipifsourcedoesntexist; Tasks: not portable
Source: "{userappdata}\OpenMPT\plugin.cache"; DestDir: "{userappdata}\OpenMPT\plugin.cache.old"; Flags: external skipifsourcedoesntexist; Tasks: not portable

[Dirs]
; option dirs for non-portable mode
Name: "{userappdata}\OpenMPT"; Tasks: not portable
Name: "{userappdata}\OpenMPT\tunings"; Tasks: not portable
; dirst for portable mode
Name: "{app}\tunings"; Tasks: portable

[Icons]
; start menu
Name: "{group}\{cm:LaunchProgram,OpenMPT}";           Filename: "{app}\mptrack.exe"
Name: "{group}\{cm:UninstallProgram,OpenMPT}";        Filename: "{uninstallexe}"
Name: "{group}\ModPlug Central";                      Filename: "{app}\ModPlug Central.url"
Name: "{group}\Configuration files";                  Filename: "{userappdata}\OpenMPT\"; Tasks: not portable

; app's directory (for ease of use)
Name: "{app}\Configuration files";                    Filename: "{userappdata}\OpenMPT\"; Tasks: not portable

; desktop, quick launch
Name: "{userdesktop}\OpenMPT";                        Filename: "{app}\mptrack.exe"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\OpenMPT"; Filename: "{app}\mptrack.exe"; Tasks: quicklaunchicon

[INI]
; enable portable mode
Filename: "{app}\mptrack.ini"; Section: "Paths"; Key: "UseAppDataDirectory"; String: "0"; Flags: uninsdeleteentry createkeyifdoesntexist; Tasks: portable
; internet shortcut
Filename: "{app}\ModPlug Central.url"; Section: "InternetShortcut"; Key: "URL"; String: "http://www.lpchip.com/modplug/"; Flags: createkeyifdoesntexist;

[Run]
; duh
Filename: "{app}\mptrack.exe"; Description: "{cm:LaunchProgram,OpenMPT}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; internet shortcut has to be deleted manually
Type: files; Name: "{app}\ModPlug Central.url";
; normal installation
Type: files; Name: "{userappdata}\OpenMPT\mptrack.ini"; Tasks: not portable; Check: DeletePersonalFilesOnUninstall;
Type: files; Name: "{userappdata}\OpenMPT\plugin.cache"; Tasks: not portable; Check: DeletePersonalFilesOnUninstall;
Type: files; Name: "{userappdata}\OpenMPT\tunings\local_tunings.tc"; Tasks: not portable; Check: DeletePersonalFilesOnUninstall;
Type: dirifempty; Name: "{userappdata}\OpenMPT\tunings"; Tasks: not portable
Type: dirifempty; Name: "{userappdata}\OpenMPT"; Tasks: not portable
; portable installation
Type: files; Name: "{app}\mptrack.ini"; Tasks: portable; Check: DeletePersonalFilesOnUninstall;
Type: files; Name: "{app}\plugin.cache"; Tasks: portable; Check: DeletePersonalFilesOnUninstall;
Type: files; Name: "{app}\tunings\local_tunings.tc"; Tasks: portable; Check: DeletePersonalFilesOnUninstall;
Type: dirifempty; Name: "{app}\tunings"; Tasks: portable; Check: DeletePersonalFilesOnUninstall;

; crappy workaround for uninstall stuff
[Code]
var
  deletesettings: Boolean;

function DeletePersonalFilesOnUninstall: Boolean;
begin
  Result := deletesettings;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  case CurUninstallStep of
    usUninstall:
      begin
        if MsgBox('Do you want to keep your personal settings and tunings?', mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then
        begin
          deletesettings := False;
        end else
        begin
          deletesettings := True;
        end;
      end;
  end;
end;

